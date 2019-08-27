/*
   pdelse.c

  Copyright (C) 2019 Eduardo Moguillansky

  This file is part of Csound.

  The Csound Library is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Csound is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA

  */

/*

  # crackle

    aout  crackle kp

    generates noise based on a chaotic equation
    y[n] = p * y[n-1]- y[n-2] - 0.05
    
    kp: the p parameter in the equation, a value between 1.0 and 2.0

  # ramptrig

    a resetable line going from 0 to 1

    kout             ramptrig ktrig, kdur, ivaluepost=1
    aout             ramptrig ktrig, kdur, ivaluepost=1
    kout, kfinished  ramptrig ktrig, kdur, ivaluepost=1

    ktrig: whenever ktrig > 0 and ktrig > previous trig the outvalue is reset to 0
    kdur: duration of the ramp (how long does it take to go from 0 to 1)
    ivaluepost: value after reaching end of ramp (1)
    ivaluepre: value before any trigger (0)

    kout: the ramp value. Will go from 0 to 1 in kdur. If the ramp reaches 1,
        kout will be set to ivaluepost (1). Before any trigger, value is ivaluepre (0)

    Main usecase is in connection to bpf, to produce something similar to Env and EnvGen
    in supercollider

    kphase ramptrig ktrig, idur
    kenv bpf kphase * idur, 0, 0, 0.01, 1, idur, 0
    asig = pinker() * interp(kenv)

  # rampgate

    kout rampgate kgate, ival0, iattack, ival1, irel, ival2

    kramp rampgate kgate, 0, 0.2, 1, 0.5, 2
    kenv bpf kramp 0, 0, 1, 1, 2, 0

  # sigmdrive  (sigmoid drive)

    aout sigmdrive ain, xfactor, kmode=0

    xfactor: drive factor (k or a)
    kmode: 0 = tanh, 1 = pow

  # lfnoise

    aout lfnoise kfreq, kinterp=0

    low frequency, band-limited noise

  # schmitt

    A schmitt trigger

    kout schmitt kin, khigh, klow
    aout schmitt ain, khigh, klow

    output value is either 1 if in > high, or 0 if in < low


  # standardchaos

    Standard map chaotic generator, the sound is generated with the difference equations;
    y[n] = (y[n-1] + k * sin(x[n-1])) % 2pi;
    x[n] = (x[n-1] + y[n]) % 2pi;
    out = (x[n] - pi) / pi;

    aout standardchaos krate, kx=0.5, ky=0

  # rampgate

    an envelope generator similar to linsegr but with retriggerable gate and
    flexible sustain point

    aout/kout rampgate kgate, isustidx, kval0, kdel0, kval1, kdel1, ..., kvaln

    when kgate changes from 0 to 1, value follows envelope until isustpoint is reached
    value stays there until kgate switches to 0, after which it follow the rest
    of the envelope and stays at the env until a new switch. If kgate switches to 0
    before reaching isustpoint, then it uses the current value as sustain point and
    follows the envelope from there. If kgate switches to 1 before reaching the
    end of the release part, it glides to the beginning and starts attack envelope
    from there

    inspired by else's envgen

    Example: generate an adsr envelope with attack=0.05, decay=0.1, sust=0.2, rel=0.5

    isustidx = 2
    aout rampgate kgate, isustidx, 0, 0.05, 1, 0.1, 0.2, 0.5, 0

    Example 2: emulate ramptrig

        These are the same:
            kout ramptrig ktrig, idur
            kout rampgate ktrig, -1, 0, idur, 1

*/

#include "csdl.h"
// #include "arrays.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

#define INITERR(m) (csound->InitError(csound, "%s", m))
#define INITERRF(fmt, ...) (csound->InitError(csound, fmt, __VA_ARGS__))
#define PERFERR(m) (csound->PerfError(csound, &(p->h), "%s", m))
#define PERFERRF(fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))

#define UI32MAX 0x7FFFFFFF

#define ONE_OVER_PI 0.3183098861837907

// #define DEBUG

#ifdef DEBUG
    #define DBG(fmt, ...) printf(">>>> "fmt"\n", __VA_ARGS__); fflush(stdout);
    #define DBG_(m) DBG("%s", m)
#else
    #define DBG(fmt, ...)
    #define DBG_(m)
#endif


#define SAMPLE_ACCURATE(out) \
    uint32_t n, nsmps = CS_KSMPS;                                    \
    uint32_t offset = p->h.insdshead->ksmps_offset;                  \
    uint32_t early = p->h.insdshead->ksmps_no_end;                   \
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));   \
    if (UNLIKELY(early)) {                                           \
        nsmps -= early;                                              \
        memset(&out[nsmps], '\0', early*sizeof(MYFLT));              \
    }                                                                \



// ----------------- opcode A ------------------

typedef struct {
    OPDS h;
    MYFLT *aout;
    MYFLT *kp;
    MYFLT y1;
    MYFLT y2;
} CRACKLE;

static inline MYFLT rnd31(CSOUND *csound, int*seed) {
    return (MYFLT) (csound->Rand31(seed) - 1) / FL(2147483645.0);
}

static int32_t crackle_init(CSOUND *csound, CRACKLE* p) {
    MYFLT kp = *p->kp;
    if (kp < 0) {
        *p->kp = 1.5;
    }
    int seed = csound->GetRandomSeedFromTime();
    p->y1 = rnd31(csound, &seed);
    // printf("y1 = %f\n", p->y1);
    p->y2 = 0;
    return OK;
}

static int32_t crackle_perf(CSOUND *csound, CRACKLE* p) {
    IGN(csound);
    MYFLT *out = p->aout;                                             \

    SAMPLE_ACCURATE(out)

    MYFLT y0;
    MYFLT kp = *p->kp;
    MYFLT y1 = p->y1;
    MYFLT y2 = p->y2;

    for(n=offset; n<nsmps; n++) {
        y0 = fabs(y1 * kp - y2 - FL(0.05));
        y2 = y1; y1 = y0;
        out[n] = y0;
    }
    p->y1 = y1;
    p->y2 = y2;

    return OK;
}


// ramptrig
// kout ramptrig ktrig, kdur, ivalpost=1, ivalpre=0
// ramp from 0 to 1 in kdur seconds, sets output to ivaloff when finished

typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *ktrig;
    MYFLT *kdur;
    MYFLT *ivalpost;
    MYFLT ivalpre;
    MYFLT lasttrig;
    MYFLT now;
    MYFLT sr;
    int started;
} RAMPTRIGK;


static int32_t ramptrig_k_kk_init(CSOUND *csound, RAMPTRIGK *p) {
    p->lasttrig = 0;
    p->sr = csound->GetKr(csound);
    p->now = 0;
    p->started = 0;
    p->ivalpre = 0;
    return OK;
}


static int32_t ramptrig_k_kk(CSOUND *csound, RAMPTRIGK *p) {
    IGN(csound);
    MYFLT ktrig = *p->ktrig;
    MYFLT now = p->now;
    MYFLT delta = 1 / (*p->kdur * p->sr);
    if(ktrig > 0 && ktrig > p->lasttrig) {
        p->started = 1;
        p->now = 0;
    } else if(!p->started) {
        *p->out = p->ivalpre;
    } else if(now < 1) {
        *p->out = now;
        p->now += delta;
    } else {
        *p->out = *p->ivalpost;
    }
    p->lasttrig = ktrig;
    return OK;
}

static int32_t ramptrig_a_kk_init(CSOUND *csound, RAMPTRIGK *p) {
    p->lasttrig = 0;
    p->sr = csound->GetSr(csound);
    p->now = 0;
    p->started = 0;
    p->ivalpre = 0;
    return OK;
}


static int32_t ramptrig_a_kk(CSOUND *csound, RAMPTRIGK *p) {
    IGN(csound);
    MYFLT *out = p->out;

    SAMPLE_ACCURATE(out)

    MYFLT ktrig = *p->ktrig;
    MYFLT now = p->now;
    MYFLT delta = 1 / (*p->kdur * p->sr);
    if(ktrig > 0 && ktrig > p->lasttrig) {
        p->started = 1;
        p->now = 0;
    } else if(!p->started) {
        MYFLT ivalpre = p->ivalpre;
        for(n=offset; n<nsmps; n++) {
            out[n] = ivalpre;
        }
    } else if(now < 1) {
        for(n=offset; n<nsmps; n++) {
            out[n] = now;
            now += delta;
        }
        p->now = now;
    } else {
        MYFLT ivalpost = *p->ivalpost;
        for(n=offset; n<nsmps; n++)
            out[n] = ivalpost;
    }
    p->lasttrig = ktrig;
    return OK;
}


// aout, afinished ramptrig atrig, kdur
typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *sync;
    MYFLT *trig;
    MYFLT *dur;
    MYFLT *ivalpost;
    MYFLT ivalpre;
    MYFLT lasttrig;
    MYFLT lastsync;
    MYFLT now;
    MYFLT sr;
    int started;
} RAMPTRIGSYNC;


static int32_t ramptrigsync_kk_kk_init(CSOUND *csound, RAMPTRIGSYNC *p) {
    p->lasttrig = 0;
    p->now = 0;
    p->sr = csound->GetKr(csound);
    p->lastsync = 0;
    p->started = 0;
    p->ivalpre = 0;
    return OK;
}

static int32_t ramptrigsync_kk_kk(CSOUND *csound, RAMPTRIGSYNC *p) {
    IGN(csound);
    MYFLT ktrig = *p->trig;
    MYFLT now = p->now;
    MYFLT delta = 1 / (*p->dur * p->sr);
    if(ktrig > 0 && ktrig > p->lasttrig) {
        p->started = 1;
        p->now = 0;
    } else if(!p->started) {
        *p->out = p->ivalpre;
        *p->sync = 0;
    } else if(now < 1) {
        *p->out = now;
        p->now += delta;
        *p->sync = 0;
        p->lastsync = 0;
    } else {
        *p->out = *p->ivalpost;
        *p->sync = 1 - p->lastsync;
        p->lastsync = 1;
    }
    p->lasttrig = ktrig;
    return OK;
}


// sigmdrive
// aout sigmdrive ain, xdrivefactor, kmode=0

typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *in;
    MYFLT *drivefactor;
    MYFLT *mode;

} SIGMDRIVE;

static int32_t sigmdrive_a_ak(CSOUND *csound, SIGMDRIVE * p) {
    IGN(csound);
    MYFLT *in = p->in;
    MYFLT *out = p->out;
    MYFLT drivefactor = *p->drivefactor;
    if(drivefactor < 0)
        drivefactor = 0;
    MYFLT x;
    SAMPLE_ACCURATE(out);
    if(*p->mode == 0.) {
        for(n=offset; n<nsmps; n++) {
            out[n] = tanh(in[n] * drivefactor);
        }
    } else {
        for(n=offset; n<nsmps; n++) {
            x = in[n];
            if(x >= 1.)
                out[n] = 1.;
            else if (x <= -1)
                out[n] = 1.0;
            else if (drivefactor < 1)
                out[n] = x * drivefactor;
            else if (x > 0)
                out[n] = 1.0 - pow(1. - x, drivefactor);
            else
                out[n] = pow(1.0 + x, drivefactor) - 1.0;
        }
    }
    return OK;
}


static int32_t sigmdrive_a_aa(CSOUND *csound, SIGMDRIVE * p) {
    IGN(csound);
    MYFLT *in = p->in;
    MYFLT *out = p->out;
    MYFLT *drivefactorp = p->drivefactor;
    MYFLT drivefactor;
    MYFLT x;
    SAMPLE_ACCURATE(out);
    int mode = (int)*p->mode;
    if(mode == 0) {
        for(n=offset; n<nsmps; n++) {
            drivefactor = drivefactorp[n];
            if(drivefactor < 0)
                drivefactor = 0;
            out[n] = tanh(in[n] * drivefactor);
        }
    } else {
        for(n=offset; n<nsmps; n++) {
            x = in[n];
            drivefactor = drivefactorp[n];
            if(drivefactor < 0)
                drivefactor = 0;
            if(x >= 1.)
                out[n] = 1.;
            else if (x <= -1)
                out[n] = 1.0;
            else if (drivefactor < 1)
                out[n] = x * drivefactor;
            else if (x > 0)
                out[n] = 1.0 - pow(1. - x, drivefactor);
            else
                out[n] = pow(1.0 + x, drivefactor) - 1.0;
        }
    }
    return OK;
}


// lfnoise
// aout lfnoise kfreq, kinterp=0

typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *freq;
    MYFLT *interp;

    MYFLT x_freq;
    int   x_val;
    int x_interp;
    double x_phase;
    MYFLT x_ynp1;
    MYFLT x_yn;
    MYFLT x_sr;
} LFNOISE;


static int32_t lfnoise_init(CSOUND *csound, LFNOISE *p) {
    int init_seed = (int)csound->GetRandomSeedFromTime();
    // static int init_seed = 234599;
    init_seed *= 1319;
    // default parameters
    MYFLT hz = *p->freq;
    int seed = init_seed;
    int interp = 0;
    if (hz >= 0)
        p->x_phase = 1.;
    p->x_freq = hz;
    p->x_interp = interp != 0;
    // get 1st output
    p->x_ynp1 = (((MYFLT)((seed & 0x7fffffff) - 0x40000000)) * (MYFLT)(1.0 / 0x40000000));
    p->x_val = seed * 435898247 + 382842987;
    p->x_sr = csound->GetSr(csound);
    return OK;
}

static int32_t lfnoise_perf(CSOUND *csound, LFNOISE *p) {
    IGN(csound);
    MYFLT *out = p->out;

    SAMPLE_ACCURATE(out);

    MYFLT hz = *p->freq;
    int interp = (int)*p->interp;
    int val = p->x_val;
    double phase = p->x_phase;
    MYFLT ynp1 = p->x_ynp1;
    MYFLT yn = p->x_yn;
    MYFLT sr = p->x_sr;
    MYFLT random;
    for(n=offset; n<nsmps; n++) {
        double phase_step = hz / sr;
        // clipped phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step;
        if (hz >= 0) {
            if (phase >= 1.) {    // update
                random = ((MYFLT)((val & 0x7fffffff) - 0x40000000)) * (MYFLT)(1.0 / 0x40000000);
                val = val * 435898247 + 382842987;
                phase = phase - 1;
                yn = ynp1;
                ynp1 = random; // next random value
            }
        } else {
           if (phase <= 0.) {    // update
               random = ((MYFLT)((val & 0x7fffffff) - 0x40000000)) * (MYFLT)(1.0 / 0x40000000);
               val = val * 435898247 + 382842987;
               phase = phase + 1;
               yn = ynp1;
               ynp1 = random; // next random value
           }
        }
        if (interp) {
            if (hz >= 0)
                *out++ = yn + (ynp1 - yn) * (phase);
            else
                *out++ = yn + (ynp1 - yn) * (1 - phase);
        }
        else {
            out[n] = yn;
        }

        phase += phase_step;
   }
   p->x_val = val;
   p->x_phase = phase;
   p->x_ynp1 = ynp1;   // next random value
   p->x_yn = yn;       // current output
   return OK;
}


// schmitt
// kout schmitt kin, khigh, klow

typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *in;
    MYFLT *high;
    MYFLT *low;
    MYFLT last;
} SCHMITT;

static int32_t schmitt_k_init (CSOUND *csound, SCHMITT *p) {
    IGN(csound);
    p->last =0;
    return OK;
}

static int32_t schmitt_k_perf (CSOUND *csound, SCHMITT *p) {
    IGN(csound);
    MYFLT last = p->last;
    MYFLT in = *p->in;
    MYFLT lo = *p->low;
    MYFLT hi = *p->high;
    *p->out = last = (in > lo && (in >= hi || last));
    p->last = last;
    return OK;
}

static int32_t schmitt_a_perf (CSOUND *csound, SCHMITT *p) {
    MYFLT *out = p->out;
    MYFLT x;
    SAMPLE_ACCURATE(out);

    MYFLT *in = p->in;
    MYFLT lo = *p->low;
    MYFLT hi = *p->high;
    MYFLT last = p->last;

    for(n=offset; n<nsmps; n++) {
        x = in[n];
        out[n] = last = (x > lo && (x >= hi || last));
    }
    p->last = last;
    return OK;
}

// standardchaos
// aout standardchaos krate, kk=1, ix=0.5, iy=0

typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *rate;
    MYFLT *k;
    MYFLT *x;
    MYFLT *y;

    int x_val;
    MYFLT  x_sr;
    MYFLT  x_lastout;
    MYFLT  x_phase;
    MYFLT  x_freq;
    MYFLT  x_yn;
    MYFLT  x_xn;
} STANDARDCHAOS;


static int32_t standardchaos_init(CSOUND *csound, STANDARDCHAOS *p) {
    p->x_sr = csound->GetSr(csound);
    MYFLT hz = p->x_sr * 0.5; // default parameters
    p->x_phase = hz >= 0 ? 1 : 0;
    p->x_freq  = hz;
    p->x_lastout = 0;
    p->x_xn = *p->x;
    p->x_yn = *p->y;
    return OK;
}

static int32_t standardchaos_init_x(CSOUND *csound, STANDARDCHAOS *p) {
    *p->x = 0.5;
    *p->y = 0;
    return standardchaos_init(csound, p);
}

static int32_t standardchaos_perf(CSOUND *csound, STANDARDCHAOS *p) {
    IGN(csound);

    MYFLT *out = p->out;

    SAMPLE_ACCURATE(out);

    MYFLT hz = *p->rate;
    MYFLT yn = p->x_yn;
    MYFLT xn = p->x_xn;
    MYFLT k = *p->k;
    MYFLT lastout = p->x_lastout;
    MYFLT phase = p->x_phase;
    MYFLT sr = p->x_sr;
    for(n=offset; n<nsmps; n++) {
        MYFLT phase_step = hz / sr; // phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step; // clipped phase_step
        int trig;
        MYFLT output;
        if (hz >= 0) {
            trig = phase >= 1.;
            if (phase >= 1.) phase = phase - 1;
        } else {
            trig = (phase <= 0.);
            if (phase <= 0.) phase = phase + 1.;
        }
        if (trig) {
            yn = fmod(yn + k * sin(xn), TWOPI);
            xn = fmod(xn + yn, TWOPI);
            if (xn < 0) xn = xn + TWOPI;
            output = lastout = (xn - PI) * ONE_OVER_PI;
        }
        else output = lastout; // last output
        out[n] = output;
        phase += phase_step;
    }
    p->x_phase = phase;
    p->x_lastout = lastout;
    p->x_yn = yn;
    p->x_xn = xn;
    return OK;
}

// aout rampgate kgate, isustpoint, kval0, kdel0, kval1, kdel1, ..., kvaln
// kout rampgate kgate, isustpoint, kval0, kdel0, kval1, kdel1, ..., kvaln

// kvalx... value in seconds


#define MAXPOINTS 128

enum RampgateState { Off, Attack, Sustain, Release, Retrigger };


typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *gate;
    MYFLT *sustpoint;
    MYFLT *points[MAXPOINTS];

    MYFLT sr;
    int lastgate;
    enum RampgateState state;
    uint32_t numpoints;

    MYFLT val;
    MYFLT t;
    MYFLT segment_end;
    MYFLT deltat;
    MYFLT prev_val;
    MYFLT next_val;
    MYFLT retrigger_ramptime;
    int32_t numsegments;
    int32_t segment_idx;
    int32_t sustain_idx;

} RAMPGATE;

static int32_t rampgate_k_init_common(CSOUND *csound, RAMPGATE *p, MYFLT sr) {
    p->sr = sr;
    p->state = Off;  // not playing
    p->numpoints = p->INOCOUNT - 2;  // this should be uneven
    if(p->numpoints % 2 == 0) {
        INITERRF(Str("Number of points should be uneven, got %d"), p->numpoints);
    }
    p->numsegments = (p->numpoints - 1) / 2;
    // printf("numpoints: %d,  numsegments: %d\n", p->numpoints, p->numsegments);
    p->lastgate = 0;
    p->t = 0;
    p->deltat = 1/p->sr;
    p->val = *(p->points[0]);
    p->segment_end = *(p->points[1]);
    p->prev_val = p->val;
    p->next_val = *(p->points[2]);
    p->segment_idx = 0;
    p->retrigger_ramptime = 0.020;  // this could be configurable
    p->sustain_idx = (int32_t)*p->sustpoint;

    if(p->sustain_idx >= 0 && p->sustain_idx >= p->numsegments)
        return INITERRF("Sustain point is %d, but there are only %d defined segments",
                        p->sustain_idx, p->numsegments);
    return OK;
}


static int32_t rampgate_k_init(CSOUND *csound, RAMPGATE *p) {
    return rampgate_k_init_common(csound, p, csound->GetKr(csound));
}


static inline void rampgate_update_segment(RAMPGATE *p, int32_t idx) {
    int32_t idx0 = idx*2;
    // we assume that p->t has been updated already
    p->segment_end = *(p->points[idx0+1]);
    p->prev_val = *(p->points[idx0]);
    p->next_val = *(p->points[idx0+2]);
    p->segment_idx = idx;
}


static int32_t rampgate_k_k(CSOUND *csound, RAMPGATE *p) {
    int gate = (int)*p->gate;
    int lastgate = p->lastgate;
    MYFLT val;
    if(gate != lastgate) {
        if(gate == 1) {
            // are we playing?
            if(p->state == Off) {
                // not playing, just waiting for a gate, so start attack
                p->state = Attack;
                p->t = 0;
                rampgate_update_segment(p, 0);
            } else if(p->state == Release) {
                // still playing release section, enter ramp to beginning state
                p->t = 0;
                p->state = Retrigger;
                p->prev_val = p->val;
                p->next_val = *p->points[0];
                p->segment_end = p->retrigger_ramptime;
            } else {
                return PERFERRF("This should not happen. state = %d", p->state);
            }
        } else {
            if(p->state != Off)
                p->state = Release;
        }
    }
    p->lastgate = gate;
    if(p->state == Off || p->state == Sustain) {
        // either not playing or sustaining, just output our current value
        *p->out = p->val;
        return OK;
    }

    val = (p->next_val - p->prev_val) * (p->t / p->segment_end) + p->prev_val;
    *p->out = p->val = val;
    p->t += p->deltat;

    if(p->t < p->segment_end)
        return OK;

    // we are finished with this segment
    if (p->state == Retrigger) {
        // finished retrigger state
        p->t -= p->segment_end;
        rampgate_update_segment(p, 0);
    } else if(p->segment_idx >= p->numsegments - 1) {
        // end of envelope
        p->t = 0;
        p->val = p->next_val;
        rampgate_update_segment(p, 0);
        p->state = Off;
        return OK;
    } else {
        // new segment
        p->t -= p->segment_end;
        rampgate_update_segment(p, p->segment_idx+1);
        if(p->segment_idx == p->sustain_idx && p->state == Attack) {
            // sustaining section
            p->state = Sustain;
            p->val = p->prev_val;
        }
    }
    return OK;
}


static int32_t rampgate_a_init(CSOUND *csound, RAMPGATE *p) {
    return rampgate_k_init_common(csound, p, csound->GetSr(csound));
}


static int32_t rampgate_a_k(CSOUND *csound, RAMPGATE *p) {
    MYFLT *out = p->out;

    SAMPLE_ACCURATE(out);

    int gate = (int)*p->gate;
    int lastgate = p->lastgate;
    MYFLT **points = p->points;

    if(gate != lastgate) {
        if(gate == 1) {  // gate is opening
            if(p->state == Off) {
                // not playing, just waiting for a gate, so start attack
                p->t = 0;
                p->prev_val = *points[0];
                p->segment_end = *points[1];
                p->next_val = *points[2];
                p->segment_idx = 0;
                p->state = p->sustain_idx == 0 ? Sustain : Attack;
                p->val = p->prev_val;
            } else if(p->state == Release) {
                // still playing release section, enter ramp to beginning state
                p->t = 0;
                p->state = Retrigger;
                p->prev_val = p->val;
                p->next_val = *p->points[0];
                p->segment_end = p->retrigger_ramptime;
                p->segment_idx = -1;
            } else
                return PERFERRF("This should not happen. state = %d", p->state);
        } else {  // gate is closing
            if(p->state == Off) {
                p->t = 0;
                rampgate_update_segment(p, 0);
            } else
                p->state = Release;
        }
    }
    p->lastgate = gate;
    MYFLT t = p->t;
    enum RampgateState state = p->state;
    MYFLT val = p->val;

    // if state is Off or Sustain, state can only change with a gate, so will not change
    // during this period
    if(state == Off || state == Sustain) {
        for(n=offset; n<nsmps; n++)
            out[n] = val;
        return OK;
    }

    MYFLT next_val = p->next_val;
    MYFLT prev_val = p->prev_val;
    MYFLT segment_end = p->segment_end;
    MYFLT deltat = p->deltat;
    int32_t segment_idx = p->segment_idx;
    int32_t last_segment_idx = p->numsegments - 1;
    uint32_t m;

    for(n=offset; n<nsmps; n++) {
        val = (next_val - prev_val) * (t / segment_end) + prev_val;
        out[n] = val;
        t += deltat;
        if(t < p->segment_end)
            continue;

        // we are finished with this segment
        if (state == Retrigger) {
            // finished retrigger state
            if(p->sustain_idx == 0) {
                // landed in Sustain section
                val = *points[0];
                for(m=n; m<nsmps; m++)
                    out[m] = val;
                break;
            }
            state = Attack;
            t -= segment_end;
            segment_end = *points[1];
            prev_val = val;
            next_val = *points[2];
            segment_idx = 0;
        } else if (segment_idx < last_segment_idx) {
            // new segment
            t -= segment_end;
            prev_val = next_val;
            segment_idx += 1;
            next_val = *points[segment_idx*2+2];
            segment_end = *points[segment_idx*2+1];

            if(segment_idx == p->sustain_idx && state == Attack) {
                // sustaining section
                state = Sustain;
                val = prev_val;
            }
        } else {
            // end of envelope
            state = Off;
            t = 0;
            val = next_val;
            for(m=n; m<nsmps; m++) {
                out[m] = val;
            }
            break;
        }
    }
    p->state = state;
    p->t = t;
    p->val = val;
    p->prev_val = prev_val;
    p->next_val = next_val;
    p->segment_end = segment_end;
    p->segment_idx = segment_idx;
    return OK;
}

#define S(x) sizeof(x)

static OENTRY localops[] = {
	{ "crackle", S(CRACKLE), 0, 3, "a", "P", (SUBR)crackle_init, (SUBR)crackle_perf },
    { "ramptrig.k_kk", S(RAMPTRIGK), 0, 3, "k", "kkP", (SUBR)ramptrig_k_kk_init, (SUBR)ramptrig_k_kk },
    { "ramptrig.a_kk", S(RAMPTRIGK), 0, 3, "a", "kkP", (SUBR)ramptrig_a_kk_init, (SUBR)ramptrig_a_kk },
    { "ramptrig.sync_kk_kk", S(RAMPTRIGSYNC), 0, 3, "kk", "kkPO", (SUBR)ramptrigsync_kk_kk_init, (SUBR)ramptrigsync_kk_kk},
    { "sigmdrive.a_ak",S(SIGMDRIVE), 0, 2, "a", "akO", NULL, (SUBR)sigmdrive_a_ak},
    { "sigmdrive.a_aa",S(SIGMDRIVE), 0, 2, "a", "aaO", NULL, (SUBR)sigmdrive_a_aa},
    { "lfnoise", S(LFNOISE), 0, 3, "a", "kO", (SUBR)lfnoise_init, (SUBR)lfnoise_perf},
    { "schmitt.k", S(SCHMITT), 0, 3, "k", "kkk", (SUBR)schmitt_k_init, (SUBR)schmitt_k_perf},
    { "schmitt.a", S(SCHMITT), 0, 3, "a", "akk", (SUBR)schmitt_k_init, (SUBR)schmitt_a_perf},
    { "standardchaos", S(STANDARDCHAOS), 0, 3, "a", "kkio", (SUBR)standardchaos_init, (SUBR)standardchaos_perf},
    { "standardchaos", S(STANDARDCHAOS), 0, 3, "a", "kP", (SUBR)standardchaos_init_x, (SUBR)standardchaos_perf},
    { "rampgate.k_k", S(RAMPGATE), 0, 3, "k", "kiM", (SUBR)rampgate_k_init, (SUBR)rampgate_k_k},
    { "rampgate.a_k", S(RAMPGATE), 0, 3, "a", "kiM", (SUBR)rampgate_a_init, (SUBR)rampgate_a_k}

};

LINKAGE
