/*
  else.c

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

  # ramptrig

  kout ramptrig ktrig, idur, [ivaloff]

  A ramp from 0 to 1 in idur seconds. Can be retriggered. Concieved to be used together
  with bpf to generate trigerrable complex envelopes

  idur = 2
  kramp ramptrig ktrig, idur
  aenv bpf kramp*idur, 0, 0, 0.3, 1, 1.7, 1, 2, 0

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

  aout standardchaos krate, kk=1, ix=0.5, iy=0

  krate: from 0 to nyquist
  kk: a value for k in the above equation
  ix: initial value for x
  iy: initial value for y

  # linenv

  an envelope generator similar to linsegr but with retriggerable gate and
  flexible sustain point

  aout/kout rampgate kgate, isustidx, kval0, kdel0, kval1, kdel1, ..., kvaln

  kgate: when kgate changes from 0 to 1, value follows envelope until isustpoint is reached
  value stays there until kgate switches to 0, after which it follow the rest
  of the envelope and stays at the env until a new switch. If kgate switches to 0
  before reaching isustpoint, then it uses the current value as sustain point and
  follows the envelope from there. If kgate switches to 1 before reaching the
  end of the release part, it glides to the beginning and starts attack envelope
  from there
  isustidx: the idx of the sustaining value. if isustidx is 2, then the value stays
  at kval2 until kgate is closed (0).
  * Use 0 to disable sustain, negative indexes count from end (-1 is the last segment)
  NB: the first segment (index 0 ) can't be used as sustain segment. In order to have
  a sustain segment just at the beginning, use a very short first segment and set
  the sustain index to 1

  kdel0, kval0, ...: the points of the envelope, similar to linseg

  If noteoff release is needed, xtratim and release opcodes are needed.

  inspired by else's envgen

  Example: generate an adsr envelope with attack=0.05, decay=0.1, sust=0.2, rel=0.5

  isustidx = 2
  aout linenv kgate, isustidx, 0, 0.05, 1, 0.1, 0.2, 0.5, 0

  Example 2: emulate ramptrig

  These are the same:
  kout ramptrig ktrig, idur
  kout rampgate ktrig, -1, 0, idur, 1

  # sp_peaklim

  aout sp_peaklim ain, ktresh=0, iattack=0.01, irelease=0.1

  # diode_ringmod

  A ring modulator with some dirtyness. Two versions: one where an inbuilt sinus as
  modulator signal; and a second where the user passed its own signal as modulator. In the
  second case the effect of nonlinearities is reduced to feedback (in the first, the freq.
  of the oscillator is also modified)

  (mod -> diode sim -> feedback) * carrier

  aout dioderingmod acarr, kmodfreq, kdiode=0, kfeedback=0,
  knonlinearities=0.1, koversample=0

  A port of jsfx Loser's ringmodulator

  # atstop

  schedule an instrument when the note stops

  NB: this is scheduled not at release start, but when the
  note is deallocated

  atstop Sintr, idelay, idur, pfields...
  atstop instrnum, idelay, idur, pfields...

*/

#include "csdl.h"
#include "arrays.h"
#include <math.h>

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif

#include <ctype.h>

// #include <fluidsynth.h>


#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

#define MSGF(fmt, ...) (csound->Message(csound, fmt, __VA_ARGS__))
#define MSG(s) (csound->Message(csound, s))
#define INITERR(m) (csound->InitError(csound, "%s", m))
#define INITERRF(fmt, ...) (csound->InitError(csound, fmt, __VA_ARGS__))
#define PERFERR(m) (csound->PerfError(csound, &(p->h), "%s", m))
#define PERFERRF(fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))

#define Uint32_tMAX 0x7FFFFFFF

#define ONE_OVER_PI 0.3183098861837907

// #define DEBUG

#ifdef DEBUG
#define DBG(fmt, ...) printf(">>>> "fmt"\n", __VA_ARGS__); fflush(stdout);
#define DBG_(m) DBG("%s", m)
#else
#define DBG(fmt, ...)
#define DBG_(m)
#endif


#define SAMPLE_ACCURATE(out)                                        \
    uint32_t n, nsmps = CS_KSMPS;                                   \
    uint32_t offset = p->h.insdshead->ksmps_offset;                 \
    uint32_t early = p->h.insdshead->ksmps_no_end;                  \
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));  \
    if (UNLIKELY(early)) {                                          \
        nsmps -= early;                                             \
        memset(&out[nsmps], '\0', early*sizeof(MYFLT));             \
    }                                                               \

// needed for each opcode using audio-rate inputs/outputs
#define AUDIO_OPCODE(csound, p) \
    IGN(csound); \
    uint32_t n, nsmps = CS_KSMPS;                                    \
    uint32_t offset = p->h.insdshead->ksmps_offset;                  \
    uint32_t early = p->h.insdshead->ksmps_no_end;                   \

// initialize an audio output variable, for sample-accurate offset/early end
// this should be called for each audio output
#define AUDIO_OUTPUT(out) \
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));   \
    if (UNLIKELY(early)) {                                           \
        nsmps -= early;                                              \
        memset(&out[nsmps], '\0', early*sizeof(MYFLT));              \
    }                                                                \


#define register_deinit(csound, p, func)                                \
    csound->RegisterDeinitCallback(csound, p, (int32_t(*)(CSOUND*, void*))(func))


#ifdef USE_DOUBLE
// from https://www.gamedev.net/forums/topic/621589-extremely-fast-sin-approximation/
static inline double fast_sin(double x) {
    int k;
    double y, z;
    z  = x;
    z *= 0.3183098861837907;
    z += 6755399441055744.0;
    k  = *((int *) &z);
    z  = k;
    z *= 3.1415926535897932;
    x -= z;
    y  = x;
    y *= x;
    z  = 0.0073524681968701;
    z *= y;
    z -= 0.1652891139701474;
    z *= y;
    z += 0.9996919862959676;
    x *= z;
    k &= 1;
    k += k;
    z  = k;
    z *= x;
    x -= z;
    return x;
}
#else
float fast_sin(float floatx) {
    return (float)fast_sin((double)floatx);
}
#endif

// ------------------ pool --------------------

// this is a data type to allocate slots for handles
typedef struct {
    int size;
    int capacity;
    int cangrow;
    int *data;
} intarray_t;

static intarray_t*
intpool_init(CSOUND *csound, intarray_t *pool, int size) {
    pool->size = size;
    pool->capacity = size;
    pool->cangrow = 1;
    pool->data = csound->Malloc(csound, sizeof(int) * size);
    for(int i=0; i<size; i++) {
        pool->data[i] = i;
    }
    return pool;
}

void intpool_deinit(CSOUND *csound, intarray_t *pool) {
    if(pool->data != NULL && pool->capacity > 0)
        csound->Free(csound, pool->data);
    pool->data = NULL;
    pool->capacity = 0;
    pool->size = 0;
    pool->cangrow = 0;
}

static void
intarray_resize(CSOUND *csound, intarray_t *arr, int capacity) {
    arr->data = csound->ReAlloc(csound, arr->data, sizeof(int)*capacity);
    arr->capacity = capacity;
    if(arr->capacity < arr->size) {
        arr->size = arr->capacity;
    }
}

static int
intpool_push(CSOUND *csound, intarray_t *pool, int val) {
    if(pool->size >= pool->capacity) {
        return INITERR("Pool full, can't push this element");
    }
    pool->data[pool->size] = val;
    pool->size += 1;
    return OK;
}

static int
intpool_pop(CSOUND *csound, intarray_t *pool) {
    if(pool->size == 0) {
        if(!pool->cangrow) {
            return INITERR("This pool is empty and can't grow");
        }
        // pool is empty. we refill it with new values outside of the current range
        int old_capacity = pool->capacity;
        intarray_resize(csound, pool, pool->capacity * 2);
        for(int i=0; i<old_capacity; i++) {
            pool->data[i] = i+old_capacity;
        }
        pool->size = old_capacity;
        // zero the new memory (is this needed?)
        // memset(&(pool->data[old_capacity]), 0, old_capacity*2*sizeof(int));
    }
    int val = pool->data[pool->size - 1];
    pool->size--;
    return val;
}

// ----------------- Crackle ------------------

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
                out[n] = 1.0 - POWER(1. - x, drivefactor);
            else
                out[n] = POWER(1.0 + x, drivefactor) - 1.0;
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
                out[n] = 1.0 - POWER(1. - x, drivefactor);
            else
                out[n] = POWER(1.0 + x, drivefactor) - 1.0;
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
    IGN(csound);
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

MYFLT mfmod(MYFLT x, MYFLT y) {
    // double mfmod(double x,double y) { double a; return ((a=x/y)-(int)a)*y; }
    MYFLT a = x/y;
    return a - (int)a * y;
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
            yn = mfmod(yn + k * fast_sin(xn), TWOPI);
            xn = mfmod(xn + yn, TWOPI);
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


#define MAXPOINTS 1900

enum RampgateState { Off, Attack, Sustain, Release, Retrigger };

char* _rampgateStateNames[] = {"Off", "Attack", "Sustain", "Release", "Retrigger"};


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
    MYFLT factor;
    int32_t numsegments;
    int32_t segment_idx;
    int32_t sustain_idx;

} RAMPGATE;


static int32_t rampgate_k_init_common(CSOUND *csound, RAMPGATE *p, MYFLT sr) {
    p->sr = sr;
    p->state = Off;  // not playing
    p->numpoints = p->INOCOUNT - 2;  // this should be odd
    if(p->numpoints % 2 == 0) {
        INITERRF(Str("Number of points should be odd (got %d points)"), p->numpoints);
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
    int sustain_idx = (int32_t)*p->sustpoint;
    if(sustain_idx < 0) {
        sustain_idx += p->numsegments;
    }
    p->sustain_idx = sustain_idx;
    if(p->sustain_idx != 0 && (p->sustain_idx < 0 || p->sustain_idx >= p->numsegments)) {
        return INITERRF("Sustain point (%d) out of range. There are %d defined segments",
                        p->sustain_idx, p->numsegments);
    }
    if(*p->points[p->numpoints - 2] == 0.) {
        return INITERR("The last point cannot have 0 duration");
    }
    return OK;
}


static int32_t linenv_k_init(CSOUND *csound, RAMPGATE *p) {
    return rampgate_k_init_common(csound, p, csound->GetKr(csound));
}


static inline void rampgate_update_segment(RAMPGATE *p, int32_t idx) {
    int32_t idx0 = idx*2;
    // we assume that p->t has been updated already
    MYFLT segment_dur = *(p->points[idx0+1]);
    if(segment_dur == 0) {
        idx += 1;
        idx0 += 2;
        segment_dur = *(p->points[idx0+1]);
    }
    p->segment_end = segment_dur;
    p->prev_val = *(p->points[idx0]);
    p->next_val = *(p->points[idx0+2]);
    p->segment_idx = idx;
}


static int32_t linenv_k_k(CSOUND *csound, RAMPGATE *p) {
    int gate = (int)*p->gate > 0 ? 1 : 0;
    int lastgate = p->lastgate;
    MYFLT val;
    if(gate != lastgate) {
        int oldstate = p->state;
        if(gate > 0) {
            // are we playing?
            if(p->state == Off) {
                // not playing yet: start attack
                p->t = 0;
                p->state = Attack;
                rampgate_update_segment(p, 0);
                p->val = p->prev_val;
            } else if(p->state == Release) {
                // still playing release section, enter ramp to beginning state
                p->t = 0;
                p->state = Retrigger;
                p->prev_val = p->val;
                p->next_val = *p->points[0];
                p->segment_end = p->retrigger_ramptime;
                p->segment_idx = -1;
            } else {
                return PERFERRF("This should not happen, state= %d (%s), kgate=%.1f, lastgate=%d", p->state, _rampgateStateNames[p->state], *p->gate, lastgate);
            }
            // printf("Gate opened, from %s to %s\n", _rampgateStateNames[oldstate], _rampgateStateNames[p->state]);

        } else {
            // transition 1 -> 0
            switch(p->state) {
            case Off:
                p->t = 0;
                rampgate_update_segment(p, 0);
                break;
            case Sustain:
                p->state = Release;
                break;
            case Attack:
            case Retrigger:
                p->state = Release;
                if(p->sustain_idx == 0) {
                    // No sustain segment
                    break;
                }
                p->t = 0;
                rampgate_update_segment(p, p->sustain_idx);
                p->prev_val = p->val;
                break;
            case Release:
                return PERFERRF("This should not happend, gate closed, state = %d (%s), last state: %s", p->state, _rampgateStateNames[p->state], _rampgateStateNames[oldstate]);
            }
            // printf("Gate closed, from %s to %s\n", _rampgateStateNames[oldstate], _rampgateStateNames[p->state]);
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
        p->val = *(p->points[0]);
        // p->state = p->sustain_idx == 0 ? Sustain : Attack;
        p->state = Attack;
    } else if(p->segment_idx >= p->numsegments - 1) {
        // end of envelope
        p->state = Off;
        p->t = 0;
        p->val = p->next_val;
        // rampgate_update_segment(p, 0);
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


static int32_t linenv_a_init(CSOUND *csound, RAMPGATE *p) {
    return rampgate_k_init_common(csound, p, csound->GetSr(csound));
}


static int32_t linenv_a_k(CSOUND *csound, RAMPGATE *p) {
    MYFLT *out = p->out;

    SAMPLE_ACCURATE(out);

    int gate = (int)*p->gate > 0 ? 1 : 0;
    int lastgate = p->lastgate;
    MYFLT **points = p->points;

    if(gate != lastgate) {
        int oldstate = p->state;
        if(gate > 0) {  // gate is opening
            if(p->state == Off) {
                // not playing, just waiting for a gate, so start attack
                p->t = 0;
                p->prev_val = *points[0];
                p->segment_end = *points[1];
                p->next_val = *points[2];
                p->segment_idx = 0;
                // p->state = p->sustain_idx == 0 ? Sustain : Attack;
                p->state = Attack;
                p->val = p->prev_val;
            } else if(p->state == Release) {
                // still playing release section, enter ramp to beginning state
                p->t = 0;
                p->state = Retrigger;
                p->prev_val = p->val;
                p->next_val = *p->points[0];
                p->segment_end = p->retrigger_ramptime;
                p->segment_idx = -1;
            } else {
                return PERFERRF("This should not happen, state= %d (%s), kgate=%.1f, lastgate=%d", p->state, _rampgateStateNames[p->state], *p->gate, lastgate);
            }
        } else {  // gate is closing
            // transition 1 -> 0
            switch(p->state) {
            case Off:
                p->t = 0;
                rampgate_update_segment(p, 0);
                break;
            case Sustain:
                p->state = Release;
                break;
            case Attack:
            case Retrigger:
                p->state = Release;
                if(p->sustain_idx == 0)
                    break;
                p->t = 0;
                rampgate_update_segment(p, p->sustain_idx);
                p->prev_val = p->val;
                break;
            case Release:
                return PERFERRF("This should not happend, gate closed, state = %d (%s), last state: %s", p->state, _rampgateStateNames[p->state], _rampgateStateNames[oldstate]);
            }
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
    MYFLT factor = (next_val - prev_val) / segment_end;
    for(n=offset; n<nsmps; n++) {
        // TODO: optimize this to extract a factor
        // factor = (next_val - prev_val) / segment_end
        // val = prev_val + t * factor
        // this factor must be calculated only when a segment is changed
        // val = (next_val - prev_val) * (t / segment_end) + prev_val;
        out[n] = val;
        val = prev_val + t*factor;
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
            while(1) {
                prev_val = next_val;
                segment_idx += 1;
                next_val = *points[segment_idx*2+2];
                segment_end = *points[segment_idx*2+1];
                if(segment_end > 0) {
                    break;
                }
            }

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


typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *in;
    MYFLT *threshdb;
    MYFLT *atk;
    MYFLT *rel;

    MYFLT patk, prel;
    MYFLT b0_r, a1_r, b0_a, a1_a, level;

} SP_PEAKLIM;

#ifndef dB
/* if below -100dB, set to -100dB to prevent taking log of zero */
#define dB(x) 20.0 * ((x) > 0.00001 ? log10(x) : log10(0.00001))
#endif

#ifndef dB2lin
#define dB2lin(x)    POWER( 10.0, (x) / 20.0 )
#endif

static int32_t sp_peaklim_init(CSOUND *csound, SP_PEAKLIM *p) {
    IGN(csound);
    p->a1_r = 0;
    p->b0_r = 1;
    p->a1_a = 0;
    p->b0_a = 1;
    p->patk = -100;
    p->prel = -100;
    p->level = 0;
    if(*p->atk < 0)
        *p->atk = 0.01;
    if(*p->rel < 0)
        *p->rel = 0.1;
    return OK;
}

static int32_t sp_peaklim_compute(CSOUND *csound, SP_PEAKLIM *p) {

    MYFLT *out = p->out;
    SAMPLE_ACCURATE(out);
    MYFLT *in = p->in;

    MYFLT gain = 0;

    MYFLT atk = *p->atk;
    MYFLT rel = *p->rel;

    MYFLT patk = p->patk;
    MYFLT prel = p->prel;
    MYFLT a1_a = p->a1_a;
    MYFLT b0_r = p->b0_r;
    MYFLT a1_r = p->a1_r;
    MYFLT b0_a = p->b0_a;
    MYFLT sr = csound->GetSr(csound);
    MYFLT level = p->level;
    MYFLT insamp;
    MYFLT threshlin = dB2lin(*p->threshdb);

    if(patk != atk) {
        patk = atk;
        a1_a = exp( -1.0 / ( atk * sr ) );
        b0_a = 1 - a1_a;
    }

    if(prel != rel) {
        prel = rel;
        a1_r = exp( -1.0 / ( rel * sr ) );
        b0_r = 1 - a1_r;
    }

    /* change coefficients, if needed */
    n = nsmps - offset;

    while (n--) {
        insamp = *in++;
        MYFLT absinsamp = fabs(insamp);
        if ( absinsamp > level)
            level += b0_a * ( absinsamp - level);
        else
            level += b0_r * ( absinsamp - level);

        gain = min(1.0, threshlin/level);
        *out++ = insamp * gain;
    }
    p->patk = patk;
    p->prel = prel;
    p->a1_a = a1_a;
    p->b0_r = b0_r;
    p->a1_r = a1_r;
    p->b0_a = b0_a;
    p->level = level;
    return OK;
}

// check if a file exists
// taken from https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c


typedef struct {
    OPDS h;
    MYFLT *out;
    STRINGDAT *path;
} FILE_EXISTS;

static int32_t file_exists_init(CSOUND *csound, FILE_EXISTS *p) {
    IGN(csound);
#ifdef _WIN32
    return INITERR("This opcode is not supported in windows");
#else
    if(access(p->path->data, F_OK) != -1 ) {
        // file exists
        *p->out = 1;
    } else {
        *p->out = 0;
    }
#endif
    return OK;
}

// #define USE_LOOKUP_SINE 1


typedef struct {
    OPDS h;
    MYFLT *aout;
    MYFLT *ain;
    MYFLT *kmodfreq;
    MYFLT *kdiodeon;
    MYFLT *kfeedback;
    MYFLT *knonlinearities;
    MYFLT *koversample;

    int started;
    MYFLT r, c1, c2, c3, fgain, bl_c1, bl_c2, bl_c3;
    MYFLT src_f;
    MYFLT sinp, ps_sin_out2;
    MYFLT sin_bl2_1, sin_bl2_2, sin_bl1_1, sin_bl1_2, o_sin_out2;
    MYFLT s1, s2;
    MYFLT ps_fx_out2l;
    MYFLT bl2_1, bl2_2, bl1_1, bl1_2;
    MYFLT o_fx_out2l;
    MYFLT s2l, s1l;
    MYFLT fp_l_os_1, fp_l_os_2, fp_l;
    uint32_t seed;

#ifdef USE_LOOKUP_SINE
    FUNC * ftp;
    MYFLT cpstoinc;
    int32_t  phase;
    int32_t  lomask;
#endif
} t_diode_ringmod;


static int32_t dioderingmod_init(CSOUND *csound, t_diode_ringmod *p) {
    //sim resistance
    p->r = 0.85;

    //fir restoration filter
    p->c1 = 1;
    p->c2 = -0.75;
    p->c3 = 0.17;
    p->fgain = 4;

    //fir bandlimit
    p->bl_c1 = 0.52;
    p->bl_c2 = 0.54;
    p->bl_c3 = -0.02;
    p->started = 0;
    p->seed = csound->GetRandomSeedFromTime();
    p->sinp = 0;
#ifdef USE_LOOKUP_SINE
    MYFLT ifn = -1;
    p->ftp = csound->FTFind(csound, &ifn);
    uint32_t tabsize = p->ftp->flen;
    MYFLT sampledur = 1 / csound->GetSr(csound);
    p->cpstoinc = tabsize * sampledur * 65536;
    p->lomask   = (tabsize - 1) << 3;
    p->phase = 0;
#endif
    return OK;
}


// uniform noise, taken from csoundRand31, returns floats between 0-1
static inline MYFLT randflt(uint32_t *seedptr) {
    uint64_t tmp1;
    uint32_t tmp2;
    /* x = (742938285 * x) % 0x7FFFFFFF */
    tmp1  = (uint64_t) ((int32_t) (*seedptr) * (int64_t) 742938285);
    tmp2  = (uint32_t) tmp1 & (uint32_t) 0x7FFFFFFF;
    tmp2 += (uint32_t) (tmp1 >> 31);
    tmp2  = (tmp2 & (uint32_t) 0x7FFFFFFF) + (tmp2 >> 31);
    (*seedptr) = tmp2;
    return (MYFLT)(tmp2 - 1) / FL(2147483648.0);
}


static inline float
PhaseFrac(uint32_t inPhase) {
    union { uint32_t itemp; float ftemp; } u;
    u.itemp = 0x3F800000 | (0x007FFF80 & ((inPhase)<<7));
    return u.ftemp - 1.f;
}

#define M_PI 3.14159265358979323846
#define pi2 2*M_PI

#define xlobits 14
#define xlobits1 13


static inline MYFLT
lookupi1(const MYFLT* table0, const MYFLT* table1,
         int32_t pphase, int32_t lomask) {
    MYFLT pfrac    = PhaseFrac(pphase);
    uint32_t index = ((pphase >> xlobits1) & lomask);
    MYFLT val1 = *(const MYFLT*)((const char*)table0 + index);
    MYFLT val2 = *(const MYFLT*)((const char*)table1 + index);
    MYFLT out  = val1 + (val2 - val1) * pfrac;
    return out;
}


static int32_t dioderingmod_perf(CSOUND *csound, t_diode_ringmod *p) {
    int os = (int)*p->koversample;
    MYFLT tgt_f = *p->kmodfreq;

    // sx = 16+slider3*1.20103;
    // tgt_f = floor(exp(sx*log(1.059))*8.17742);

    MYFLT fb = *p->kfeedback;
    int diode = (int)*p->kdiodeon;
    MYFLT nl = *p->knonlinearities * 100;
    MYFLT outgain = 1;

    MYFLT *ain = p->ain;
    MYFLT *aout = p->aout;

    if(! p->started) {
        p->src_f = tgt_f;
        p->started = 1;
    }

    MYFLT samplesblock = p->h.insdshead->ksmps;
    MYFLT srate = csound->GetSr(csound);

    //interpolate 'f'
    MYFLT d_f = (tgt_f - p->src_f) / samplesblock;
    MYFLT tf = p->src_f;
    p->src_f = tgt_f;

    int nsmps = p->h.insdshead->ksmps;
    uint32_t seed = p->seed;
    MYFLT pi2_over_srate = pi2 / srate;
    MYFLT sinp = p->sinp;

#ifdef USE_LOOPUP_SINE
    MYFLT *table0 = p->ftp->ftable;
    MYFLT *table1 = table0 + 1;
    int32_t intphase = p->phase;
    MYFLT cpstoinc = p->cpstoinc;
#endif
    MYFLT nl_fb = 0;
    MYFLT nl_f = 0;
    for(int i=0; i < nsmps; i++) {
        nl_f = nl == 0 ? 0 : randflt(&seed) * 4*nl - 2*nl;
        nl_fb = ((fb == 0)|(nl == 0)) ? 0 : (randflt(&seed)*2*nl - nl)*0.001;
        //interpolate 'f'
        tf += d_f;

        // mod signal gen
#ifdef USE_LOOPUP_SINE
        int32_t phaseinc = (int32_t)(cpstoinc * (tf - nl_f));
        MYFLT sinout = lookupi1(table0, table1, intphase, p->lomask);
        intphase += phaseinc;
#else
        MYFLT sina = (tf - nl_f) * pi2_over_srate;
        // MYFLT sinout = sin(sinp);
        MYFLT sinout = fast_sin(sinp);
        // MYFLT sinout = sinf(sinp);
        sinp += sina;
        if(sinp >= pi2)
            sinp -= pi2;
#endif
        //diode - positive semi-periods
        MYFLT m_out;
        if (diode == 0) {
            m_out = sinout;
        } else {
            //os - diode-ed signal?
            MYFLT d_sin;
            if(os == 0) {
              d_sin = fabs(sinout)*2-0.20260;
            } else {
                //power series in
                MYFLT ps_sin_out1 = 0.5*(sinout + p->ps_sin_out2);
                p->ps_sin_out2 = 0.5 * ps_sin_out1;
                //abs()
                MYFLT ps_d_sin1 = fabs(ps_sin_out1)*2-0.20260;
                MYFLT ps_d_sin2 = fabs(p->ps_sin_out2)*2-0.20260;
                //bandlimit
                MYFLT sin_bl3_1 = p->sin_bl2_1;
                MYFLT sin_bl3_2 = p->sin_bl2_2;
                p->sin_bl2_1 = p->sin_bl1_1;
                p->sin_bl2_2 = p->sin_bl1_2;
                p->sin_bl1_1 = ps_d_sin1;
                p->sin_bl1_2 = ps_d_sin2;
                MYFLT sin_bl_out1 = (p->sin_bl1_1*p->bl_c1 +
                                     p->sin_bl2_1*p->bl_c2 +
                                     sin_bl3_1*p->bl_c3);
                MYFLT sin_bl_out2 = (p->sin_bl1_2*p->bl_c1 +
                                     p->sin_bl2_2*p->bl_c2 +
                                     sin_bl3_2*p->bl_c3);
                //power series out
                MYFLT o_sin_out1 = 0.5*(sin_bl_out1+p->o_sin_out2);
                p->o_sin_out2 = 0.5*(sin_bl_out2+o_sin_out1);
                //fir restoration
                MYFLT s3 = p->s2;
                p->s2 = p->s1;
                p->s1 = o_sin_out1;
                d_sin = (p->s1*p->c1+p->s2*p->c2+s3*p->c3) * p->fgain;
            }
            m_out = d_sin;
        }

        //-------------------------------------------------
        //input
        //-------------------------------------------------
        MYFLT in_l = ain[i];
        MYFLT fx_outl;
        if(os == 0) {   // No oversampling
            //feedback ala Paul Kellet
            p->fp_l = (in_l+(fb-nl_fb)*p->fp_l)*sinout*p->r;
            //multiply carrier with mod
            MYFLT s_out_l = m_out*in_l;
            //apply feedback
            s_out_l += p->fp_l;

            fx_outl = s_out_l;


        } else {      // Yes oversampling
            //power series in
            MYFLT ps_fx_out1l = 0.5*(in_l+p->ps_fx_out2l);
            p->ps_fx_out2l = 0.5*ps_fx_out1l;

            //------------------------
            //fx
            //------------------------
            p->fp_l_os_1 = (ps_fx_out1l+(fb-nl_fb)*p->fp_l_os_1)*sinout*p->r;
            MYFLT s_out_l_os_1 = m_out*ps_fx_out1l;
            s_out_l_os_1 += p->fp_l_os_1;

            p->fp_l_os_2 = (p->ps_fx_out2l+(fb-nl_fb)*p->fp_l_os_2)*sinout*p->r;
            MYFLT s_out_l_os_2 = m_out*p->ps_fx_out2l;
            s_out_l_os_2 += p->fp_l_os_2;

            //------------------------
            //bandlimit
            //------------------------
            MYFLT bl3_1 = p->bl2_1;
            MYFLT bl3_2 = p->bl2_2;

            p->bl2_1 = p->bl1_1;
            p->bl2_2 = p->bl1_2;

            p->bl1_1 = s_out_l_os_1;
            p->bl1_2 = s_out_l_os_2;

            MYFLT bl_out1 = (p->bl1_1*p->bl_c1 + p->bl2_1*p->bl_c2 + bl3_1*p->bl_c3);
            MYFLT bl_out2 = (p->bl1_2*p->bl_c1 + p->bl2_2*p->bl_c2 + bl3_2*p->bl_c3);

            //------------------------
            //power series out
            //------------------------
            MYFLT o_fx_out1l = 0.5*(bl_out1+p->o_fx_out2l);
            p->o_fx_out2l = 0.5*(bl_out2+o_fx_out1l);

            //fir restoration
            MYFLT s3l = p->s2l;
            p->s2l = p->s1l;
            p->s1l = o_fx_out1l;

            fx_outl = (p->s1l*p->c1+p->s2l*p->c2+s3l*p->c3)*p->fgain;
        }

        aout[i] = fx_outl * p->r * outgain;
    }
    p->seed = seed;
    p->sinp = sinp;
#ifdef USE_LOOPUP_SINE
    p->phase = intphase;
#endif
    return OK;
}


/**
 * pread / pwrite
 *
 * Communicate between notes via p-fields
 *
 */

INSTRTXT *find_instrdef(INSDS *insdshead, int instrnum) {
    INSDS *instr = insdshead->nxtact;
    while (instr) {
        if(instr->insno == instrnum)
            return instr->instr;
        instr = instr->nxtact;
    }
    // not found yet, search backwards
    instr = insdshead->prvact;
    while (instr) {
        if(instr->insno == instrnum)
            return instr->instr;
        instr = instr->prvact;
    }
    return NULL;
}

INSDS *find_instance_exact(INSTRTXT *instrdef, MYFLT instrnum, int must_be_active) {
    // Find an exact instance of the given instr, returns NULL if not found
    INSDS *instance = instrdef->instance;
    if(instance == NULL) {
        // printf("find_instance_exact: No instances of instr\n");
        return NULL;
    }
    while(instance) {
        // printf("Searching for %f, looking at %f (active=%c)\n", instrnum, instance->p1.value, instance->actflg);
        if(instance->p1.value == instrnum && (instance->actflg || !must_be_active )) {
            return instance;
        }
        instance = instance->nxtinstance;
    }
    return NULL;
}


/**
 * pread
 *
 * read p-fields from a different instrument
 *
 * iout pread instrnum, index  [, inotfound=-1]
 * kout pread instrnum, index  [, inotfound=-1]
 * kout pread instrnum, kindex [, inotfound=-1]
 * iout pread instrnum, index[], inotfound=-1
 * kout pread instrnum, kindex[], inotfound=-1
 * kout pread instrnum, index[], inotfound=-1
 */

typedef struct {
    OPDS h;
    MYFLT *outval, *instrnum, *pindex, *inotfound;

    CS_VAR_MEM *pfields;    // points to instr's pfields
    int maxpfield;         // max readable pfield
    INSDS *instr;         // found instr
    int retry;            // should we retry when not found?
    int found;            // -1 if not yet searched, 0 if not found, 1 if found
    INSTRTXT *instrtxt;   // instrument definition
} PREAD;

typedef struct {
    OPDS h;
    ARRAYDAT *outvals;
    MYFLT *instrnum;
    ARRAYDAT *pindexes;
    MYFLT *inotfound;

    CS_VAR_MEM *pfields;    // points to instr's pfields
    int maxpfield;         // max readable pfield
    INSDS *instr;         // found instr
    int retry;            // should we retry when not found?
    int found;            // -1 if not yet searched, 0 if not found, 1 if found
    INSTRTXT *instrtxt;   // instrument definition
} PREADARR;


int32_t pread_search_(CSOUND *csound, MYFLT p1, INSTRTXT **instrtxt, INSDS **instr) {
    IGN(csound);
    INSTRTXT *instrtxt_ = *instrtxt;
    if(!instrtxt_) {
        instrtxt_ = csound->GetInstrument(csound, (int)p1, NULL);
        if(!instrtxt_) {
            // No instances of instr
            return 0;
        }
    }
    INSDS *instr_;
    if(p1 != floor(p1)) {
        // fractional instrnum
        instr_ = find_instance_exact(instrtxt_, p1, 1);
    } else {
        // find first instance of this instr
        instr_ = instrtxt_->instance;
        while(instr_) {
            if(instr_->actflg)
                break;
            instr_ = instr_->nxtinstance;
        }
    }
    if(!instr_)
        return 0;
    *instrtxt = instrtxt_;
    *instr = instr_;
    return 1;
}

int32_t pread_search(CSOUND *csound, PREAD *p) {
    int found = pread_search_(csound, *p->instrnum, &(p->instrtxt), &(p->instr));
    p->found = found;
    if(!found)
        return 0;
    p->pfields = &(p->instr->p0);
    p->maxpfield = p->instrtxt->pmax;
    return 1;
}

/*
int32_t _pread_search(CSOUND *csound, PREAD *p) {
    IGN(csound);
    MYFLT p1 = *p->instrnum;
    INSDS *instr;
    p->found = 0;
    if(!p->instrtxt) {
        printf("pread_searh: GetInstrument(p1=%d)\n", (int)p1);
        p->instrtxt = csound->GetInstrument(csound, (int)p1, NULL);
    }
    if(!p->instrtxt) {
        printf("No instances of instr %d\n", (int)p1);
        return 0;
    }
    // found an instrument definition
    p->maxpfield = p->instrtxt->pmax;

    if(p1 != floor(p1)) {
        // fractional instrnum
        printf("Searching exact instance\n");
        instr = find_instance_exact(p->instrtxt, p1);
    } else {
        // find first instance of this instr
        instr = p->instrtxt->instance;
        while(instr) {
            if(instr->actflg)
                break;
            instr = instr->nxtinstance;
        }
    }
    if(!instr)
        return 0;
    p->found = 1;
    p->instr = instr;
    p->pfields = &(instr->p0);
    return 1;
}
*/

static int32_t
pread_init(CSOUND *csound, PREAD *p) {
    IGN(csound);
    p->pfields = NULL;
    p->maxpfield = 0;
    p->instr = NULL;
    p->found = -1;
    p->instrtxt = NULL;
    MYFLT p1 = *p->instrnum;
    // a negative instr. number indicates NOT to search again after failed to
    // find the given instance
    if(p1 < 0) {
        p->retry = 0;
        *p->instrnum = p1 = -p1;
    } else {
        p->retry = 1;
    }
    *p->outval = *p->inotfound;
    return OK;
}

static int32_t
preadarr_init(CSOUND *csound, PREADARR *p) {
    IGN(csound);
    p->pfields = NULL;
    p->maxpfield = 0;
    p->instr = NULL;
    p->found = -1;
    p->instrtxt = NULL;
    MYFLT p1 = *p->instrnum;
    if(p->pindexes->dimensions != 1) {
        return INITERRF("Expected a 1D array, got %d", p->pindexes->dimensions);
    }
    int numpfields = p->pindexes->sizes[0];
    tabinit(csound, p->outvals, numpfields);
    // a negative instr. number indicates NOT to search again after failed to
    // find the given instance
    if(p1 < 0) {
        p->retry = 0;
        *p->instrnum = p1 = -p1;
    } else {
        p->retry = 1;
    }
    return OK;
}

static int32_t
pread_perf(CSOUND *csound, PREAD *p) {
    int idx = (int)*p->pindex;
    if(p->found == -1 || (p->found==0 && p->retry)) {
        int found = pread_search(csound, p);
        if (!found) {
            printf("pread_perf: instr %f not found\n", *p->instrnum);
            return OK;
        }
    }
    if(!p->instr->actflg) {
        return OK;
    }
    if(idx > p->maxpfield) {
        return PERFERRF(Str("pread: can't read p%d (max index = %d)"), idx, p->maxpfield);
    }
    MYFLT value = p->pfields[idx].value;
    *p->outval = value;
    return OK;
}


static int32_t
preadarr_perf(CSOUND *csound, PREADARR *p) {
    int numindexes = p->pindexes->sizes[0];
    tabcheck(csound, p->outvals, numindexes, &(p->h));

    if(p->found == -1 || (p->found==0 && p->retry)) {
        p->found = pread_search_(csound, *p->instrnum, &(p->instrtxt), &(p->instr));
        if (!p->found) {
            printf("pread_perf: instr %f not found\n", *p->instrnum);
            for(int i=0; i<numindexes; i++) {
                p->outvals->data[i] = *p->inotfound;
            }
            return OK;
        }
        p->maxpfield = p->instrtxt->pmax;
        p->pfields = &(p->instr->p0);
    }
    if(!p->instr->actflg) {
        return OK;
    }

    for(int i=0; i<numindexes; i++) {
        int pfield = (int)(p->pindexes->data[i]);
        if(pfield > p->maxpfield) {
            p->outvals->data[i] = *p->inotfound;
            printf("pread.arr: can't read p%d (max index = %d)", pfield, p->maxpfield);
        } else {
            p->outvals->data[i] = p->pfields[pfield].value;
        }
    }
    return OK;
}


static int32_t
pread_i(CSOUND *csound, PREAD *p) {
    int ans = pread_init(csound, p);
    if(ans == NOTOK)
        return NOTOK;
    return pread_perf(csound, p);
}

static int32_t
preadarr_i(CSOUND *csound, PREADARR *p) {
    int ans = preadarr_init(csound, p);
    if(ans == NOTOK)
        return NOTOK;
    return preadarr_perf(csound, p);
}


#define PWRITE_MAXINPUTS 40


/**
 * pwrite
 *
 * Modifies the p-fields of another instrument
 *
 * pwrite instrnum, index1, value1 [, index2, value2, ...]
 *
 * instrnum (i): the instrument number.
 *      * If a fractional number is given, only the instance with this
 *        exact number is modified.
 *      * If an integer number is given, ALL instances of this instrument are modified
 *      * Matching instances are searched at performance, and search is retried
 *        if no matching instances were found. If a negative instrnum is given,
 *        search is attempted only once and the opcode becomes a NOOP if no instances
 *        were found
 */

enum PwriteStatus { FirstRun, InstanceNotFound, InstanceFound, NoOp };


typedef struct {
    OPDS h;
    // inputs
    MYFLT *instrnum;
    MYFLT *inputs[PWRITE_MAXINPUTS];

    MYFLT p1;             // cached instrnum, will always be positive
    int numpairs;         // number of index:value pairs
    int retry;            // should we retry when no instance matched?
    INSDS *instr;         // a pointer to the found instance, when matching exact number
    INSTRTXT *instrtxt;   // used in broadcasting mode, a pointer to the instrument template
    int maxpfield;        // max pfield accepted by the instrument
    int broadcasting;     // are we broadcasting?
    enum PwriteStatus status;
    CS_VAR_MEM *pfields;  // a pointer to the instance pfield, when doing exact matching
} PWRITE;


static inline void
pwrite_writevalues(CSOUND *csound, PWRITE *p, CS_VAR_MEM *pfields) {
    for(int pair=0; pair < p->numpairs; pair++) {
        int indx = (int)*(p->inputs[pair*2]);
        MYFLT value = *(p->inputs[pair*2+1]);
        if(indx > p->maxpfield)
            MSGF("pwrite: can't write to p%d (max index=%d)\n", indx, p->maxpfield);
        else
            pfields[indx].value = value;
    }
}


static int32_t
pwrite_search(CSOUND *csound, PWRITE *p) {
    IGN(csound);
    MYFLT p1 = p->p1;
    if(p->instrtxt == NULL) {
        // INSTRTXT *instrdef = find_instrdef(p->h.insdshead, (int)p1);
        INSTRTXT *instrdef = csound->GetInstrument(csound, (int)p1, NULL);
        if(!instrdef) {
            return 0;
        }
        p->instrtxt = instrdef;
        p->maxpfield = instrdef->pmax;
    }
    // if we are not broadcasting, search the exact match
    if (!(p->broadcasting)) {
        INSDS *instr = find_instance_exact(p->instrtxt, p1, 1);
        if(!instr) {
            return 0;
        }
        p->instr = instr;
        p->pfields = &(instr->p0);
    }
    return 1;
}


static int32_t
pwrite_initcommon(CSOUND *csound, PWRITE *p) {
    IGN(csound);
    MYFLT p1 = *p->instrnum;
    if (p1 < 0) {
        p1 = -p1;
        p->retry = 0;
    } else {
        p->retry = 1;
    }
    p->p1 = p1;
    p->broadcasting = floor(p1) == p1;
    p->numpairs = (csound->GetInputArgCnt(p) - 1) / 2;
    p->status = FirstRun;
    p->instrtxt = NULL;
    return OK;
}


static int32_t
pwrite_perf(CSOUND *csound, PWRITE *p) {
    if(p->status == NoOp)
        return OK;
    if(p->status == FirstRun) {
        if(pwrite_search(csound, p))
            p->status = InstanceFound;
        else {
            p->status = p->retry ? InstanceNotFound : NoOp;
            return OK;
        }
    } else if (p->status == InstanceNotFound && p->retry) {
        if(pwrite_search(csound, p))
            p->status = InstanceFound;
        else
            return OK;
    }

    if(p->status != InstanceFound) {
        return PERFERR("This should not happen!");
    }

    if (!p->broadcasting) {
        if(p->instr->actflg && p->p1 == p->instr->p1.value) {
            pwrite_writevalues(csound, p, p->pfields);
        } else {
            // the instr is not active anymore
            p->status = NoOp;
        }
        return OK;
    }
    // broadcasting mode
    INSDS *instance = p->instrtxt->instance;
    while(instance) {
        if (instance->actflg) {  // is this instance active?
            pwrite_writevalues(csound, p, &(instance->p0));
        }
        instance = instance->nxtinstance;
    }
    return OK;
}


static int32_t
pwrite_i(CSOUND *csound, PWRITE *p) {
    int32_t ans = pwrite_initcommon(csound, p);
    if(ans == NOTOK) return NOTOK;
    return pwrite_perf(csound, p);
}

static int32_t
pwriten_init(CSOUND *csound, PWRITE *p) {
    p->p1 = 0;
    p->broadcasting = 0;
    p->numpairs = (csound->GetInputArgCnt(p) - 1) / 2;
    p->status = FirstRun;
    p->instrtxt = NULL;
    p->maxpfield = 0;
    return OK;
}

static int32_t
pwriten_perf(CSOUND *csound, PWRITE *p) {
    INSDS *instance = (INSDS *) ((uintptr_t)*p->instrnum);
    p->maxpfield = instance->instr->pmax;
    if (instance->actflg) {
        pwrite_writevalues(csound, p, &(instance->p0));
    }
    return OK;
}


/** uniqinstance
 *
 * given an integer instrument number, return a fractional instr. number
 * which is not active now and can be used as p1 for "event" or similar
 * opcodes to create a unique instance of an instrument
 *
 * instrnum  uniqinstrance integer_instrnum
 *
 */

#define UNIQ_NUMSLOTS 10000

typedef struct {
    OPDS h;
    MYFLT *out, *int_instrnum, *max_instances;
    int p1;
    int numslots;
    char slots[UNIQ_NUMSLOTS];
} UNIQINSTANCE;

static MYFLT
uniqueinstance_(CSOUND *csound, UNIQINSTANCE *p) {
    // char slots[UNIQ_NUMSLOTS] = {0};
    char *slots = p->slots;
    int numslots = p->numslots;
    memset(slots, 0, numslots);
    int p1 = p->p1;
    int idx;
    int maxidx = 0;
    int minidx = numslots;
    MYFLT fractional_part, integral_part;

    INSTRTXT *instrtxt = csound->GetInstrument(csound, p1, NULL);
    if(!instrtxt || instrtxt->instance == NULL) {
        // no instances of this instrument, so pick first index
        return p1 + FL(1)/numslots;
    }

    INSDS *instance = instrtxt->instance;
    while(instance) {
        if(instance->actflg && instance->p1.value != instance->insno) {
            fractional_part = modf(instance->p1.value, &integral_part);
            idx = (int)(fractional_part * numslots + 0.5);
            if(idx >= numslots)
                continue;
            if(idx > maxidx)
                maxidx = idx;
            else if(idx < minidx)
                minidx = idx;
            slots[idx] = 1;
        }
        instance = instance->nxtinstance;
    }
    if(maxidx + 1 < numslots) {
        return p1 + (maxidx+1)/FL(numslots);
    }
    if(minidx > 2) {
        return p1 + (minidx-1)/FL(numslots);
    }
    for(int i=1; i < numslots; i++) {
        if(slots[i] == 0) {
            return p1 + i / FL(numslots);
        }
    }
    return -1;
}

static int32_t
uniqueinstance_initcommon(CSOUND *csound, UNIQINSTANCE *p) {
    IGN(csound);
    p->numslots = (int)*p->max_instances;
    if(p->numslots == 0)
        p->numslots = UNIQ_NUMSLOTS;
    else if(p->numslots > UNIQ_NUMSLOTS)
        p->numslots = UNIQ_NUMSLOTS;
    MYFLT instrnum = uniqueinstance_(csound, p);
    *p->out = instrnum;
    return OK;
}

static int32_t
uniqueinstance_i(CSOUND *csound, UNIQINSTANCE *p) {
    p->p1 = (int)*p->int_instrnum;
    return uniqueinstance_initcommon(csound, p);
}

static int32_t
uniqueinstance_S_init(CSOUND *csound, UNIQINSTANCE *p) {
    STRINGDAT *instrname = (STRINGDAT *)p->int_instrnum;
    p->p1 = csound->strarg2insno(csound, instrname->data, 1);
    if (UNLIKELY(p->p1 == NOT_AN_INSTRUMENT))
        return NOTOK;
    return uniqueinstance_initcommon(csound, p);
}


/*

instr 100  ; generator
    pset 0,0,0, 440, 0.5, 4000
    kfreq, kamp, kcutoff passign 4
    a0 ... ; do something with this params
endin

instr 200  ; controls
    inst uniqinstance 100
    event_i inst, 0, -1
    kfreq line 440, p3, 880
    kcutoff line 4000, p3, 400
    pwrite inst, 4, kfreq, 6, kcutoff
endin

 */


/** ----------------- atstop ---------------

  schedule an instrument when the note stops

  atstop works at instr deinit. Any k-variable
  might already be deallocated so we copy all arguments
  at init time

  atstop Sintr, idelay, idur, pfields...
  atstop instrnum, idelay, idur, pfields...

*/

#define ATSTOP_MAXPARGS 64

typedef struct {
    OPDS h;
    // outputs

    // inputs
    void *instr;
    MYFLT *pargs [ATSTOP_MAXPARGS];

    // internal
    MYFLT instrnum;   // cached instrnum

    MYFLT pargscopy[ATSTOP_MAXPARGS];
    size_t numargs;

} SCHED_DEINIT;


static int32_t
atstop_deinit(CSOUND *csound, SCHED_DEINIT *p) {
    EVTBLK evt;
    memset(&evt, 0, sizeof(EVTBLK));
    evt.opcod = 'i';
    evt.strarg = NULL;
    evt.scnt = 0;
    evt.pinstance = NULL;
    evt.p2orig = p->pargscopy[0]; // *p->pargs[0];
    evt.p3orig = p->pargscopy[1]; // *p->pargs[1];
    size_t pcnt = p->numargs;
    evt.p[1] = p->instrnum;
    for(size_t i=0; i < pcnt-1; i++) {
        evt.p[2+i] = p->pargscopy[i];
        // evt.p[2+i] = *p->pargs[i];
    }
    evt.pcnt = (int16_t) pcnt;
    csound->insert_score_event_at_sample(csound, &evt,
                                         csound->GetCurrentTimeSamples(csound));
    return OK;
}


static int32_t
atstop_(CSOUND *csound, SCHED_DEINIT *p, MYFLT instrnum) {
    p->instrnum = instrnum;
    p->numargs = max(3, p->INOCOUNT);
    for(size_t i=0; i < p->numargs - 1;i++) {
        p->pargscopy[i] = *p->pargs[i];
    }
    register_deinit(csound, p, atstop_deinit);
    return OK;
}

static int32_t
atstop_i(CSOUND *csound, SCHED_DEINIT *p) {
    MYFLT instrnum = *((MYFLT*)p->instr);
    return atstop_(csound, p, instrnum);
}

static int32_t
atstop_s(CSOUND *csound, SCHED_DEINIT *p) {
    STRINGDAT *instrname = (STRINGDAT*) p->instr;
    int32_t instrnum = csound->strarg2insno(csound, instrname->data, 1);
    if (UNLIKELY(instrnum == NOT_AN_INSTRUMENT)) return NOTOK;
    return atstop_(csound, p, (MYFLT) instrnum);
}

/*
 * accum
 *
 * a simple accumulator
 *
 * kout accum kstep, initial=0, reset=0
 *
 * Can be used together with changed for all opcodes which need an increasing
 * trigger (printf, for example), or as a simple phasor
 *
 * printf "kvar=%f \n", accum(changed(kvar)), kvar
 *
 */
typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *step, *initial_value, *reset;
    MYFLT value;
} ACCUM;

static int32_t
accum_init(CSOUND *csound, ACCUM *p) {
    p->value = *p->initial_value;
    return OK;
}

static int32_t
accum_perf(CSOUND *csound, ACCUM *p) {
    if(*p->reset == 1) {
        p->value = *p->initial_value;
    }
    *p->out = p->value;
    p->value += *p->step;
    return OK;
}

static int32_t
accum_perf_audio(CSOUND *csound, ACCUM *p) {
    MYFLT step = *p->step;
    MYFLT value = *p->reset == 0 ? p->value : *p->initial_value;
    MYFLT *out = p->out;

    SAMPLE_ACCURATE(out);

    for(n=offset; n<nsmps; n++) {
        out[n] = value;
        value += step;
    }
    p->value = value;
    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *in1;
    MYFLT *in2;
} FUNC12;

/** frac2int
 *
 * convert the fractional part to an integer by extracting
 * the fractional part and multiplying by a factor
 *
 * thourght to be used together with fractional p1
 *
 * schedule 10 + inum/1000, ...
 *
 * in instr 10
 * inum = frac2int(p1, 1000)  ; this should result in the original inum if the same factor
 *                            ; (in this case 1000) is used
 *
 * NB: the integral part is discarded
 *
 * The inverse is not necessary, as it is simply inum/ifactor
 */

// frac2int(10.456, 1000) -> 456
// frac2int(0.134, 100)   -> 13


static int32_t
frac2int(CSOUND *csound, FUNC12 *p) {
    IGN(csound);
    double integ;
    double fract = modf(*p->in1, &integ);
    MYFLT mul = *p->in2;
    *p->out = floor(fract * mul + 0.5);
    return OK;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*

  iout[]  vecview ifn, istart, iend

  An array view into a function table. The returned array does not
  own the memory, it is just a view (an alias) into the memory
  allocated by the function table

 */

// init array 'arr' to point to data


static void _init_array_view(CSOUND *csound, ARRAYDAT *outarr, MYFLT *sourcedata,
                             int sourcesize, size_t allocated) {
    if(outarr->data != NULL) {
        printf("$$$ freeing original data (size=%d, allocated=%ld) \n",
               outarr->sizes[0], outarr->allocated);
        csound->Free(csound, outarr->data);
    } else {
        CS_VARIABLE* var = outarr->arrayType->createVariable(csound, NULL);
        outarr->arrayMemberSize = var->memBlockSize;
    }
    size_t minallocated = outarr->arrayMemberSize * sourcesize;
    outarr->allocated = minallocated > allocated ? minallocated : allocated;
    outarr->data = sourcedata;
    outarr->dimensions = 1;
    if(outarr->sizes == NULL)
        outarr->sizes = csound->Malloc(csound, sizeof(int));
    outarr->sizes[0] = sourcesize;
    // _mark_array_as_view(csound, outarr);
}




// arrsetslice in, start=0, end=0, step=1
// arrsetslice in, start=0, end=0
typedef struct {
    OPDS h;
    ARRAYDAT *in;
    MYFLT *value, *start, *end, *step;
} ARRSETSLICE;

static int32_t
array_set_slice(CSOUND *csound, ARRSETSLICE *p) {
    if(p->in->dimensions!=1) {
        MSGF("Expected array of 1 dimension, but array has"
             "got %d dimensions", p->in->dimensions);
        return NOTOK;
    }
    int32_t start = (int32_t)*p->start;
    int32_t end = (int32_t)*p->end;
    int32_t step = (int32_t)*p->step;
    int32_t ksmps = p->h.insdshead->ksmps;
    MYFLT value = *p->value;
    if(end == 0) {
        end = p->in->sizes[0];
    }
    MYFLT *data = p->in->data;
    char vartype = p->in->arrayType->varTypeName[0];
    switch(vartype) {
    case 'i':
    case 'k':
        for(int i=start; i<end; i+=step) {
            data[i] = value;
        }
        break;
    case 'a':
        for(int i=start*ksmps; i<end*ksmps; i+=step) {
            data[i] = value;
        }
    }
    return OK;
}


typedef struct {
    OPDS h;
    ARRAYDAT *out;
    MYFLT *ifn, *istart, *iend;
    FUNC * ftp;
    int size;
} TABALIAS;

static int tabalias_deinit(CSOUND *csound, TABALIAS *p) {
    IGN(csound);
    p->out->data = NULL;
    p->out->sizes[0] = 0;
    return OK;
}

static int tabalias_init(CSOUND *csound, TABALIAS *p) {
    FUNC *ftp = csound->FTnp2Find(csound, p->ifn);
    if (UNLIKELY(ftp == NULL))
        return NOTOK;
    int tabsize = ftp->flen;
    int start = (int)*p->istart;
    int end   = (int)*p->iend;
    if(end == 0)
        end = tabsize;
    p->size = end - start;
    p->ftp = ftp;

    if (tabsize < end)
        return INITERR("end is bigger than the length of the table");

    _init_array_view(csound, p->out, &(ftp->ftable[start]), end-start,
                     sizeof(MYFLT)*(tabsize-start));
    register_deinit(csound, p, tabalias_deinit);
    return OK;
}


/*

  iout[]  vecview  in[],  istart, iend=0
  kout[]  vecview  kin[], istart, iend=0

  An array view into another array, useful to operate on a row
  of a large 2D array.

*/


typedef struct {
    OPDS h;
    ARRAYDAT *out, *in;
    MYFLT *istart, *iend;
    MYFLT *dataptr;
    int size;
} ARRAYVIEW;

static void _array_view_deinit(ARRAYDAT *arr) {
    arr->data = NULL;
    arr->sizes[0] = 0;
}


static int
    arrayview_deinit(CSOUND *csound, ARRAYVIEW *p) {
    IGN(csound);
    _array_view_deinit(p->out);
    return OK;
}


static int32_t
    arrayview_init(CSOUND *csound, ARRAYVIEW *p) {
    if(p->in->data == NULL)
        return INITERR("source array has not been initialized");
    if(p->in->dimensions > 1)
        return INITERR(Str("A view can only be taken from a 1D array"));

    int end   = (int)*p->iend;
    int start = (int)*p->istart;
    if(end == 0)
        end = p->in->sizes[0];
    _init_array_view(csound, p->out, &(p->in->data[start]), end - start,
                     p->in->allocated - start);
    register_deinit(csound, p, arrayview_deinit);
    return OK;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*
 * ref
 *
 * iref ref iArray
 * iArray deref iref
 *
 * iref ref kscalar
 * kscalar deref iref
 *
 */


enum RefType { RefScalar, RefAudio, RefArrayScalar };

typedef struct REF_GLOBALS_ REF_GLOBALS;

typedef struct {
    int active;
    MYFLT *data;
    enum RefType type;
    int size;
    int *sizes;
    size_t allocated;
    int refcount;
    int ownsdata;
    int isglobal;
    int slot;
    REF_GLOBALS *g;
} REF_HANDLE;

struct REF_GLOBALS_ {
    CSOUND *csound;
    int numhandles;
    REF_HANDLE *handles;
    intarray_t slots;
};

static inline int32_t ref_handle_valid(REF_GLOBALS *g, int slot) {
    // array out of bounds
    if(slot < 0 || slot >= g->numhandles)
        return 0;
    REF_HANDLE *h = &(g->handles[slot]);
    // an active handle is always valid
    if(h->active == 1)
        return 1;
    // an inactive array handle can also be valid as a deref if it owns data
    if(h->type == RefArrayScalar && h->ownsdata)
        return 1;
    return 0;
}

// static int32_t ref_reset(CSOUND *csound, REF_GLOBALS *g);

#define REF_GLOBALS_VARNAME "__ref_globals__"

static int32_t ref_reset(CSOUND *csound, REF_GLOBALS *g) {
    for(int i=0; i<g->numhandles; i++) {
        REF_HANDLE *h = &(g->handles[i]);
        if(h->ownsdata && h->data!=NULL) {
            csound->Free(csound, h->data);
            if(h->sizes != NULL)
                csound->Free(csound, h->sizes);
        }
        h->active = 0;
    }
    csound->Free(csound, g->handles);
    intpool_deinit(csound, &(g->slots));
    csound->DestroyGlobalVariable(csound, REF_GLOBALS_VARNAME);
    return OK;
}

static REF_GLOBALS *ref_globals(CSOUND *csound) {
    REF_GLOBALS *g = csound->QueryGlobalVariable(csound, REF_GLOBALS_VARNAME);
    if(g != NULL) return g;

    int err = csound->CreateGlobalVariable(csound, REF_GLOBALS_VARNAME, sizeof(REF_GLOBALS));
    if(err != 0) {
        INITERR("failed to create globals for ref");
        return NULL;
    }
    g = csound->QueryGlobalVariable(csound, REF_GLOBALS_VARNAME);
    g->csound = csound;
    // initial size for handles, will double when full
    g->numhandles = 64;
    g->handles = csound->Calloc(csound, sizeof(REF_HANDLE) * g->numhandles);
    intpool_init(csound, &(g->slots), g->numhandles);
    csound->RegisterResetCallback(csound, (void *)g, (int32_t(*)(CSOUND*, void*))ref_reset);
    return g;
}

static inline void _ref_handle_init(REF_HANDLE *h, REF_GLOBALS *g, int slot) {
    h->refcount = 0;
    h->ownsdata = 0;
    h->size = 0;
    h->allocated = 0;
    h->data = NULL;
    h->sizes = NULL;
    h->active = 0;
    h->slot = slot;
    h->g = g;
}


static void _init_array_clone(CSOUND *csound, ARRAYDAT *outarr, REF_HANDLE *h) {
    if(outarr->data != NULL) {
        printf("$$$ freeing original data (size=%d, allocated=%ld) \n",
               outarr->sizes[0], outarr->allocated);
        csound->Free(csound, outarr->data);
    } else {
        CS_VARIABLE* var = outarr->arrayType->createVariable(csound, NULL);
        outarr->arrayMemberSize = var->memBlockSize;
    }
    if(outarr->sizes != NULL) {
        csound->Free(csound, outarr->sizes);
    }
    outarr->allocated = h->allocated;
    outarr->sizes = h->sizes;
    outarr->dimensions = 1;
    outarr->data = h->data;
}


static int32_t _ref_handle_release(CSOUND *csound, REF_HANDLE *h) {
    if(h->data != NULL && h->ownsdata) {
        csound->Free(csound, h->data);
        csound->Free(csound, h->sizes);
        h->data = NULL;
        h->sizes = NULL;
        if(csound->GetDebug(csound)) {
            MSG("ref: Releasing memory of array ref \n");
        }
    }
    int res = intpool_push(csound, &(h->g->slots), h->slot);
    if(res == NOTOK) {
        MSGF("Could not return slot %d to pool", h->slot);
        return NOTOK;
    }
    _ref_handle_init(h, h->g, h->slot);
    return OK;
}

static inline int32_t
_ref_get_slot(REF_GLOBALS *g) {
    CSOUND *csound = g->csound;
    int freeslot = intpool_pop(csound, &(g->slots));
    if(g->slots.capacity > g->numhandles) {
        // if we are out of slots the slot pool is resized. If it is bigger
        // that the number of handles, we need to match the handles array
        g->numhandles = g->slots.capacity;
        g->handles = csound->ReAlloc(csound, g->handles, sizeof(REF_HANDLE) * g->numhandles);
        if(g->handles == NULL) {
            printf("Memory error\n");
            return -1;
        }
    }
    REF_HANDLE *h = &(g->handles[freeslot]);
    if(h->active == 1) {
        printf("Got free slot %d, but handle is active???\n", freeslot);
        return -1;
    }
    _ref_handle_init(h, g, freeslot);
    return freeslot;
}


typedef struct {
    OPDS h;
    MYFLT *idx;
    ARRAYDAT *arr;
    MYFLT *extrarefs;
    int slot;
    REF_GLOBALS *g;
} REF_NEW_ARRAY;

void _handle_copy_data_from_array(CSOUND *csound, ARRAYDAT *arr, REF_HANDLE *h) {
    size_t numbytes = arr->arrayMemberSize * arr->sizes[0];
    size_t sizes_numbytes = sizeof(int) * arr->dimensions;
    h->data = csound->Malloc(csound, numbytes);
    h->sizes = csound->Malloc(csound, sizes_numbytes);
    memcpy(h->data, arr->data, numbytes);
    memcpy(arr->sizes, h->sizes, sizes_numbytes);
    h->size = arr->sizes[0];
    h->allocated = numbytes;
    h->ownsdata = 1;
}

void _ref_array_transfer(CSOUND *csound, ARRAYDAT *src, REF_HANDLE *h) {
    // this should NOT be called on global arrays
    h->active = 1;
    h->refcount = 1;
    h->data = src->data;
    h->sizes = src->sizes;
    h->size = src->sizes[0];
    h->allocated = src->allocated;
    h->ownsdata = 1;
    h->type = RefArrayScalar;
}

static int32_t ref_new_deinit(CSOUND *csound, REF_NEW_ARRAY *p);
static int32_t ref_local_deinit(CSOUND *csound, REF_NEW_ARRAY *p);


static int32_t
ref_new_array(CSOUND *csound, REF_NEW_ARRAY *p) {
    if(p->arr->data == NULL || p->arr->allocated == 0) {
        return INITERR("Cannot take a reference from uninitialized array");
    }
    if(p->arr->dimensions != 1) {
        return INITERRF("Only 1D arrays supported (array has %d dims)", p->arr->dimensions);
    }
    REF_GLOBALS *g = ref_globals(csound);
    int slot = _ref_get_slot(g);
    if(slot == -1) {
        return INITERR("ref (array): Could not get a free slot");
    }
    REF_HANDLE *h = &(g->handles[slot]);
    _ref_handle_init(h, g, slot);
    h->active = 1;
    p->slot = slot;
    char *argname = csound->GetInputArgName(p, 0);
    h->isglobal = argname[0] == 'g' ? 1 : 0;
    if(!(h->isglobal)) {
        _ref_array_transfer(csound,  p->arr, h);
        h->refcount += (int)*p->extrarefs;
        register_deinit(csound, p, ref_local_deinit);
    } else {
        h->active = 1;
        h->type = RefArrayScalar;  // todo: detect type
        h->data = p->arr->data;
        h->size = p->arr->sizes[0];
        h->sizes = p->arr->sizes;
        h->allocated = p->arr->allocated;
        h->ownsdata = 0;
        h->refcount = 1;
        register_deinit(csound, p, ref_new_deinit);
    }
    p->g = g;
    *p->idx = slot;
    p->slot = slot;
    return OK;
}


static int32_t ref_handle_decref(CSOUND *csound, REF_HANDLE *h) {
    if(h == NULL)
        return INITERR("handle is NULL!");
    if(h->refcount <= 0)
        return INITERRF("Cannot decrease refcount (refcount now: %d)", h->refcount);
    if(h->active == 0)
        return INITERRF("Handle %d is not active", h->slot);
    h->refcount--;
    if(h->refcount == 0) {
        if(_ref_handle_release(csound, h) == NOTOK)
            return INITERRF("Error while releasing handle for slot %d", h->slot);
    }
    return OK;
}


static int32_t
ref_new_deinit(CSOUND *csound, REF_NEW_ARRAY *p) {
    REF_HANDLE *h = &(p->g->handles[p->slot]);
    h->active = 0;
    // if there are clients of this data, we should own the
    // data (if we don't already own it or it is global)
    h->refcount--;
    if(h->refcount < 0) {
        return PERFERRF("Error deiniting ref: refcount was %d", h->refcount);
    }
    if(h->refcount > 0) {
        // there are clients
        if(!h->isglobal && !h->ownsdata) {
            _handle_copy_data_from_array(csound, p->arr, h);
        }
    } else {
        // no clients
        if(_ref_handle_release(csound, h) == NOTOK)
            return PERFERR("Error releasing handle");
    }
    return OK;
}

static int32_t
ref_local_deinit(CSOUND *csound, REF_NEW_ARRAY *p) {
    // zero own array to avoid deallocations
    // ref is just a normal client, like deref
    p->arr->data = NULL;
    p->arr->dimensions = 0;
    p->arr->sizes = NULL;
    p->arr->allocated = 0;
    if(ref_handle_decref(csound, &(p->g->handles[p->slot])) == NOTOK)
        return PERFERRF("Error decrementing reference for slot %d", p->slot);
    p->slot = -1;
    return OK;
}

typedef struct {
    OPDS h;
    ARRAYDAT *arr;
    MYFLT *idx;
    MYFLT *extrarefs;
    int slot;
    REF_GLOBALS *g;
} DEREF_ARRAY;


static int32_t
deref_array_deinit(CSOUND *csound, DEREF_ARRAY *p) {
    p->arr->data = NULL;
    p->arr->dimensions = 0;
    p->arr->sizes = NULL;
    p->arr->allocated = 0;
    ref_handle_decref(csound, &(p->g->handles[p->slot]));
    return OK;
}

static int32_t
deref_array(CSOUND *csound, DEREF_ARRAY *p) {
    int slot = (int)*p->idx;
    REF_GLOBALS *g = ref_globals(csound);
    if(ref_handle_valid(g, slot) == 0) {
        return INITERRF("Ref handle (%d) is not valid", slot);
    }
    REF_HANDLE *h = &(g->handles[slot]);
    if(h->data == NULL)
        return INITERR("Handle not active");
    if(h->allocated == 0)
        return INITERR("Array has no elements allocated");
    _init_array_clone(csound, p->arr, h);
    h->refcount++;
    int extrarefs = (int)*p->extrarefs;
    if(extrarefs >= h->refcount) {
        return INITERRF("deref: too many extra derefs (%d), refcount is %d", extrarefs, h->refcount);
    }
    h->refcount -= extrarefs;
    p->slot = slot;
    p->g = g;
    register_deinit(csound, p, deref_array_deinit);
    return OK;
}


typedef struct {
    OPDS h;
    MYFLT *args[10];
    REF_GLOBALS *g;
    REF_HANDLE *handle;
    int numouts;
} REF1;

#define RETURN_IF_NOTOK(code) { int32_t _ret = (code); if(_ret != OK) return _ret; }


static inline void
ref1_init(CSOUND *csound, REF1 *p) {
    p->g = ref_globals(csound);
    p->numouts = csound->GetOutputArgCnt(p);
}


static int32_t
ref_valid_perf(CSOUND *csound, REF1 *p) {
    IGN(csound);
    int slot = (int)*p->args[1];
    *p->args[0] = ref_handle_valid(p->g, slot);
    return OK;
}

static int32_t
ref_valid_i(CSOUND *csound, REF1 *p) {
    ref1_init(csound, p);
    return ref_valid_perf(csound, p);
}


/* xtracycles
 *
 * inumcycles xtracycles
 *
 * Returns the number of extra cycles (extended through opcodes)
 * like xtratim or through xtratim directly
 */

typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT state1;
} OPCk_0;

static int32_t
xtracycles(CSOUND *csound, OPCk_0 *p) {
    IGN(csound);
    int numcycles = p->h.insdshead->xtratim;
    *p->out = FL(numcycles);
    return OK;
}

/* errormsg
 *
 * errormsg Smsg
 * errormsg Skind, Smsg
 *
 * errormsg "my error message"
 * errormsg "warning", "this is only a warning, event will keep running"
 *
 * Skind can be "error", "warning" or "info"
 * If "error", (the default) a performance error is thrown, which will delete the current event
 * If "warning", a warning will be thrown but the event goes on (a warning is only shown once)
 * If "info", a warning will be thrown each iteration
 *
 * To throw a critical error stopping csound just use exitnow
 *
 * Only k-rate. If "init" rate is needed, just do
 * if timeinstk() == 1 then
 *   errormsg "my init error message"
 * endif
 */

enum Errorkind_t { ERRORMSG_ERROR, ERRORMSG_WARNING, ERRORMSG_INFO, ERRORMSG_INIT, ERRORMSG_UNKNOWN};

typedef struct {
    OPDS h;
    STRINGDAT *S1;
    STRINGDAT *S2;
    enum Errorkind_t kind;
    int warning_done;
    int which;
} ERRORMSG;


static int32_t errormsg_init(CSOUND *csound, ERRORMSG *p) {
    IGN(csound);
    if(!strcmp(p->S1->data, "init")) {
        p->which = ERRORMSG_UNKNOWN;
        return INITERRF("\n   %s\n", p->S2->data);
    }
    if(!strcmp(p->S1->data, "error"))
        p->kind = ERRORMSG_ERROR;
    else if(!strcmp(p->S1->data, "warning"))
        p->kind = ERRORMSG_WARNING;
    else if(!strcmp(p->S1->data, "info"))
        p->kind = ERRORMSG_INFO;
    else
        return INITERR("Unknown type");
    p->warning_done = 0;   // default: mark message as "error"
    p->which = 1;
    return OK;
}

static int32_t initerror(CSOUND *csound, ERRORMSG *p) {
    IGN(csound);
    INITERRF("\n   %s\n", p->S1->data);
    return NOTOK;
}

static int32_t errormsg_init0(CSOUND *csound, ERRORMSG *p) {
    IGN(csound);
    p->kind = ERRORMSG_ERROR;
    p->which = 0;
    return OK;
}



static int32_t errormsg_perf(CSOUND *csound, ERRORMSG *p) {
    char *name;
    INSDS *ip;
    char *msg = p->which == 0 ? p->S1->data : p->S2->data;

    switch(p->kind) {
    case ERRORMSG_ERROR:
        return csound->PerfError(csound, &(p->h), "%s\n", msg);
    case ERRORMSG_WARNING:
        if(p->warning_done)
            return OK;
        ip = p->h.insdshead;
        name = (ip->instr->insname != NULL) ? ip->instr->insname : "";
        csound->Warning(csound, "Warning from instr %d (%s), line %d\n    %s\n",
                       ip->insno, name, p->h.optext->t.linenum, msg);
        p->warning_done = 1;
        return OK;
    case ERRORMSG_INFO:
        ip = p->h.insdshead;
        name = (ip->instr->insname != NULL) ? ip->instr->insname : "";
        csound->Warning(csound, "Info from instr %d (%s), line %d\n    %s\n",
                       ip->insno, name, p->h.optext->t.linenum, msg);
        return OK;
    case ERRORMSG_UNKNOWN:
        return NOTOK;
    default:
        return csound->PerfError(csound, &(p->h),
                                 "throwerror: internal error %d\n", p->kind);
    }
}


typedef struct {
    OPDS h;
    ARRAYDAT *A;
    ARRAYDAT *B;
    char varTypeName;
    int sizeA;
} _AA;

typedef struct {
    OPDS h;
    ARRAYDAT *A;
    ARRAYDAT *B;
    MYFLT *k1;
    char varTypeName;
} _AAk;



static int32_t extendArray_init(CSOUND *csound, _AA *p) {
    p->sizeA = p->A->sizes[0];
    if(p->A->dimensions == 1 && p->B->dimensions == 1) {
        int size = p->A->sizes[0] + p->B->sizes[0];
        tabinit(csound, p->A, size);
        p->varTypeName = p->A->arrayType->varTypeName[0];
        return OK;
    }
    return NOTOK;
}

static int32_t extendArray_k(CSOUND *csound, _AA *p) {
    int offset = p->sizeA;
    if(p->varTypeName == 'S') {
        STRINGDAT *deststrs = (STRINGDAT*)p->A->data;
        STRINGDAT *srcstrs = (STRINGDAT*)p->B->data;
        for(int i=0; i<p->B->sizes[0]; i++) {
            char *srcstr = srcstrs[i].data;
            deststrs[offset + i].size = (int)strlen(srcstr);
            deststrs[offset + i].data = csound->Strdup(csound, srcstr);
        }
    } else if(p->varTypeName == 'k' || p->varTypeName == 'i') {
        memcpy(&(p->A->data[offset]), p->B->data, sizeof(MYFLT)*p->B->sizes[0]);
    } else {
        return PERFERRF("extendArray: Arrays of type %c not supported", p->varTypeName);
    }
    return OK;
}

static int32_t extendArray_i(CSOUND *csound, _AA *p) {
    int ret = extendArray_init(csound, p);
    if(ret == NOTOK) {
        return INITERR("Error initializing extendArray");
    }
    return extendArray_k(csound, p);
}


static int32_t setslice_array_k(CSOUND *csound, _AAk *p) {
    if(p->A->dimensions != 1 || p->B->dimensions != 1)
        return PERFERR("Arrays should be 1D");

    int offset = (int)*p->k1;
    int sizeB = p->B->sizes[0];
    int sizeA = p->A->sizes[0];
    int numitems = min(sizeA - offset, sizeB);
    if(p->varTypeName == 'S') {
        STRINGDAT *deststrs = (STRINGDAT*)p->A->data;
        STRINGDAT *srcstrs = (STRINGDAT*)p->B->data;
        for(int i=0; i<numitems; i++) {
            char *srcstr = srcstrs[i].data;
            deststrs[offset + i].size = (int)strlen(srcstr);
            deststrs[offset + i].data = csound->Strdup(csound, srcstr);
        }
    } else if(p->varTypeName == 'k' || p->varTypeName == 'i') {
        memcpy((char*)p->A->data+offset*sizeof(MYFLT), p->B->data, sizeof(MYFLT)*numitems);
    } else {
        MSGF("setslice: Arrays of type %c not supported", p->varTypeName);
        return NOTOK;
    }
    return OK;
}

static int32_t setslice_array_k_init_i(CSOUND *csound, _AAk *p) {
    p->varTypeName = 'i';
    return setslice_array_k(csound, p);
}

static int32_t setslice_array_k_init_k(CSOUND *csound, _AAk *p) {
    IGN(csound);
    p->varTypeName = 'k';
    return OK;
}

static int32_t setslice_array_k_init_S(CSOUND *csound, _AAk *p) {
    IGN(csound);
    p->varTypeName = 'S';
    return OK;
}

// -- perlin3, taken verbatim from supercollider's Perlin3 --
// Based on java code by Ken Perlin, published at http://mrl.nyu.edu/~perlin/noise/
static int _p[512], _permutation[256] = {
    151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

static double fade(double t) { return t * t * t * (t * (t * 6. - 15.) + 10.); }
static double lerp(double t, double a, double b) { return a + t * (b - a); }
static double grad(int hash, double x, double y, double z) {
    int h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
    double u = h<8 ? x : y,                 // INTO 12 GRADIENT DIRECTIONS.
        v = h<4 ? y : h==12||h==14 ? x : z;
    return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}

double perlin_noise(double x, double y, double z) {
    // find unit cube that contains point
    int X = (int)x & 255,
        Y = (int)y & 255,
        Z = (int)z & 255;
    x -= floor(x);                                // FIND RELATIVE X,Y,Z
    y -= floor(y);                                // OF POINT IN CUBE.
    z -= floor(z);
    double u = fade(x),                                // COMPUTE FADE CURVES
           v = fade(y),                                // FOR EACH OF X,Y,Z.
           w = fade(z);
    int A  = _p[X  ]+Y,
        AA = _p[A]+Z,
        AB = _p[A+1]+Z,      // HASH COORDINATES OF
        B  = _p[X+1]+Y,
        BA = _p[B]+Z,
        BB = _p[B+1]+Z;      // THE 8 CUBE CORNERS,

    return lerp(w, lerp(v, lerp(u, grad(_p[AA  ], x  , y  , z   ),  // AND ADD
                                grad(_p[BA  ], x-1, y  , z   )), // BLENDED
                        lerp(u, grad(_p[AB  ], x  , y-1, z   ),  // RESULTS
                             grad(_p[BB  ], x-1, y-1, z   ))),// FROM  8
                lerp(v, lerp(u, grad(_p[AA+1], x  , y  , z-1 ),  // CORNERS
                             grad(_p[BA+1], x-1, y  , z-1 )), // OF CUBE
                     lerp(u, grad(_p[AB+1], x  , y-1, z-1 ),
                          grad(_p[BB+1], x-1, y-1, z-1 ))));
}

typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *x, *y, *z;
} PERLIN3;

static void perlin_noise_init(CSOUND *csound) {
    int *loaded = csound->QueryGlobalVariable(csound, "_perlin3_loaded");
    if(loaded != NULL) return;
    csound->CreateGlobalVariable(csound, "_perlin3_loaded", sizeof(int));
    for (int i=0; i < 256 ; i++)
        _p[256+i] = _p[i] = _permutation[i];
}


static int32_t perlin3_init(CSOUND *csound, PERLIN3 *p) {
    perlin_noise_init(csound);
    return OK;
}

static int32_t perlin3_k_kkk(CSOUND *csound, PERLIN3 *p) {
    IGN(csound);
    *p->out = perlin_noise(*p->x, *p->y, *p->z);
    return OK;
}

static int32_t perlin3_a_aaa(CSOUND *csound, PERLIN3 *p) {
    IGN(csound);
    MYFLT *out = p->out;

    SAMPLE_ACCURATE(out);

    MYFLT *x = p->x;
    MYFLT *y = p->y;
    MYFLT *z = p->z;

    for(n=offset; n<nsmps; n++) {
        out[n] = perlin_noise(x[n], y[n], z[n]);
    }

    return OK;
}


typedef struct {
    OPDS h;
    MYFLT *tabnum;
    // sr, nchnls, loopstart=0, basenote=60
    MYFLT *sr;
    MYFLT *nchnls;
    MYFLT *loopstart;
    MYFLT *basenote;

} FTSETPARAMS;

static int32_t ftsetparams(CSOUND *csound, FTSETPARAMS *p) {
    FUNC    *ftp;

    if(*p->nchnls <= 0)
        return INITERRF("Number of channels must be 1 or higher, got %d", (int)*p->nchnls);

    if (UNLIKELY((ftp = csound->FTnp2Finde(csound, p->tabnum)) == NULL))
        return INITERRF("ftsetparams: table %d not found", (int)*p->tabnum);

    if(ftp->flen % (int)*p->nchnls != 0)
        return INITERRF("ftsetparms: the table has a length of %d, which is not divisible"
                        "by the number of channels (%d)", ftp->flen, (int)*p->nchnls);

    ftp->gen01args.sample_rate = *p->sr;
    ftp->nchanls = (int)*p->nchnls;
    MYFLT basenote = *p->basenote;

    if (basenote < 0)
        basenote = FL(60);
    double natcps = POWER(2.0, (basenote - FL(69)) / 12.0) * csound->GetA4(csound);

    if(*p->loopstart > 0) {
        ftp->begin1 = (int)*p->loopstart;
        ftp->loopmode1 = 1;
        ftp->cvtbas = LOFACT * (*p->sr / csound->GetSr(csound));
        ftp->cpscvt = ftp->cvtbas / natcps;
        ftp->end1 = ftp->flenfrms;
    } else {
        // no looping
        ftp->cpscvt = FL(0.0);
        ftp->loopmode1 = 0;
        ftp->loopmode2 = 0;
        ftp->end1 = ftp->flenfrms;
        ftp->begin2 = 0;
        ftp->end2 = 0;
        ftp->cvtbas = LOFACT * 1;
    }
    ftp->soundend = ftp->flen / ftp->nchanls;
    return OK;
}


/*
 * interparr
 *
 * kout  interparr kidx, kxs[], Smode="linear", kparam=0
 *
 * Interpolate between adjacent values of array kxs depending on the
 * index kidx. The fractional part of kidx is used for the interpolation
 * Used together with searchsorted can construct any bpf
 *
 * Smode: one of "linear", "cos", "exp", "smooth", "floor"
 *
 * A normal bpf:
 * ky bpf kx, kxs[], kys[]   -> kidx searchsorted kx, kxs[]
 *                              ky interparr kidx, kys[]
 * Audio bpf
 *
 * ay bpf ax, ixs[], iys[]    -> aidx searchsorted ax, kxs[]
 *                               ay interparr aidx, kys[]
 *
 * A bpf would then be just useful for the case where the pairs are given
 * as arguments to the opcode.
 */

enum InterpMethod {InterpError,
                   InterpLinear,
                   InterpCos,
                   InterpFloor,
                   InterpCubic,
                   InterpExp,
                   InterpSmooth,
                   InterpSmoother
                  };


// t is a value that goes from 0 to 1 to interpolate in a C1 continuous way across
// uniformly sampled data points.
// when t is 0, this will return B.  When t is 1, this will return C.
static inline MYFLT _cubic_interpol(MYFLT t, MYFLT A, MYFLT B, MYFLT C, MYFLT D) {
    MYFLT a = -A/2.0 + (3.0*B)/2.0 - (3.0*C)/2.0 + D/2.0;
    MYFLT b = A - (5.0*B)/2.0 + 2.0*C - D / 2.0;
    MYFLT c = -A/2.0 + C/2.0;
    MYFLT d = B;

    return a*t*t*t + b*t*t + c*t + d;
}

static inline MYFLT _exp_interpol(MYFLT y0, MYFLT y1, MYFLT frac, MYFLT expon) {
    return y0 + (y1 - y0) * POWER(frac, expon);
}

static inline MYFLT _smooth_interpol2(MYFLT y0, MYFLT y1, MYFLT frac, MYFLT param) {
    MYFLT frac2 = frac*frac*(3 - 2*frac);
    if(param > 0) {
        for(int i=0; i<(int)param; i++) {
            frac2 = frac2*frac2*(3 - 2*frac2);
        }
    }
    return y0 + (y1 - y0) * frac2;
}

static inline MYFLT _smooth_interpol(MYFLT y0, MYFLT y1, MYFLT frac, MYFLT param) {
    if(param==0) {
        MYFLT frac2 = frac*frac*(3 - 2*frac);
        return y0 + (y1 - y0) * frac2;
    }
    MYFLT paramint;
    MYFLT paramfrac = modf(param, &paramint);
    MYFLT frac1 = frac*frac*(3-2*frac);
    for(int i=0; i<(int)paramint; i++) {
        frac1 = frac1*frac1*(3-2*frac1);
    }
    if(paramfrac == 0) {
        return y0 + (y1 - y0) * frac1;
    }
    MYFLT frac2 = frac1*frac1*(3 - 2*frac1);
    // MYFLT val0 = y0 + (y1 - y0) * frac1;
    // MYFLT val1 = y0 + (y1 - y0) * frac2;
    // return val0 + (val1-val0)*paramfrac;
    // this is a simplified version of the above:
    MYFLT ydiff = y0-y1;
    return -frac1*ydiff + paramfrac*(frac1 - frac2)*ydiff + y0;
}

static inline MYFLT _smoother_interpol(MYFLT y0, MYFLT y1, MYFLT x, MYFLT param) {
    // perlin's version of smoothstep, corresponds to ~0.7 smoothstep
    MYFLT delta = x * x * x * (x * (x * 6. - 15.) + 10.);
    return y0 + (y1-y0) * delta;
}


typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *idx;
    ARRAYDAT *arr;
    STRINGDAT *mode;
    MYFLT *param;
    enum InterpMethod method;
} INTERPARR_x_xK;


static enum InterpMethod _interp_parse_mode(char *s) {
    if(!strcmp(s, "linear"))
        return InterpLinear;
    else if(!strcmp(s, "cos"))
        return InterpCos;
    else if(!strcmp(s, "floor"))
        return InterpFloor;
    else if(!strcmp(s, "cubic"))
        return InterpCubic;
    else if(!strcmp(s, "exp"))
        return InterpExp;
    else if(!strcmp(s, "smooth"))
        return InterpSmooth;
    else if(!strcmp(s, "smoother"))
        return InterpSmoother;
    return InterpError;
}

static enum InterpMethod _interp_parse_mode_with_param(char *s, MYFLT *param) {
    if(!strcmp(s, "linear"))
        return InterpLinear;
    else if(!strcmp(s, "cos"))
        return InterpCos;
    else if(!strcmp(s, "floor"))
        return InterpFloor;
    else if(!strcmp(s, "cubic"))
        return InterpCubic;
    else if(!strncmp(s, "exp=", 4)) {
        *param = atof(&s[4]);
        return InterpExp;
    }
    else if(!strncmp(s, "smooth=", 7)) {
        *param = atof(&(s[7]));
        return InterpSmooth;
    }
    else if(!strcmp(s, "smooth")) {
        *param = 0;
        return InterpSmooth;
    }
    return InterpError;
}


static int32_t interparr_k_kK_init(CSOUND *csound, INTERPARR_x_xK *p) {
    IGN(csound);
    p->method = p->mode != NULL ? _interp_parse_mode(p->mode->data) : InterpLinear;
    return OK;
}


static int32_t interparr_k_kK_kr(CSOUND *csound, INTERPARR_x_xK *p) {
    IGN(csound);
    MYFLT idx = *p->idx;
    MYFLT intpart, ypre, ypost;
    MYFLT frac = modf(idx, &intpart);
    MYFLT *data = p->arr->data;
    uint64_t len = p->arr->sizes[0];
    if (idx <= 0) {
        *p->out = data[0];
        return OK;
    }
    if (idx >= len - 1) {
        *p->out = data[len - 1];
        return OK;
    }

    uint64_t i = (uint64_t)intpart;

    MYFLT y0 = data[i];
    MYFLT y1 = data[i+1];
    switch(p->method) {
    case InterpExp:
        *p->out = _exp_interpol(y0, y1, frac, *p->param);
        break;
    case InterpLinear:
        *p->out = y0 + (y1 - y0)*frac;
        break;
    case InterpCos:
        *p->out = y0 + (y1-y0) * (1 - COS(frac*PI)) / 2.;
        break;
    case InterpFloor:
        *p->out = y0;
        break;
    case InterpCubic:
        // this only is a true continuous shape is the x coord is periodic!
        ypre = i > 0 ? data[i-1] : y0;
        ypost = len - i > 2 ? data[i+2] : y1;
        *p->out = _cubic_interpol(frac, ypre, y0, y1, ypost);
        break;
    case InterpSmooth:
        *p->out = _smooth_interpol(y0, y1, frac, *p->param);
        break;
    case InterpSmoother:
        *p->out = _smoother_interpol(y0, y1, frac, *p->param);
        break;
    default:
        *p->out = 0;
    }
    return OK;
}

static int32_t interparr_k_kK_ir(CSOUND *csound, INTERPARR_x_xK *p) {
    interparr_k_kK_init(csound, p);
    return interparr_k_kK_kr(csound, p);
}


static int32_t interparr_a_aK_kr(CSOUND *csound, INTERPARR_x_xK *p) {
    MYFLT *out = p->out;
    MYFLT *in = p->idx;
    MYFLT *data = p->arr->data;
    MYFLT frac, intpart, idx;
    uint64_t len = p->arr->sizes[0];

    AUDIO_OPCODE(csound, p);
    AUDIO_OUTPUT(out);
    int method = p->method;
    MYFLT param = *p->param;
    for(n=offset; n<nsmps; n++) {
        idx = in[n];
        intpart = modf(idx, &frac);
        if (idx <= 0) {
            out[n] = data[0];
            return OK;
        }
        if (idx >= len - 1) {
            out[n] = data[len - 1];
            return OK;
        }

        uint64_t i = (uint64_t)intpart;

        MYFLT y0 = data[i];
        MYFLT y1 = data[i+1];
        switch(method) {
        case InterpLinear:
            out[n] = y0 + (y1 - y0)*frac;
            break;
        case InterpCos:
            out[n] = y0 + (y1-y0) * (1 - COS(frac*PI)) / 2.;
            break;
        case InterpFloor:
            out[n] = y0;
            break;
        case InterpSmooth:
            out[n] = _smooth_interpol(y0, y1, frac, param);
            break;
        case InterpSmoother:
            out[n] = _smoother_interpol(y0, y1, frac, param);
            break;
        case InterpExp:
            out[n] = _exp_interpol(y0, y1, frac, param);
            break;
        case InterpCubic:
            out[n] = 0;
            break;
        }
    }
    return OK;
}

typedef struct {
    OPDS h;
    ARRAYDAT *out;
    ARRAYDAT *idx;
    ARRAYDAT *arr;
    STRINGDAT *mode;
    MYFLT *param;
    enum InterpMethod method;
} INTERPARR_K_KK;

static int32_t interparr_K_KK_init(CSOUND *csound, INTERPARR_K_KK *p) {
    p->method = p->mode != NULL ? _interp_parse_mode(p->mode->data) : InterpLinear;
    if(p->idx->dimensions > 1)
        return INITERR("idx array should be 1D");
    if(p->arr->dimensions > 1)
        return INITERR("data array should be 1D");
    tabinit(csound, p->out, p->idx->sizes[0]);
    return OK;
}

static int32_t interparr_K_KK_init_simple(CSOUND *csound, INTERPARR_K_KK *p) {
    p->method = InterpLinear;
    if(p->idx->dimensions > 1)
        return INITERR("idx array should be 1D");
    if(p->arr->dimensions > 1)
        return INITERR("data array should be 1D");
    tabinit(csound, p->out, p->idx->sizes[0]);
    return OK;
}



static int32_t interparr_K_KK_kr(CSOUND *csound, INTERPARR_K_KK *p) {
    MYFLT *out = p->out->data;
    MYFLT *in = p->idx->data;
    MYFLT *data = p->arr->data;
    MYFLT frac, intpart, idx, ypre, ypost;
    size_t len = p->arr->sizes[0];
    size_t numitems = p->idx->sizes[0];
    tabcheck(csound, p->out, numitems, &(p->h));
    int method = p->method;
    MYFLT param = *p->param;
    MYFLT data0 = data[0];
    MYFLT data1 = data[len-1];
    for(size_t n=0; n < numitems; n++) {
        idx = in[n];
        frac = modf(idx, &intpart);
        if (idx <= 0) {
            out[n] = data0;
            return OK;
        }
        if (idx >= len - 1) {
            out[n] = data1;
            return OK;
        }

        size_t i = (uint64_t)intpart;

        MYFLT y0 = data[i];
        MYFLT y1 = data[i+1];
        switch (method) {
        case InterpLinear:
            out[n] = y0 + (y1 - y0)*frac;
            break;
        case InterpCos:
            out[n] = y0 + (y1-y0) * (1 - COS(frac*PI)) / 2.;
            break;
        case InterpFloor:
            out[n] = y0;
            break;
        case InterpCubic:
            ypre = i > 0 ? data[i-1] : y0;
            ypost = len - i > 3 ? data[i+2] : y1;
            out[n] = _cubic_interpol(frac, ypre, y0, y1, ypost);
            break;
        case InterpExp:
            out[n] = _exp_interpol(y0, y1, frac, param);
            break;
        case InterpSmooth:
            out[n] = _smooth_interpol(y0, y1, frac, param);
            break;
        case InterpSmoother:
            out[n] = _smoother_interpol(y0, y1, frac, param);
            break;
        default:
            out[n] = 0;
        }
    }
    return OK;
}

static int32_t interparr_K_KK_ir(CSOUND *csound, INTERPARR_K_KK *p) {
    p->method = InterpLinear;
    if(p->idx->dimensions > 1)
        return INITERR("idx array should be 1D");
    if(p->arr->dimensions > 1)
        return INITERR("data array should be 1D");
    tabinit(csound, p->out, p->idx->sizes[0]);
    return interparr_K_KK_kr(csound, p);
}


typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *idx, *tabnum;
    STRINGDAT *mode;
    MYFLT *step, *offset;

    FUNC *ftp;
    int lasttab;
    MYFLT param;
    int numargs;
    enum InterpMethod method;
} INTERPTAB;


static int32_t interptab_init_kk(CSOUND *csound, INTERPTAB *p) {
    FUNC *ftp;
    ftp = csound->FTnp2Find(csound, p->tabnum);
    if (UNLIKELY(ftp == NULL)) {
        MSGF("table %d not found", (int)*p->tabnum);
        return NOTOK;
    }
    p->ftp = ftp;
    p->lasttab = (int)*p->tabnum;
    p->method = InterpLinear;
    p->numargs = 2;
    return OK;
}

static int32_t interptab_init_kkSkk(CSOUND *csound, INTERPTAB *p) {
    FUNC *ftp;
    ftp = csound->FTnp2Find(csound, p->tabnum);
    if (UNLIKELY(ftp == NULL)) {
        MSGF("table %d not found", (int)*p->tabnum);
        return NOTOK;
    }
    p->ftp = ftp;
    p->lasttab = (int)*p->tabnum;
    if(*p->step <= 0)
        *p->step = 1;
    p->numargs = 5;
    p->method = _interp_parse_mode_with_param(p->mode->data, &(p->param));
    return OK;
}

static int32_t interptab_kr(CSOUND *csound, INTERPTAB *p) {
    IGN(csound);
    MYFLT idx = *p->idx;
    MYFLT *data = p->ftp->ftable;
    int32_t step, taboffset;

    if(p->numargs == 2) {
        step = 1;
        taboffset = 0;
    } else {
        step = (int32_t)*p->step;
        taboffset = (int32_t)*p->offset;
    }

    if (idx <= 0) {
        *p->out = data[taboffset];
        return OK;
    }

    MYFLT intpart;
    MYFLT frac = modf(idx, &intpart);

    uint64_t len = p->ftp->flen;
    uint64_t i = (uint64_t)intpart * step + taboffset;

    if (i >= len - 1) {
        *p->out = data[len - step + taboffset];
        return OK;
    }

    if (frac == 0) {
        *p->out = data[i];
        return OK;
    }
    MYFLT y0 = data[i];
    MYFLT y1 = data[i+step];
    MYFLT ypre, ypost;
    switch(p->method) {
    case InterpLinear:
        *p->out = y0 + (y1 - y0)*frac;
        break;
    case InterpCos:
        *p->out = y0 + (y1-y0) * (1 - COS(frac*PI)) / 2.;
        break;
    case InterpFloor:
        *p->out = y0;
        break;
    case InterpCubic:
        ypre = i > 0 ? data[i-1] : y0;
        ypost = len - i > 3 ? data[i+2] : y1;
        *p->out = _cubic_interpol(frac, ypre, y0, y1, ypost);
        break;
    case InterpSmooth:
        *p->out = _smooth_interpol(y0, y1, frac, p->param);
        break;
    case InterpSmoother:
        *p->out = _smoother_interpol(y0, y1, frac, p->param);
        break;
    case InterpExp:
        *p->out = _exp_interpol(y0, y1, frac, p->param);
        break;
    default:
        MSGF("Invalid interpolation mode: %s\n", p->mode->data);
        *p->out = 0;
        return NOTOK;
    }
    return OK;
}

static int32_t interptab_ir5(CSOUND *csound, INTERPTAB *p) {
    if(interptab_init_kkSkk(csound, p) == NOTOK)
        return NOTOK;
    return interptab_kr(csound, p);
}

static int32_t interptab_ir2(CSOUND *csound, INTERPTAB *p) {
    if(interptab_init_kk(csound, p) == NOTOK)
        return NOTOK;
    return interptab_kr(csound, p);
}

static int32_t interptab_a_a_kr(CSOUND *csound, INTERPTAB *p) {
    IGN(csound);
    if((int)*p->tabnum != p->lasttab) {
        FUNC *ftp = csound->FTnp2Find(csound, p->tabnum);
        if (UNLIKELY(ftp == NULL)) {
            MSGF("table %d not found", (int)*p->tabnum);
            return NOTOK;
        }
        p->ftp = ftp;
        p->lasttab = (int)*p->tabnum;
    }
    MYFLT *out = p->out;
    MYFLT *in = p->idx;
    uint64_t taboffset = (uint64_t)*p->offset;
    uint64_t step = (uint64_t)*p->step;
    if(step <= 0) {
        return PERFERRF("step cannot be less than 1, got %lu", step);
    }

    MYFLT *data = &(p->ftp->ftable[taboffset]);
    MYFLT idx, intpart, frac, y0, y1, ypre, ypost;
    uint64_t len = p->ftp->flen;

    AUDIO_OPCODE(csound, p);
    AUDIO_OUTPUT(out);

    // we branch outside the loop depending on the interpolation mode
    MYFLT firstelem = data[0];
    MYFLT lastelem = data[len - step];
    MYFLT param = p->param;
    switch(p->method) {
    case InterpLinear:
        for(n=offset; n < nsmps; n++) {
            idx = in[n];
            frac = modf(idx, &intpart);
            uint64_t i = (uint64_t)intpart * step;
            if(i <= 0)
                out[n] = firstelem;
            else if(i>= len - 1)
                out[n] = lastelem;
            else if(frac == 0)
                out[n] = data[i];
            else {
                y0 = data[i];
                y1 = data[i+step];
                out[n] = y0 + (y1 - y0)*frac;
            }
        }
        break;
    case InterpCos:
        for(n=offset; n < nsmps; n++) {
            idx = in[n];
            frac = modf(idx, &intpart);
            uint64_t i = (uint64_t)intpart * step;
            if(i <= 0)
                out[n] = firstelem;
            else if(i>= len - 1)
                out[n] = lastelem;
            else if(frac == 0)
                out[n] = data[i];
            else {
                y0 = data[i];
                y1 = data[i+step];
                out[n] = y0 + (y1-y0) * (1 - COS(frac*PI)) / 2.;
            }
        }
        break;
    case InterpExp:
        for(n=offset; n < nsmps; n++) {
            idx = in[n];
            frac = modf(idx, &intpart);
            uint64_t i = (uint64_t)intpart * step ;
            if(i <= 0)
                out[n] = firstelem;
            else if(i>= len - 1)
                out[n] = lastelem;
            else if(frac == 0)
                out[n] = data[i];
            else {
                y0 = data[i];
                y1 = data[i+step];
                out[n] = _exp_interpol(y0, y1, frac, param);
            }
        }
        break;
    case InterpFloor:
        for(n=offset; n < nsmps; n++) {
            idx = in[n];
            frac = modf(idx, &intpart);
            uint64_t i = (uint64_t)intpart * step;
            if(i <= 0)
                out[n] = firstelem;
            else if(i>= len - 1)
                out[n] = lastelem;
            else
                out[n] = data[i];
        }
        break;
    case InterpCubic:
        for(n=offset; n < nsmps; n++) {
            idx = in[n];
            frac = modf(idx, &intpart);
            uint64_t i = (uint64_t)intpart * step;
            if(i <= 0)
                out[n] = firstelem;
            else if(i>= len - 1)
                out[n] = lastelem;
            else if(frac == 0)
                out[n] = data[i];
            else {
                y0 = data[i];
                y1 = data[i+step];
                ypre = i >= step ? data[i-step] : y0;
                ypost = len - i > step*2 ? data[i+step+step] : y1;
                out[n] = _cubic_interpol(frac, ypre, y0, y1, ypost);
            }
        }
        break;
    case InterpSmooth:
        for(n=offset; n < nsmps; n++) {
            idx = in[n];
            frac = modf(idx, &intpart);
            uint64_t i = (uint64_t)intpart * step;
            if(i <= 0)
                out[n] = firstelem;
            else if(i>= len - 1)
                out[n] = lastelem;
            else {
                y0 = data[i];
                y1 = data[i+step];
                out[n] = _smooth_interpol(y0, y1, frac, param);
            }
        }
        break;
    case InterpSmoother:
        for(n=offset; n < nsmps; n++) {
            idx = in[n];
            frac = modf(idx, &intpart);
            uint64_t i = (uint64_t)intpart * step;
            if(i <= 0)
                out[n] = firstelem;
            else if(i>= len - 1)
                out[n] = lastelem;
            else {
                y0 = data[i];
                y1 = data[i+step];
                out[n] = _smoother_interpol(y0, y1, frac, param);
            }
        }
        break;
    default:
        return PERFERRF("Invalid interpolation method %s", p->mode->data);
    }
    return OK;
}


/*
 * bisect
 * bisect
 *
 * Returns the idx where kx would fit inside karray, with a fractional
 * part indicating the distance between the adjacent items. Together
 * with interparr this can be used to construct piecewise interpolation
 * curves (bpf) in many different configurations
 *
 * kidx bisect kx, karray[]
 * aidx bisect ax, karray[]
 * idx  bisect ix, iarray[]
 * kidx[] bisect kxs[], karray[]
 * idx[]  bisect ixs[], iarray[]
 */


typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *x;
    ARRAYDAT *arr;
    int64_t lastidx;
} BISECT;


static inline int64_t array_bisect_multidim(MYFLT x, MYFLT *xs, int64_t len,
                                            int step, int offset, int64_t lastidx) {
    // step: the frame size, a.k.a the number of columns per row. Must be >= 1
    // offset: the column index to use for comparison
    // returns: the fractional row index
    if(x <= xs[offset]) {
        return -1;
    }
    if(x >= xs[len-step+offset]) {
        return -2;
    }

    if(lastidx >= 0 &&
            lastidx < len-step*2 &&
            xs[lastidx*step+offset] <= x && x < xs[(lastidx+1)*step+offset]) {
        return lastidx;
    }

    int64_t numframes = (int64_t)ceil((len-offset)/step);
    int64_t imin = 0;
    int64_t imax = numframes;
    int64_t imid;

    while (imin < imax) {
        imid = (imax + imin) / 2;
        if (xs[imid*step+offset] < x)
            imin = imid + 1;
        else
            imax = imid;
    }
    // now the right item is in pairmin
    return imin - 1;
}

static inline int64_t array_bisect(MYFLT x, MYFLT *xs, int64_t xslen, int64_t lastidx) {
    // -1: lower bound, -2: upper bound
    if(x <= xs[0]) {
        return -1;
    }

    if(x >= xs[xslen-1]) {
        return -2;
    }
    
    int64_t imin;
    
    if(lastidx >= 0 && xs[lastidx] <= x) {
        if(x < xs[lastidx+1]) {
            return lastidx;
        }
        if (lastidx < xslen-2 && x < xs[lastidx+2]) {
            return lastidx + 1;
        }
        imin = lastidx;      
    }
    else {
        imin = 0;       
    }
    int64_t imax = xslen;

    int64_t imid;
    while (imin < imax) {
        imid = (imax + imin) / 2;
        if (xs[imid] < x)
            imin = imid + 1;
        else
            imax = imid;
    }

    // now the right pair is in pairmin
    return imin - 1;
}


static int32_t bisect_init(CSOUND *csound, BISECT *p) {
    IGN(csound);
    p->lastidx = -1;
    return OK;
}

static int32_t bisect_kr(CSOUND *csound, BISECT *p) {
    IGN(csound);
    int64_t lenarr = p->arr->sizes[0];
    MYFLT *arr = p->arr->data;
    MYFLT x = *p->x;
    MYFLT x0, x1, frac;

    int64_t idx = array_bisect(x, arr, lenarr, p->lastidx);

    if(idx == -1) {
        *p->out = 0;
        p->lastidx = -1;
        return OK;
    }

    if(idx == -2) {
        *p->out = lenarr - 1;
        p->lastidx = -1;
        return OK;
    }

    x0 = arr[idx];
    x1 = arr[idx+1];
    frac = (x - x0) / (x1 - x0);
    *p->out = (MYFLT)idx + frac;
    p->lastidx = idx;
    return OK;
}

static int32_t bisect_ir(CSOUND *csound, BISECT *p) {
    bisect_init(csound, p);
    return bisect_kr(csound, p);
}

static int32_t bisect_a_a_kr(CSOUND *csound, BISECT *p) {
    IGN(csound);
    int64_t lenarr = p->arr->sizes[0];
    MYFLT *arr = p->arr->data;
    MYFLT *out = p->out;
    MYFLT *in = p->x;
    MYFLT x0, x1, frac, x;
    int64_t idx, lastidx = p->lastidx;
    AUDIO_OPCODE(csound, p);
    AUDIO_OUTPUT(out);

    for(n=offset; n<nsmps; n++) {
        x = in[n];
        idx = array_bisect(x, arr, lenarr, lastidx);
        if(idx == -1) {
            out[n] = 0;
            lastidx = -1;
        } else if(idx == -2) {
            out[n] = lenarr - 1;
            lastidx = -1;
        } else {
            x0 = arr[idx];
            x1 = arr[idx+1];
            frac = (x - x0) / (x1 - x0);
            out[n] = (MYFLT)idx + frac;
            lastidx = idx;
        }
    }
    p->lastidx = lastidx;
    return OK;
}

typedef struct {
    OPDS h;
    ARRAYDAT *out;
    ARRAYDAT *xs;
    ARRAYDAT *arr;
    int64_t lastidx;
} BISECTARR;

static int32_t bisectarr_init(CSOUND *csound, BISECTARR *p) {
    IGN(csound);
    p->lastidx = -1;
    tabinit(csound, p->out, p->xs->sizes[0]);
    return OK;
}

static int32_t bisectarr_kr(CSOUND *csound, BISECTARR *p) {
    IGN(csound);
    MYFLT *out = p->out->data;
    MYFLT *in = p->xs->data;
    MYFLT *arr = p->arr->data;

    size_t lenarr = p->arr->sizes[0];
    size_t numitems = p->xs->sizes[0];
    tabcheck(csound, p->out, numitems, &(p->h));

    MYFLT x0, x1, frac, x;
    int64_t idx, lastidx = p->lastidx;

    for(size_t n=0; n<numitems; n++) {
        x = in[n];
        idx = array_bisect(x, arr, lenarr, lastidx);
        if(idx == -1) {
            out[n] = 0;
            lastidx = -1;
        } else if(idx == -2) {
            out[n] = lenarr - 1;
            lastidx = -1;
        } else {
            x0 = arr[idx];
            x1 = arr[idx+1];
            frac = (x - x0) / (x1 - x0);
            out[n] = (MYFLT)idx + frac;
            lastidx = idx;
        }
    }
    p->lastidx = lastidx;
    return OK;
}

static int32_t bisectarr_ir(CSOUND *csound, BISECTARR *p) {
    bisectarr_init(csound, p);
    return bisectarr_kr(csound, p);
}

// kidx bisect kx, ktab, kstep=1, koffset=0
// aidx bisect ax, ktab, kstep=1, koffset=0
// kidx[] bisect kx[], ktab, kstep=1, koffset=0

typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *in, *tabnum, *step, *offset;

    FUNC *ftp;
    int64_t lastidx;
    int lasttab;
} BISECTTAB;

static int32_t bisecttab_init(CSOUND *csound, BISECTTAB *p) {
    FUNC *ftp;
    ftp = csound->FTnp2Find(csound, p->tabnum);
    if (UNLIKELY(ftp == NULL)) {
        MSGF("table %d not found", (int)*p->tabnum);
        return NOTOK;
    }
    p->ftp = ftp;
    p->lastidx = -1;
    p->lasttab = (int)*p->tabnum;
    return OK;
}

static int32_t bisecttab_k_k_kr(CSOUND *csound, BISECTTAB *p) {
    IGN(csound);
    if((int)*p->tabnum != p->lasttab) {
        FUNC *ftp = csound->FTnp2Find(csound, p->tabnum);
        if (UNLIKELY(ftp == NULL)) {
            MSGF("table %d not found", (int)*p->tabnum);
            return NOTOK;
        }
        p->ftp = ftp;
        p->lasttab = (int)*p->tabnum;
    }
    MYFLT *data = p->ftp->ftable;
    MYFLT x = *p->in;
    int64_t lendata = p->ftp->flen;
    MYFLT x0, x1, frac;
    int32_t taboffset = (int32_t)(*p->offset);
    int32_t step = (int32_t)(*p->step);
    if(step == 0)
        step = 1;
    else if(step < 0)
        return PERFERRF("step cannot be negative, got %d", step);
    int64_t row = array_bisect_multidim(x, data, lendata, step, taboffset, p->lastidx);

    if(row == -1) {
        *p->out = 0;
        p->lastidx = -1;
    } else if (row == -2) {
        *p->out = ceil((lendata-taboffset)/step)-1;
        p->lastidx = -1;
    } else {
        x0 = data[taboffset+row*step];
        x1 = data[taboffset+(row+1)*step];
        frac = (x - x0) / (x1 - x0);
        *p->out = (MYFLT)row + frac;
        p->lastidx = row;
    }
    return OK;
}

static int32_t bisecttab_k_k_ir(CSOUND *csound, BISECTTAB *p) {
    int res = bisecttab_init(csound, p);
    if (res == NOTOK)
        return NOTOK;
    return bisecttab_k_k_kr(csound, p);
}

static int32_t bisecttab_a_a_kr(CSOUND *csound, BISECTTAB *p) {
    IGN(csound);
    if((int)*p->tabnum != p->lasttab) {
        FUNC *ftp = csound->FTnp2Find(csound, p->tabnum);
        if (UNLIKELY(ftp == NULL)) {
            MSGF("table %d not found", (int)*p->tabnum);
            return NOTOK;
        }
        p->ftp = ftp;
        p->lasttab = (int)*p->tabnum;
    }
    MYFLT *out = p->out;
    MYFLT *in = p->in;
    int32_t taboffset = (int32_t)*p->offset;
    int32_t step = (int32_t)*p->step;
    if(step <= 0) {
        return PERFERRF("step cannot be less than 1, got %d", step);
    } else if(step == 0) {
        step = 1;
    }

    MYFLT *data = p->ftp->ftable;

    AUDIO_OPCODE(csound, p);
    AUDIO_OUTPUT(out);

    int64_t idx,
            lendata = p->ftp->flen,
            lastidx = p->lastidx;

    MYFLT x0, x1, frac, x;
    MYFLT rightmost = ceil((lendata-taboffset)/step)-1;
    for(n=offset; n < nsmps; n++) {
        x = in[n];
        idx = array_bisect_multidim(x, data, lendata, step, taboffset, lastidx);
        if(idx == -1) {
            out[n] = 0;
            lastidx = -1;
        } else if(idx == -2) {
            out[n] = rightmost;
            lastidx = -1;
        } else {
            int64_t idx0 = taboffset + idx*step;
            x0 = data[idx0];
            x1 = data[idx0+step];
            frac = (x - x0) / (x1 - x0);
            out[n] = (MYFLT)idx + frac;
            lastidx = idx;
        }
    }
    return OK;
}

typedef struct {
    OPDS h;
    ARRAYDAT *out;
    ARRAYDAT *in;
    MYFLT *tabnum, *step, *offset;

    FUNC *ftp;
    int64_t lastidx;
    int lasttab;
} BISECTTAB_ARR;

static int32_t bisecttabarr_init(CSOUND *csound, BISECTTAB_ARR *p) {
    FUNC *ftp;
    ftp = csound->FTnp2Find(csound, p->tabnum);
    if (UNLIKELY(ftp == NULL)) {
        MSGF("table %d not found", (int)*p->tabnum);
        return NOTOK;
    }
    p->ftp = ftp;
    p->lastidx = -1;
    p->lasttab = (int)*p->tabnum;
    if(*p->step <= 0)
        *p->step = 1;
    tabinit(csound, p->out, p->in->sizes[0]);
    return OK;
}

static int32_t bisecttabarr_kr(CSOUND *csound, BISECTTAB_ARR *p) {
    IGN(csound);
    if((int)*p->tabnum != p->lasttab) {
        FUNC *ftp = csound->FTnp2Find(csound, p->tabnum);
        if (UNLIKELY(ftp == NULL)) {
            MSGF("table %d not found", (int)*p->tabnum);
            return NOTOK;
        }
        p->ftp = ftp;
        p->lasttab = (int)*p->tabnum;
    }
    MYFLT *data = p->ftp->ftable;

    MYFLT *out = p->out->data;
    MYFLT *in = p->in->data;
    int32_t taboffset = (int32_t)*p->offset;
    int32_t step = (int32_t)*p->step;
    size_t arrsize = p->in->sizes[0];
    tabcheck(csound, p->out, arrsize, &(p->h));
    if(step < 0) {
        MSGF("step cannot be negative, got %d", step);
        return NOTOK;
    }
    if(step == 0)
        step = 1;

    int64_t idx,
            lendata = p->ftp->flen,
            lastidx = p->lastidx;

    MYFLT x0, x1, frac, x;
    for(size_t n=0; n < arrsize; n++) {
        x = in[n];
        idx = array_bisect_multidim(x, data, lendata, step, taboffset, lastidx);
        if(idx == -1) {
            out[n] = 0;
            lastidx = -1;
        } else if(idx == -2) {
            out[n] = lendata - step + taboffset;
            lastidx = -1;
        } else {
            x0 = data[taboffset + idx*step];
            x1 = data[taboffset + idx*step + 1];
            frac = (x - x0) / (x1 - x0);
            out[n] = (MYFLT)idx + frac;
            lastidx = idx;
        }
    }
    p->lastidx = lastidx;
    return OK;
}

static int32_t bisecttabarr_ir(CSOUND *csound, BISECTTAB_ARR *p) {
    bisecttabarr_init(csound, p);
    return bisecttabarr_kr(csound, p);
}

typedef struct {
    OPDS h;
    MYFLT *out;
    // MYFLT *tabnum;
    MYFLT *pargs [VARGMAX-5];
} FILLTAB;

// itab filltab 100, 0, 1, 2, 3, 4
// creates a table with given number (0=let csound make the number) and fill
// it with the pargs following. This is the same as ftgen 0, 0, 0, -2, ...
static int32_t filltab(CSOUND *csound, FILLTAB *p) {
    MYFLT   *fp;
    FUNC    *ftp;
    EVTBLK  *ftevt;

    ftevt =(EVTBLK*) csound->Malloc(csound, sizeof(EVTBLK));
    ftevt->opcod = 'f';
    ftevt->strarg = NULL;
    fp = &ftevt->p[0];
    size_t n = csound->GetInputArgCnt(p);
    size_t tabsize = n;
    fp[0] = FL(0.0);
    fp[1] = 0; // *p->tabnum;                                 /* copy p1 - p5 */
    fp[2] = ftevt->p2orig = FL(0.0);                    /* force time 0 */
    fp[3] = ftevt->p3orig = tabsize;
    fp[4] = 2;
    fp[5] = 0;
    ftevt->pcnt = (int16) 6;
    n = csound->hfgens(csound, &ftp, ftevt, 1);         /* call the fgen */
    csound->Free(csound, ftevt);
    if (UNLIKELY(n != 0) || ftp == NULL)
        return INITERR("ftgen error");
    MYFLT **pargs = p->pargs;
    MYFLT *ftable = ftp->ftable;
    for(size_t i=0; i<tabsize; i++) {
        ftable[i] = *pargs[i];
    }
    *p->out = (MYFLT) ftp->fno;                      /* record the fno */
    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *size;
    MYFLT *defaultvalue;
} FTNEW;

static int32_t ftnew(CSOUND *csound, FTNEW *p) {
    MYFLT   *fp;
    FUNC    *ftp;
    EVTBLK  *ftevt;

    ftevt =(EVTBLK*) csound->Malloc(csound, sizeof(EVTBLK));
    ftevt->opcod = 'f';
    ftevt->strarg = NULL;
    fp = &ftevt->p[0];
    size_t tabsize = (size_t)*p->size;
    fp[0] = FL(0.0);
    fp[1] = FL(0.0);  // *p->tabnum;
    fp[2] = ftevt->p2orig = FL(0.0);                    /* force time 0 */
    fp[3] = ftevt->p3orig = tabsize;
    fp[4] = 2;
    fp[5] = 0;
    ftevt->pcnt = (int16) 6;
    int n = csound->hfgens(csound, &ftp, ftevt, 1);         /* call the fgen */
    csound->Free(csound, ftevt);
    if (UNLIKELY(n != 0) || ftp == NULL)
        return INITERR("ftgen error");
    if(*p->defaultvalue != 0) {
        MYFLT v = *p->defaultvalue;
        MYFLT *ftable = ftp->ftable;
        for(size_t i=0; i<tabsize; i++) {
            ftable[i] = v;
        }
    }
    *p->out = (MYFLT) ftp->fno;                      /* record the fno */
    return OK;
}


typedef struct {
    OPDS h;
    ARRAYDAT *arr;
    ARRAYDAT *mask;
} ZEROARR;

typedef struct {
    OPDS h;
    ARRAYDAT *arr;
    MYFLT *masktab;
    FUNC *ftp;
} ZEROARR_TAB;


static int32_t zeroarr_perf(CSOUND *csound, ZEROARR *p) {
    MYFLT *data = p->arr->data;
    // size_t numbytes = p->arr->allocated;
    // TODO: zero only the used part of an array
    // The size might change between cycles
    size_t size = 1;
    for(int dim=0; dim<p->arr->dimensions; dim++) {
        size *= p->arr->sizes[dim] * size;
    }
    size_t numbytes = size * p->arr->arrayMemberSize;
    memset(data, 0, numbytes);
    return OK;
}

void inline _zeroarr_masked(CSOUND *csound, ARRAYDAT *arr, MYFLT *mask) {
    // This is only for 1D audio arrays
    // For audio arrays arrayMemberSize holds the number of bytes of an audio signal,
    // that is ksmps * sizeof(MYFLT)
    size_t arrsize = arr->sizes[0];
    int ksmpsbytes = arr->arrayMemberSize;
    MYFLT *arrdata = arr->data;
    for(size_t i=0; i < arrsize; i++) {
        if(mask[i] > 0) {
            memset((char *)(arrdata) + i*ksmpsbytes, 0, ksmpsbytes);
        }
    }

}

static int32_t zeroarr_masked_perf(CSOUND *csound, ZEROARR *p) {
    if(p->arr->dimensions != 1) {
        return PERFERR("Only 1D audio arrays supported");
    }
    if(p->arr->sizes[0] > p->mask->sizes[0]) {
        PERFERRF("The mask is too small (mask size=%d, array size=%d)", p->mask->sizes[0], p->arr->sizes[0]);
        return NOTOK;
    }
    MYFLT *maskdata = p->mask->data;
    _zeroarr_masked(csound, p->arr, maskdata);
    return OK;
}

static int32_t zeroarr_maskedtab_init(CSOUND *csound, ZEROARR_TAB *p) {
    FUNC *ftp = csound->FTnp2Find(csound, p->masktab);
    if (UNLIKELY(ftp == NULL))
        return NOTOK;
    p->ftp = ftp;
    return OK;
}

static int32_t zeroarr_maskedtab_perf(CSOUND *csound, ZEROARR_TAB *p) {
    if(p->arr->dimensions != 1) {
        return PERFERR("Only 1D audio arrays supported");
    }
    int tabsize = p->ftp->flen;
    if(p->arr->sizes[0] > tabsize) {
        PERFERRF("The mask is too small (mask size=%d, array size=%d)", tabsize, p->arr->sizes[0]);
        return NOTOK;
    }
    _zeroarr_masked(csound, p->arr, p->ftp->ftable);
    return OK;
}

typedef struct {
    OPDS h;
    ARRAYDAT *arr;
    MYFLT *idx;
    MYFLT *insig;
} MIXARRAY1;

// Mix an audio source with an element of an audio array
// mixarray ga_dest, kidx, asource
// this is the same as: ga_dest[kidx] = ga_dest[kidx] + asource
static int32_t mixarray_perf(CSOUND *csound, MIXARRAY1 *p) {
    int idx = (int)*p->idx;
    if(idx > p->arr->sizes[0]) {
        PERFERRF("Index %d out of bounds (len=%d)", idx, p->arr->sizes[0]);
        return NOTOK;
    }
    int ksmps = p->h.insdshead->ksmps;
    size_t samp0 = idx * ksmps;
    MYFLT *dest = &(p->arr->data[samp0]);
    MYFLT *source = p->insig;
    for(int i=0; i<ksmps; i++ ) {
        dest[i] += source[i];
    }
    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *outidx;
    MYFLT *ftnum;
    MYFLT *x;
    MYFLT *tolerance;
} FTINDEX;

static int32_t ftindex_perf(CSOUND *csound, FTINDEX *p) {
    MYFLT x = *p->x;
    MYFLT tolerance = *p->tolerance;
    FUNC *ftp = csound->FTnp2Find(csound, p->ftnum);
    if (UNLIKELY(ftp == NULL)) {
        MSGF("table %d not found", (int)*p->ftnum);
        return NOTOK;
    }
    if(tolerance <= 0) {
        tolerance = 1e-12;
    }
    MYFLT *data = ftp->ftable;
    size_t len = ftp->flen;
    for(size_t i=0; i<len; i++) {
        if(fabs(data[i] - x) < tolerance ) {
            *p->outidx = i;
            return OK;
        }
    }
    *p->outidx = -1.0;
    return OK;
}


typedef struct {
    OPDS h;
    MYFLT *outidx;
    ARRAYDAT *arr;
    MYFLT *x;
    MYFLT *tolerance;
} FINDARR;

static int32_t findarr_perf(CSOUND *csound, FINDARR *p) {
    // p->h.insdshead->instr->psetdata
    MYFLT *data = p->arr->data;
    MYFLT x = *p->x;
    MYFLT tolerance = *p->tolerance;
    if(tolerance <= 0) {
        tolerance = 1e-12;
    }
    for(size_t i=0; i<p->arr->sizes[0]; i++) {
        if(fabs(data[i] - x) < tolerance ) {
            *p->outidx = (MYFLT)i;
            return OK;
        }
    }
    *p->outidx = -1.0;
    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *outidx;
    ARRAYDAT *arr;
    STRINGDAT *val;
} FINDARR_S;


static int32_t findarr_s(CSOUND *csound, FINDARR_S *p) {
    STRINGDAT *data = (STRINGDAT *)p->arr->data;
    char *val = p->val->data;
    int32_t vallen = strlen(val);
    for(int32_t i=0; i<p->arr->sizes[0]; i++) {
        if(data[i].size >= vallen && strcmp(val, data[i].data) == 0) {
            *p->outidx = (MYFLT)i;
            return OK;
        }
    }
    *p->outidx = -1.0;
    return OK;
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// iarr[] loadnpy "foo.npy"
typedef struct {
    OPDS h;
    ARRAYDAT *out;
    STRINGDAT *path;
} loadnpy_ARR;


int64_t strfind(char *s, char *subs) {
    char *s2 = strstr(s, subs);
    if(s2 != NULL) {
        return s2 - s;
    }
    return -1;
}

typedef struct {
    size_t word_size;
    int numdimensions;
    int sizes[8];
    int is_fortran_order;
    int is_little_endian;
    char type;
} npy_header;

// returns 0 if ok, 1 if error
int _parse_npy_header(FILE* fp, npy_header *h) {
    //std::string magic_string(buffer,6);
    char buffer[256];
    size_t res = fread(buffer, sizeof(char), 11, fp);
    if(res != 11)
        return 1;
    char *header = fgets(buffer, 256, fp);
    int64_t loc1, loc2;
    //fortran order
    // loc1 = header.find("fortran_order")+16;
    loc1 = strfind(header, "fortran_order")+16;
    // fortran_order = (header.substr(loc1,4) == "True" ? true : false);
    h->is_fortran_order = strncmp(header+loc1, "True", 4) == 0;
    //shape: "shape": (4, 5 [,...])
    loc1 = strfind(header, "(");
    loc2 = strfind(header, ")");
    int numdims = 0;
    char *s = header + loc1 + 1;
    char *send = header + loc2;
    char *next = NULL;
    for (;;) {
        int64_t num = strtol(s, &next, 10);
        if (s==send)
            break;
        h->sizes[numdims] = (int)num;
        numdims += 1;
        if(numdims > 8) {
            return 1;
        }
        s = next;
        // skip comma
        for(; (s < send) & (*s == ','); s++);
    }
    h->numdimensions = numdims;
    //endian, word size, data type
    //byte order code | stands for not applicable.
    //not sure when this applies except for byte array
    loc1 = strfind(header, "descr")+9;
    h->is_little_endian = (header[loc1] == '<' || header[loc1] == '|' ? 1 : 0);
    h->type = header[loc1+1]; // 'i', 'f';
    h->word_size = strtol(header+loc1+2, &next, 10);
    return 0;
}

static int32_t load_npy_file(CSOUND *csound, FILE* fp, ARRAYDAT *arr) {
    npy_header header;
    int err = _parse_npy_header(fp, &header);
    if(err)
        return err;
    size_t numitems = 1;
    for(int i=0; i<header.numdimensions; i++)
        numitems = numitems*header.sizes[i];
    tabinit(csound, arr, numitems);
    size_t numbytes = header.word_size * numitems;
    size_t nread = 0;
    if(header.type == 'f') {
        if(header.word_size == 8) {
            // read directly into the array data
            nread = fread(arr->data, 1, numbytes, fp);
            if (nread != numbytes) return 2;
        } else if(header.word_size == 4) {
            // float *tmp = (float*)malloc(numbytes);
            float *tmp = (float*)csound->Malloc(csound, numbytes);
            nread = fread(tmp, 1, numbytes, fp);
            if (nread != numbytes) return 2;
            for(size_t i=0; i<numitems; i++) {
                arr->data[i] = (MYFLT)tmp[i];
            }
            // free(tmp);
            csound->Free(csound, tmp);
        } else
            return 4;
    } else if(header.type == 'i') {
        if(header.word_size == 8) {
            // int64_t *tmp = (int64_t*)malloc(numbytes);
            int64_t *tmp = (int64_t*)csound->Malloc(csound, numbytes);
            nread = fread(tmp, 1, numbytes, fp);
            if (nread != numbytes) return 2;
            for(size_t i=0; i<numitems; i++) {
                arr->data[i] = (MYFLT)tmp[i];
            }
            // free(tmp);
            csound->Free(csound, tmp);
        } else if (header.word_size == 4) {
            // int32_t *tmp = (int32_t*)malloc(numbytes);
            int32_t *tmp = (int32_t*)csound->Malloc(csound, numbytes);
            nread = fread(tmp, 1, numbytes, fp);
            if (nread != numbytes) return 2;
            for(size_t i=0; i<numitems; i++) {
                arr->data[i] = (MYFLT)tmp[i];
            }
            // free(tmp);
            csound->Free(csound, tmp);
        }
    }
    arr->dimensions = header.numdimensions;
    if(header.numdimensions > 1) {
        csound->Free(csound, arr->sizes);
        csound->Calloc(csound, sizeof(int32_t)*arr->dimensions);
        for(int i=0; i<header.numdimensions; i++)
            arr->sizes[i] = header.sizes[i];
    }
    return 0;
}


static const char *_get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}


static int32_t loadnpy(CSOUND *csound, loadnpy_ARR *p) {
    const char *ext = _get_filename_ext(p->path->data);
    if(strncmp(ext, "npy", 3)) {
       return INITERRF("Format should be npy, but got %s", ext);
    }

    FILE* fp = fopen(p->path->data, "rb");
    if(fp == NULL) {
        return INITERRF("File %s not found", p->path->data);
    }

    int err = load_npy_file(csound, fp, p->out);
    if(err != 0) {
        return INITERRF("Could not load file, error: %d", err);
    }

    fclose(fp);
    return OK;
}

// ---------------
/* rowsweightedsum - apply weights and sum the rows of an array
 * krow[] rowsewightedsum gipresetRows[], kweights
 */

typedef struct {
    OPDS h;
    ARRAYDAT *outrow;
    ARRAYDAT *rows;
    ARRAYDAT *weights;
} ROWSWEIGHTEDSUM;

static int32_t rowsweightedsum_init(CSOUND *csound, ROWSWEIGHTEDSUM *p) {
    if(p->rows->dimensions != 2) {
        return INITERRF("This opcode expects a 2d array as its first argument, got %d dimensions", p->rows->dimensions);
    }
    if(p->weights->dimensions != 1) {
        return INITERRF("This opcode expects a 1d array as its 2nd argument, got %d dimensions", p->weights->dimensions);
    }
    int numrows = p->rows->sizes[0];
    int numweights = p->rows->sizes[0];
    if(numrows != numweights) {
        csound->Warning(csound,
                        "weightedsum: number of rows (%d) != number of weights (%d), "
                        "will use the first %d rows for calculation\n",
                        numrows, numweights, min(numrows, numweights));
    }
    int rowsize = p->rows->sizes[1];
    tabinit(csound, p->outrow, rowsize);
    return OK;
}


static int32_t rowsweightedsum_perf(CSOUND *csound, ROWSWEIGHTEDSUM *p) {
    MYFLT *out = p->outrow->data;
    MYFLT *mtx = p->rows->data;
    int numrows = p->rows->sizes[0];
    int numweights = p->weights->sizes[0];
    int n = min(numrows, numweights);
    int rowsize = p->rows->sizes[1];
    for(int j=0; j < rowsize; j++) {
        out[j] = 0.;
    }
    for(int i=0; i < n; i++) {
        MYFLT weight = p->weights->data[i];
        for(int j=0; j < rowsize; j++) {
            out[j] += mtx[i*rowsize+j] * weight;
        }
    }
    return OK;
}

static int32_t rowsweightedsum_i(CSOUND *csound, ROWSWEIGHTEDSUM *p) {
    rowsweightedsum_init(csound, p);
    return rowsweightedsum_perf(csound, p);
}


/* ----------------
  presetinterp - interpolate between "presets" in 2D

  iCoords[] fillarray ix0, iy0, ix1, iy1, ..., ixn, iyn
  kweights[] presetinterp kx, ky, kCoords, iclamp=0

  A second version allows each point to have a third value: a weight
  With this it is possible to make the range of each point smaller/larger

  iPoints[] fillarray ix0, iy0, iweight0, ix1, iy1, iweight1, ...

*/

#define PRESETINTERP_MAXPOINTS 100

typedef struct {
    OPDS h;
    ARRAYDAT *weights;
    MYFLT *kx;
    MYFLT *ky;
    ARRAYDAT *coords;
    MYFLT *clamp;
    size_t numpoints;
    double mindistances[PRESETINTERP_MAXPOINTS];
    int pointsize;

} PRESETINTERP;


static inline MYFLT euclidian_distance(MYFLT x0, MYFLT y0, MYFLT x1, MYFLT y1) {
    MYFLT dx = x1 - x0;
    MYFLT dy = y1 - y0;
    return sqrt(dx*dx + dy*dy);
}

static MYFLT distance_to_nearest_point(MYFLT x, MYFLT y, ARRAYDAT *arr, int pointsize) {
    size_t n = arr->sizes[0];
    if(n == 0) {
        return -1.;
    }
    MYFLT mindistance = INF;
    MYFLT *data = arr->data;
    MYFLT px, py;
    MYFLT distance;
    for(size_t i=0; i < n; i++) {
        px = data[i*pointsize];
        py = data[i*pointsize+1];
        distance = euclidian_distance(x, y, px, py);
        if(distance < mindistance)
            mindistance = distance;
    }
    return mindistance;
}


static void calculate_mindistances(ARRAYDAT *points, double *mindistances, const int pointsize) {
    size_t numpoints = points->sizes[0] / pointsize;
    double distance, ix, iy, jx, jy;
    double *data = points->data;

    for(size_t i=0; i < numpoints; i++) {
        mindistances[i] = INF;
    }
    for(size_t i=0; i < numpoints - 1; i++) {
        for(size_t j=i+1; j < numpoints; j++) {
            ix = data[i*pointsize];
            iy = data[i*pointsize+1];
            jx = data[j*pointsize];
            jy = data[j*pointsize+1];
            distance = euclidian_distance(ix, iy, jx, jy);
            if(distance < mindistances[i]) {
                mindistances[i] = distance;
            }
            if(distance < mindistances[j]) {
                mindistances[j] = distance;
            }
        }
    }
}

static MYFLT calculate_weight(int pointidx, PRESETINTERP *p, MYFLT cursorx, MYFLT cursory) {
    MYFLT cursor_radius = distance_to_nearest_point(cursorx, cursory, p->coords, p->pointsize);
    MYFLT point_radius = p->mindistances[pointidx];
    double *coordsdata = p->coords->data;
    MYFLT dist = euclidian_distance(cursorx, cursory, coordsdata[pointidx*p->pointsize], coordsdata[pointidx*p->pointsize+1]);
    MYFLT intersection = point_radius + cursor_radius - dist;
    MYFLT weight = intersection / point_radius;
    if(p->pointsize == 3) {
        MYFLT weightfactor = p->coords->data[pointidx*p->pointsize+2];
        weight *= weightfactor;
    }

    return weight;
}

static void calculate_weights(PRESETINTERP *p) {
    MYFLT weights[PRESETINTERP_MAXPOINTS];
    MYFLT weight;
    MYFLT x = *p->kx;
    MYFLT y = *p->ky;
    MYFLT sumweights = 0;
    for(size_t i=0; i < p->numpoints; i++) {
        weight = max(0, calculate_weight(i, p, x, y));
        weights[i] = weight;
        sumweights += weight;
    }
    if(sumweights == 0.) {
        for(size_t i=0; i < p->numpoints; i++) {
            p->weights->data[i] = 0;
        }
        return;
    }
    for(size_t i=0; i < p->numpoints; i++) {
        weights[i] /= sumweights;
    }
    MYFLT clamp = *p->clamp;
    if(clamp > 0) {
        MYFLT sumvalid = 0.;
        clamp = clamp/p->numpoints;
        for(size_t i=0; i < p->numpoints; i++) {
            weight = weights[i];
            if(weight < clamp) {
                weights[i] = 0.;
            } else {
                sumvalid += weight;
            }
        }
        for(size_t i=0; i < p->numpoints; i++) {
            weights[i] /= sumvalid;
        }
    }
    for(size_t i=0; i < p->numpoints; i++) {
        p->weights->data[i] = weights[i];
    }
}


static int32_t presetinterp_init(CSOUND *csound, PRESETINTERP *p) {
    int coordslen = p->coords->sizes[0];
    if(coordslen % 2 != 0) {
        return INITERRF("The points array should be a multiple of 2, got %d items", coordslen);
    }
    p->numpoints = coordslen / 2;
    if(p->numpoints <= 0) {
        return INITERRF("Not enough points defined: %zu", p->numpoints);
    } else if(p->numpoints > PRESETINTERP_MAXPOINTS) {
        return INITERRF("Too many points, max=%d", PRESETINTERP_MAXPOINTS);
    }
    tabinit(csound, p->weights, p->numpoints);
    p->pointsize = 2;
    return OK;
}


static int32_t presetinterp_perf(CSOUND *csound, PRESETINTERP *p) {
    IGN(csound);
    calculate_mindistances(p->coords, p->mindistances, p->pointsize);
    calculate_weights(p);
    return OK;
}


static int32_t presetinterpw_init(CSOUND *csound, PRESETINTERP *p) {
    int coordslen = p->coords->sizes[0];
    if(coordslen % 3 != 0) {
        return INITERRF("The points array should be a multiple of 3, got %d items", coordslen);
    }
    p->numpoints = coordslen / 3;
    if(p->numpoints <= 0) {
        return INITERRF("Not enough points defined: %zu", p->numpoints);
    } else if(p->numpoints > PRESETINTERP_MAXPOINTS) {
        return INITERRF("Too many points, max=%d", PRESETINTERP_MAXPOINTS);
    }
    tabinit(csound, p->weights, p->numpoints);
    p->pointsize = 3;
    return OK;
}


// ---------------
// DetectSilence - ported from supercollider
// ksilenceDetected detectSilence ain, kamp=-1, ktime=-1 (k, aJJ)
// in: audio, amp: 0.0001, time: 0.1
typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *ain;
    MYFLT *kthresh;
    MYFLT *ktime;
    int32_t counter;
} DETECT_SILENCE;

static int32_t detectSilence_init(CSOUND *csound, DETECT_SILENCE *p) {
    p->counter = -1;
    return OK;
}

static int32_t detectSilence_k_a(CSOUND *csound, DETECT_SILENCE *p) {
    MYFLT thresh = *p->kthresh;
    if(thresh < 0)
        thresh = 0.0001;
    MYFLT ktime = *p->ktime;
    if(ktime < 0)
        ktime = 0.1;

    int endCounter = (int32_t)(csound->GetSr(csound) * ktime);
    int counter = p->counter;
    MYFLT val;
    MYFLT out = 0.;
    MYFLT *in = p->ain;

    uint32_t n, nsmps = CS_KSMPS;
    uint32_t offset = p->h.insdshead->ksmps_offset;

    for(n=offset; n<nsmps; n++) {
        val = fabs(*in++);
        if(val > thresh) {
            counter = 0;
        } else if(counter >= 0) {
            if(++counter >= endCounter) {
                out = 1.;
                break;
            }
        }
    }
    p->counter = counter;
    *p->out = out;
    return OK;
}

static int32_t detectSilence_a_a(CSOUND *csound, DETECT_SILENCE *p) {
    MYFLT thresh = *p->kthresh;
    MYFLT ktime = *p->ktime;
    int endCounter = (int32_t)(csound->GetSr(csound) * ktime);
    int counter = p->counter;
    MYFLT val;
    MYFLT *in = p->ain;
    MYFLT *out = p->out;

    AUDIO_OPCODE(csound, p);
    AUDIO_OUTPUT(out);

    for(n=offset; n<nsmps; n++) {
        val = fabs(*in++);
        if(val > thresh) {
            counter = 0;
            *out++ = 0.;
        } else if(counter >= 0) {
            if(++counter >= endCounter) {
                *out++ = 1.;
            } else {
                *out++ = 0.;
            }
        } else
            *out++ = 0.;
    }
    return OK;
}

/*
typedef struct {
    OPDS h;
    ARRAYDAT *programs;
    STRINGDAT *s;
} SFLISTPROGRAMS;


static int32_t sflistprograms(CSOUND *csound, SFLISTPROGRAMS *p) {
    enum { maxprogs = 200, bufsize = 256 };
    const char *sfpath = p->s->data;
    char buf[bufsize];
    const char *adrivers[1] = { NULL };
    fluid_audio_driver_register(adrivers);
    int ret = OK;
    fluid_settings_t *settings = new_fluid_settings();
    fluid_settings_setint(settings, "synth.dynamic-sample-loading", 1);

    fluid_synth_t *sf = new_fluid_synth(settings);
    int id = fluid_synth_sfload(sf, sfpath, 0);
    if (id < 0) {
        INITERRF("Could not load soundfont %s", sfpath);
        ret = NOTOK;
        goto EXIT;
    }

    tabinit(csound, p->programs, maxprogs);

    fluid_sfont_t *sfont = fluid_synth_get_sfont_by_id(sf, id);

    fluid_sfont_iteration_start(sfont);
    int n = 0;
    STRINGDAT *programs = (STRINGDAT *)p->programs->data;
    while(n < maxprogs) {
        fluid_preset_t *preset = fluid_sfont_iteration_next(sfont);
        if(preset == NULL) {
            break;
        }

        const char *name = fluid_preset_get_name(preset);
        int banknum = fluid_preset_get_banknum(preset);
        int num = fluid_preset_get_num(preset);
        snprintf(buf, bufsize, "%03d-%03d %s", banknum, num, name);
        programs[n].size = strlen(buf);
        programs[n].data = csound->Strdup(csound, buf);
        n++;
    }
    p->programs->sizes[0] = n;
 EXIT:
    delete_fluid_synth(sf);
    delete_fluid_settings(settings);
    return ret;
}
*/


#define clip(a,b,c) (a<b ? b : a>c ? c : a)
#define PI2 1.5707963267948966

typedef struct {
    OPDS h;
    MYFLT *leftout, *rightout;
    MYFLT *leftin, *rightin;
    MYFLT *kpos, *klevel;
    MYFLT m_pos, m_level, m_leftamp, m_rightamp;
} BALANCE2;

static int32_t balance2_init(CSOUND *csound, BALANCE2 *p) {
    IGN(csound);
    p->m_pos = clip(*p->kpos, 0, 1);
    p->m_level = *p->klevel;
    MYFLT pos = p->m_pos;
    p->m_leftamp = p->m_level * sin(PI2 * (1-pos));
    p->m_rightamp = p->m_level * sin(PI2*pos);
    return OK;
}

static int32_t balance2_ak(CSOUND *csound, BALANCE2 *p) {
    // TODO: sample accurate
    IGN(csound);
    int32_t nsmps = CS_KSMPS;

    MYFLT *leftout = p->leftout;
    MYFLT *rightout = p->rightout;
    MYFLT *leftin = p->leftin;
    MYFLT *rightin = p->rightin;
    MYFLT pos = *(p->kpos);
    MYFLT level = *p->klevel;
    MYFLT leftamp = p->m_leftamp;
    MYFLT rightamp = p->m_rightamp;
    if(pos != p->m_pos || p->m_level != level) {
        MYFLT nextleftamp = level * sin(PI2 * (1-pos));
        MYFLT nextrightamp = level * sin(PI2 * pos);
        MYFLT leftampslope = (nextleftamp - leftamp) / nsmps;
        MYFLT rightampslope = (nextrightamp - rightamp) / nsmps;
        for(int i=0; i<nsmps; i++) {
            leftout[i] = leftin[i] * leftamp;
            rightout[i] = rightin[i] * rightamp;
            leftamp += leftampslope;
            rightamp += rightampslope;
        }
        p->m_pos = pos;
        p->m_level = level;
        p->m_leftamp = nextleftamp;
        p->m_rightamp = nextrightamp;
    } else {
        for(int i=0; i<nsmps; i++) {
            leftout[i] = leftin[i] * leftamp;
            rightout[i] = rightin[i] * rightamp;
        }
    }
    return OK;
}


/*
 zerocrossing

 afreq zerocrossing ain
 

void ZeroCrossing_Ctor(ZeroCrossing* unit) {
    SETCALC(ZeroCrossing_next_a);

    unit->m_prevfrac = 0.f;
    unit->m_previn = ZIN0(0);
    ZOUT0(0) = unit->mLevel = 0.f;
    unit->mCounter = 0;
}

void ZeroCrossing_next_a(ZeroCrossing* unit, int inNumSamples) {
    float* out = ZOUT(0);
    float* in = ZIN(0);
    float previn = unit->m_previn;
    float prevfrac = unit->m_prevfrac;
    float level = unit->mLevel;
    long counter = unit->mCounter;

    LOOP1(
        inNumSamples, counter++; float curin = ZXP(in); if (counter > 4 && previn <= 0.f && curin > 0.f) {
            float frac = -previn / (curin - previn);
            level = unit->mRate->mSampleRate / (frac + counter - prevfrac);
            prevfrac = frac;
            counter = 0;
        } ZXP(out) = level;
        previn = curin;);

    unit->m_previn = previn;
    unit->m_prevfrac = prevfrac;
    unit->mLevel = level;
    unit->mCounter = counter;
}

*/


typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *in;
    MYFLT previn, prevfrac, level;
    size_t counter;
} ZEROCROSSING;


static int32_t zerocrossing_init(CSOUND *csound, ZEROCROSSING *p) {
    p->prevfrac = FL(0);
    p->previn = FL(0);
    p->level = FL(0);
    p->counter = 0;
    return OK;
}

static int32_t zerocrossing_a_a(CSOUND *csound, ZEROCROSSING *p) {
    MYFLT* out = p->out;
    MYFLT* in = p->in;
    MYFLT previn = p->previn;
    MYFLT prevfrac = p->prevfrac;
    MYFLT level = p->level;
    MYFLT curin;
    size_t counter = p->counter;
    size_t nsmps = CS_KSMPS;
    MYFLT sr = csound->GetSr(csound);

    for(size_t i=0; i<nsmps; i++) {
        counter++;
        curin = in[i];
        if(counter > 4 && previn<0 && curin >0) {
            MYFLT frac = -previn / (curin - previn);
            level = sr / (frac + counter - prevfrac);
            prevfrac = frac;
            counter = 0;
        }
        out[i] = level;
        previn = curin;
    }
    p->previn = previn;
    p->prevfrac = prevfrac;
    p->level = level;
    p->counter = counter;
    return OK;
}

// ----------------------------------------
// vowelsdb
// iformants[], ibws[], iamps[] vowelsdb Sspeakername, Svowels
// Svowels: a space separated list of vowels to get from the database
// Example:
// iformants[], ibws[], iamps[] vowelsdb "vtl-male", "a e i o u ae"
// If the speaker does not define a given vowel then the row will be empty

#define MAX_VOWELS 20

typedef struct {
    OPDS h;
    ARRAYDAT *freqs, *bws, *amps;
    STRINGDAT *speaker;
    STRINGDAT *vowels;

    int vowelindexes[MAX_VOWELS];

} VOWELSDB;


typedef struct {
    char name[32];
    MYFLT freqs[MAX_VOWELS][5];
    MYFLT bws[MAX_VOWELS][5];
    MYFLT amps[MAX_VOWELS][5];
} vowelspeaker;

const static vowelspeaker voweldb[] = {
    {
        .name = "vtl-male",
        .freqs = {
            {668, 1191, 2428, 3321, 4600}, // A
            {327, 2157, 2754, 3630, 4600}, // E
            {208, 2152, 3128, 3425, 4200}, // I
            {335, 628, 2689, 3515, 4200},  // O
            {254, 796, 2515, 3274, 4160}   // U
        },
        .bws = {
            {80, 90, 120, 130, 140},
            {60, 100, 120, 150, 200},
            {60, 90, 100, 120, 120},
            {40, 80, 100, 120, 120},
            {50, 60, 170, 180, 200}
        },
        .amps = {
            {25, 25, 12, 10, 10},
            {5.6, 17.7, 15.8, 10, 14},
            {3.16, 10, 22.4, 19.9, 10},
            {5.6, 7.9, 1.78, 2.24, 3.98},
            {3.98, 3.16, 1.99, 1.77, 3.98}
        }
    },
    {
        .name = "csound-soprano",
        .freqs = {
            {800, 1150, 2900, 3900, 4950}, // A
            {350, 2000, 2800, 3600, 4950}, // E
            {270, 2140, 2950, 3900, 4950}, // I
            {450, 800,  2830, 3800, 4950}, // O
            {325, 700,  2700, 3800, 4950}, // U
        },
        .bws = {
            {80, 90,  120, 130, 140},
            {60, 100, 120, 150, 200},
            {60, 90,  100, 120, 120},
            {40, 80,  100, 120, 120},
            {50, 60,  170, 180, 200}

        },
        .amps = {
            {1.000115, 0.501187, 0.025119, 0.1, 0.003162},
            {1.000115, 0.1, 0.177828, 0.01, 0.001585},
            {1.000115, 0.251189, 0.050119, 0.050119, 0.00631},
            {1.000115, 0.281838, 0.079433, 0.079433, 0.003162},
            {1.000115, 0.158489, 0.017783, 0.01, 0.001}

        }
    },
    {
        .name = "csound-alto",
        .freqs = {
            {800, 1150, 2800, 3500, 4950},  // A
            {400, 1600, 2700, 3300, 4950},  // E
            {350, 1700, 2700, 3700, 4950},  // I
            {450, 800, 2830, 3500, 4950},   // O
            {325, 700, 2530, 3500, 4950}    // U
        },
        .bws = {
            {80, 90, 120, 130, 140},
            {60, 80, 120, 150, 200},
            {50, 100, 120, 150, 200},
            {70, 80, 100, 130, 135},
            {50, 60, 170, 180, 200}
        },
        .amps = {
            {1.000115, 0.630957, 0.1, 0.015849, 0.001},
            {1.000115, 0.063096, 0.031623, 0.017783, 0.001},
            {1.000115, 0.1, 0.031623, 0.015849, 0.001},
            {1.000115, 0.354813, 0.158489, 0.039811, 0.001778},
            {1.000115, 0.251189, 0.031623, 0.01, 0.000631}
        }
    },
    {
        .name = "csound-countertenor",
        .freqs = {
            {660, 1120, 2750, 3000, 3350},  // A
            {440, 1800, 2700, 3000, 3300},  // E
            {270, 1850, 2900, 3350, 3590},  // I
            {430, 820, 2700, 3000, 3300},   // O
            {370, 630, 2750, 3000, 3400}    // U
        },
        .bws = {
            {80, 90, 120, 130, 140},
            {70, 80, 100, 120, 120},
            {40, 90, 100, 120, 120},
            {40, 80, 100, 120, 120},
            {40, 60, 100, 120, 120}
        },
        .amps = {
            {1.000115, 0.501187, 0.070795, 0.063096, 0.012589},
            {1.000115, 0.199526, 0.125893, 0.1, 0.1},
            {1.000115, 0.063096, 0.063096, 0.015849, 0.015849},
            {1.000115, 0.316228, 0.050119, 0.079433, 0.019953},
            {1.000115, 0.1, 0.070795, 0.031623, 0.019953}
        }
    },
    {
        .name = "csound-tenor",
        .freqs = {
            {650, 1080, 2650, 2900, 3250},
            {400, 1700, 2600, 3200, 3580},
            {290, 1870, 2800, 3250, 3540},
            {400, 800, 2600, 2800, 3000},
            {350, 600, 2700, 2900, 3300}
        },
        .bws = {
            {80, 90, 120, 130, 140},
            {70, 80, 100, 120, 120},
            {40, 90, 100, 120, 120},
            {70, 80, 100, 130, 135},
            {40, 60, 100, 120, 120}
        },
        .amps = {
            {1.000115, 0.501187, 0.446684, 0.398107, 0.079433},
            {1.000115, 0.199526, 0.251189, 0.199526, 0.1},
            {1.000115, 0.177828, 0.125893, 0.1, 0.031623},
            {1.000115, 0.316228, 0.251189, 0.251189, 0.050119},
            {1.000115, 0.1, 0.141254, 0.199526, 0.050119}
        }
    },
    {
        .name = "csound-bass",
        .freqs = {
            {600, 1040, 2250, 2450, 2750},
            {400, 1620, 2400, 2800, 3100},
            {250, 1750, 2600, 3050, 3340},
            {400, 750, 2400, 2600, 2900},
            {350, 600, 2400, 2675, 2950}
        },
        .bws = {
            {60, 70, 110, 120, 130},
            {40, 80, 100, 120, 120},
            {60, 90, 100, 120, 120},
            {40, 80, 100, 120, 120},
            {40, 80, 100, 120, 120}
        },
        .amps = {
            {1.000115, 0.446684, 0.354813, 0.354813, 0.1},
            {1.000115, 0.251189, 0.354813, 0.251189, 0.125893},
            {1.000115, 0.031623, 0.158489, 0.079433, 0.039811},
            {1.000115, 0.281838, 0.089125, 0.1, 0.01},
            {1.000115, 0.1, 0.025119, 0.039811, 0.015849}
        }
    },
    // this definition should be last
    {
        .name = ""
    }
};

const char *_defined_speakers = "vtl-male, csound-soprano, csound-alto, csound-countertenor, csound-tenor, csound-bass";


int speaker_name_to_index(char *name) {
    if(strcmp(name, "vtl-male")==0)
        return 0;
    return -1;
}

static char* vowelnames[] = {
    "a",   // 0
    "e",   // 1
    "i",   // 2
    "o",   // 3
    "u"    // 4
};

int vowel_to_index(char *vowel) {
    int l = strlen(vowel);
    if(l == 1) {
        char vowel0 = vowel[0];
        switch(vowel0) {
        case 'a':
        case 'A':
            return 0;
        case 'e':
        case 'E':
            return 1;
        case 'i':
        case 'I':
            return 2;
        case 'o':
        case 'O':
            return 3;
        case 'u':
        case 'U':
            return 4;
        }
    }
    return -1;
}

static char * _strsep(char **sp, char *sep) {
    char *p, *s;
    if (sp == NULL || *sp == NULL || **sp == '\0') return(NULL);
    s = *sp;
    p = s + strcspn(s, sep);
    if (*p != '\0') *p++ = '\0';
    *sp = p;
    return(s);
}

int parse_requested_vowels(const char *vowelsdef, int vowelindexes[MAX_VOWELS]) {
    char vowelsbuf[256];
    char *vowels = vowelsbuf;
    strncpy(vowelsbuf, vowelsdef, 255);
    char *token;
    int numvowels = 0;
    while ((token = _strsep(&vowels, " "))) {
        if(strlen(token)==0)
            continue;
        int vowelindex = vowel_to_index(token);
        if(vowelindex == -1) {
            printf("Unknown vowel: '%s'\n", token);
            return -1;
        }
        vowelindexes[numvowels] = vowel_to_index(token);
        numvowels++;
        if(numvowels == MAX_VOWELS) {
            break;
        }
    }
    return numvowels;
}

int32_t arraymake2d(CSOUND *csound, ARRAYDAT *arr, int numcols) {
    if(arr->dimensions != 1) {
        printf("arraymake2d: array is not 1D\n");
        return NOTOK;
    }
    int flatsize = arr->sizes[0];
    if(flatsize % numcols != 0) {
        printf("arraymale2d: array size %d is not divisible by colsize %d\n", flatsize, numcols);
        return NOTOK;
    }
    arr->sizes = csound->ReAlloc(csound, arr->sizes, sizeof(int32_t)*2);
    arr->dimensions = 2;
    arr->sizes[0] = flatsize / numcols;
    arr->sizes[1] = numcols;
    return OK;
}

int32_t tabinit2d(CSOUND *csound, ARRAYDAT *arr, int numrows, int numcols) {
    int numelements = numrows * numcols;
    tabinit(csound, arr, numelements);
    int res = arraymake2d(csound, arr, numcols);
    return res;
}

const vowelspeaker *find_speaker(char *speakername) {
    for(int i=0;;i++) {
        const vowelspeaker *speaker = &voweldb[i];
        if(speaker->name[0] == '\0')
            return NULL;
        if(!strcmp(speaker->name, speakername))
            return speaker;
    }
}

static int32_t vowelsdb_i(CSOUND *csound, VOWELSDB *p) {
    int numvowels = parse_requested_vowels(p->vowels->data, p->vowelindexes);
    if(numvowels == -1) {
        return INITERRF("Could not parse vowels: %s", p->vowels->data);
    }
    tabinit2d(csound, p->freqs, numvowels, 5);
    tabinit2d(csound, p->bws, numvowels, 5);
    tabinit2d(csound, p->amps, numvowels, 5);
    const vowelspeaker *speaker = find_speaker(p->speaker->data);
    if(speaker == NULL) {
        return INITERRF("Speaker not found: %s. Defined speakers: %s", p->speaker->data, _defined_speakers);
    }
    for(int i=0; i < numvowels; i++) {
        int vowelindex = p->vowelindexes[i];
        for(int j=0; j < 5; j++) {
            p->freqs->data[i*5 + j] = speaker->freqs[vowelindex][j];
            p->amps->data[i*5 + j] = speaker->amps[vowelindex][j];
            p->bws->data[i*5 + j] = speaker->bws[vowelindex][j];
        }
    }

    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *kfreq;
    MYFLT *initphase;
    MYFLT accum;
    size_t counter;
    MYFLT lastfreq;
} MTRO;

static int32_t mtro_init(CSOUND *csound, MTRO *p) {
    p->counter = 0;
    p->lastfreq = 0;
    p->accum = *p->initphase;
    return OK;
}

static int32_t mtro(CSOUND *csound, MTRO *p) {
    MYFLT freq = *p->kfreq;
    if(freq != p->lastfreq) {
        MYFLT lastdx = p->lastfreq * CS_ONEDKR;
        p->accum = p->accum + p->counter * lastdx;
        p->counter = 0;
        p->lastfreq = freq;
    }
    MYFLT dx = freq * CS_ONEDKR;

    MYFLT phase = p->accum + p->counter * dx;
    // printf("kcount: %d, phase: %f\n", CS_KCNT, phase);
    if(phase >= 1.) {
        p->accum = phase - 1.0;
        p->counter = 0;
        *p->out= 1.;
    } else {
        *p->out = 0.;
    }
    p->counter++;

    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *instrnum;
    STRINGDAT *instrname;
} NAMETOINSTRNUM;

static int32_t nametoinstrnum(CSOUND *csound, NAMETOINSTRNUM *p) {
    int instrnum = csound->strarg2insno(csound, ((STRINGDAT*)p->instrname)->data, 1);
    if(instrnum == NOT_AN_INSTRUMENT) {
        *p->instrnum = -1;
    } else {
        *p->instrnum = (MYFLT)instrnum;
    }
    return OK;
}


// kt eventtime                    ; time
// kindex cuetrig kt, 0.1, 0.3, 0.4, 0.9
// kindex is 0 normally, the index of the cue when kt reaches a value in ifn



typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *kcursor;
    MYFLT *pargs [VARGMAX-5];
    MYFLT nextval;
    size_t nextidx;
    MYFLT lastcursor;
    size_t numargs;
} CUETRIG;

static int32_t cuetrig_init(CSOUND *csound, CUETRIG *p) {
    p->lastcursor = -INF;
    p->numargs = csound->GetInputArgCnt(p) - 1;
    p->nextidx = 0;
    p->nextval = *(p->pargs[0]);
    return OK;
}

static int32_t cuetrig(CSOUND *csound, CUETRIG *p) {
    MYFLT cursor = *(p->kcursor);
    if(cursor < p->lastcursor) {
        p->nextidx = 0;
        p->nextval = *(p->pargs[0]);
    }
    if(cursor >= p->nextval) {
        *p->out = p->nextidx + 1;
        if(p->nextidx + 1 >= p->numargs) {
            // last trigger
            p->nextval = INF;
            p->nextidx = 0;
        } else {
            p->nextidx += 1;
            p->nextval = *(p->pargs[p->nextidx]);
        }
    } else {
        *p->out = 0.;
    }
    p->lastcursor = cursor;
    return OK;
}


// Map a gain to a velocity
// gain in 0-1
// mingain: the gain equivalent to velocity 1 (gain 0 always equals to vel 0)
// exp: an exponent to apply to the curve
// minvel=0: the min. velocity
// round=0: if different than 0, round the resulting velocity to an int

typedef struct {
    OPDS h;
    MYFLT *vel;
    MYFLT *gain;
    MYFLT *mingain;
    MYFLT *exp;
    MYFLT *minvel;
    MYFLT *round;

} GAINTOVEL;

static int32_t gaintovel(CSOUND *csound, GAINTOVEL *p) {
    MYFLT gain = *p->gain;
    MYFLT minvel = max(1., *p->minvel);
    MYFLT vel;
    if(gain <= 0.) {
        vel = 0.;
    } else {
        MYFLT mingain = *p->mingain;
        MYFLT relgain = (gain - mingain) / (1. - mingain);
        if(relgain <= 0.) {
            vel = minvel;
        } else {
            vel = pow(relgain, *p->exp) * (127 - minvel) + minvel;
        }
    }
    if(vel > 0. && *p->round != 0) {
        vel = max(minvel, round(vel));
    }
    *p->vel = vel;
    return OK;
}


typedef struct {
    OPDS h;
    MYFLT *y;
    MYFLT *x;
    MYFLT *exp;
    MYFLT *x0;
    MYFLT *x1;
    MYFLT *y0;
    MYFLT *y1;
} LINEXP;


static int32_t linexp(CSOUND *csound, LINEXP *p) {
    MYFLT x0 = *p->x0, x1 = *p->x1, x = *p->x, y0 = *p->y0;
    MYFLT dx = (x - x0) / (x1 - x0);
    if(dx < 0) {
        return NOTOK;
    }
    *p->y = pow(dx, *p->exp) * (*p->y1 - y0) + y0;
    return OK;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#define S(x) sizeof(x)

static OENTRY localops[] = {
    { "crackle", S(CRACKLE), 0, 3, "a", "P", (SUBR)crackle_init, (SUBR)crackle_perf },

    { "ramptrig.k_kk", S(RAMPTRIGK), 0, 3, "k", "kkP", (SUBR)ramptrig_k_kk_init,
      (SUBR)ramptrig_k_kk },
    { "ramptrig.a_kk", S(RAMPTRIGK), 0, 3, "a", "kkP", (SUBR)ramptrig_a_kk_init,
      (SUBR)ramptrig_a_kk },
    { "ramptrig.sync_kk_kk", S(RAMPTRIGSYNC), 0, 3, "kk", "kkPO",
      (SUBR)ramptrigsync_kk_kk_init, (SUBR)ramptrigsync_kk_kk},

    { "sigmdrive.a_ak",S(SIGMDRIVE), 0, 2, "a", "akO", NULL, (SUBR)sigmdrive_a_ak},
    { "sigmdrive.a_aa",S(SIGMDRIVE), 0, 2, "a", "aaO", NULL, (SUBR)sigmdrive_a_aa},

    { "lfnoise", S(LFNOISE), 0, 3, "a", "kO", (SUBR)lfnoise_init, (SUBR)lfnoise_perf},

    { "schmitt.k", S(SCHMITT), 0, 3, "k", "kkk", (SUBR)schmitt_k_init, (SUBR)schmitt_k_perf},
    { "schmitt.a", S(SCHMITT), 0, 3, "a", "akk", (SUBR)schmitt_k_init, (SUBR)schmitt_a_perf},

    { "standardchaos", S(STANDARDCHAOS), 0, 3, "a", "kkio",
      (SUBR)standardchaos_init, (SUBR)standardchaos_perf},
    { "standardchaos", S(STANDARDCHAOS), 0, 3, "a", "kP",
      (SUBR)standardchaos_init_x, (SUBR)standardchaos_perf},

    { "linenv.k_k", S(RAMPGATE), 0, 3, "k", "kiM", (SUBR)linenv_k_init, (SUBR)linenv_k_k},
    { "linenv.a_k", S(RAMPGATE), 0, 3, "a", "kiM", (SUBR)linenv_a_init, (SUBR)linenv_a_k},

    // aout dioderingmod ain, kfreq, kdiodeon=1, kfeedback=0, knonlinear=0, ioversample=0
    { "diode_ringmod", S(t_diode_ringmod), 0, 3, "a", "akPOOO",
      (SUBR)dioderingmod_init, (SUBR)dioderingmod_perf},

    { "fileexists", S(FILE_EXISTS), 0, 1, "i", "S", (SUBR)file_exists_init},

    { "pread.i", S(PREAD), 0, 1, "i", "iij", (SUBR)pread_i},
    { "pread.k", S(PREAD), 0, 3, "k", "ikJ", (SUBR)pread_init, (SUBR)pread_perf},
    { "pread.arr_i", S(PREADARR), 0, 1, "i[]", "ii[]j", (SUBR)preadarr_i},
    { "pwrite.i", S(PWRITE), 0, 1, "", "im", (SUBR)pwrite_i},
    { "pwrite.k", S(PWRITE), 0, 3, "", "i*",
      (SUBR)pwrite_initcommon, (SUBR)pwrite_perf},

    { "pwriten.k", S(PWRITE), 0, 3, "", "k*",
      (SUBR)pwriten_init, (SUBR)pwriten_perf},

    { "uniqinstance.i", S(UNIQINSTANCE),   0, 1, "i", "io", (SUBR)uniqueinstance_i},
    { "uniqinstance.S_i", S(UNIQINSTANCE), 0, 1, "i", "So", (SUBR)uniqueinstance_S_init},

    { "atstop.s1", S(SCHED_DEINIT), 0, 1, "", "Soj", (SUBR)atstop_s },
    { "atstop.s", S(SCHED_DEINIT),  0, 1, "", "Siim", (SUBR)atstop_s },
    { "atstop.i1", S(SCHED_DEINIT), 0, 1, "", "ioj", (SUBR)atstop_i },
    { "atstop.i", S(SCHED_DEINIT), 0, 1, "", "iiim", (SUBR)atstop_i },
    // { "atstop.k", S(SCHED_DEINIT), 0, 1, "", "iii*", (SUBR)atstop_i },
    // { "atstop.Sk", S(SCHED_DEINIT), 0, 1, "", "Sii*", (SUBR)atstop_s },

    { "accum.k", S(ACCUM), 0, 3, "k", "koO", (SUBR)accum_init, (SUBR)accum_perf},
    { "accum.a", S(ACCUM), 0, 3, "a", "koO", (SUBR)accum_init, (SUBR)accum_perf_audio},


    { "frac2int.i", S(FUNC12), 0, 1, "i", "ii", (SUBR)frac2int},
    { "frac2int.k", S(FUNC12), 0, 2, "k", "kk", NULL, (SUBR)frac2int},

    { "memview.i_table", S(TABALIAS),  0, 1, "i[]", "ioo", (SUBR)tabalias_init},
    { "memview.k_table", S(TABALIAS),  0, 1, "k[]", "ioo", (SUBR)tabalias_init},
    { "memview.i", S(ARRAYVIEW), 0, 1, "i[]", ".[]oo", (SUBR)arrayview_init},
    { "memview.k", S(ARRAYVIEW), 0, 1, "k[]", ".[]oo", (SUBR)arrayview_init},

    { "ref.arr", S(REF_NEW_ARRAY), 0, 1, "i", ".[]o", (SUBR)ref_new_array},
    // { "ref.k_send", S(REF1), 0, 3, "i", "k", (SUBR)ref_scalar_init, (SUBR)ref_scalar_perf},
    // { "ref.a_send", S(REF1), 0, 3, "i", "a", (SUBR)ref_audio_init, (SUBR)ref_audio_perf},

    // { "derefview.arr_i", S(DEREF_ARRAY), 0, 1, "i[]", "io", (SUBR)deref_array},
    // { "derefview.arr_k", S(DEREF_ARRAY), 0, 1, "k[]", "io", (SUBR)deref_array},
    { "deref.arr_i", S(DEREF_ARRAY), 0, 1, "i[]", "io", (SUBR)deref_array},
    { "deref.arr_k", S(DEREF_ARRAY), 0, 1, "k[]", "io", (SUBR)deref_array},

    // { "refread.k_recv", S(REF1), 0, 3, "k", "i", (SUBR)deref_scalar_init, (SUBR)deref_scalar_perf},
    // { "refread.a_recv", S(REF1), 0, 3, "a", "i", (SUBR)deref_audio_init, (SUBR)deref_audio_perf},

    { "refvalid.i", S(REF1), 0, 1, "i", "i", (SUBR)ref_valid_i},
    { "refvalid.k", S(REF1), 0, 3, "k", "k", (SUBR)ref1_init, (SUBR)ref_valid_perf},

    // { "xtracycles.i", S(OPCk_0), 0, 1, "i", "", (SUBR)xtracycles},

    { "throwerror.s", S(ERRORMSG), 0, 3, "", "S", (SUBR)errormsg_init0, (SUBR)errormsg_perf},
    { "throwerror.ss", S(ERRORMSG), 0, 3, "", "SS", (SUBR)errormsg_init, (SUBR)errormsg_perf},
    { "initerror.s", S(ERRORMSG), 0, 1, "", "S", (SUBR)initerror},

    { "setslice.i", S(ARRSETSLICE), 0, 1, "", "i[]ioop", (SUBR)array_set_slice},
    { "setslice.k", S(ARRSETSLICE), 0, 2, "", "k[]kOOP", NULL, (SUBR)array_set_slice},
    { "setslice.a", S(ARRSETSLICE), 0, 2, "", "a[]kOOP", NULL, (SUBR)array_set_slice},

    { "setslice.i[]", S(_AAk), 0, 1, "", "i[]i[]i", (SUBR)setslice_array_k_init_i},
    { "setslice.k[]", S(_AAk), 0, 3, "", "k[]k[]k", (SUBR)setslice_array_k_init_k, (SUBR)setslice_array_k},
    { "setslice.S[]", S(_AAk), 0, 3, "", "S[]S[]k", (SUBR)setslice_array_k_init_S, (SUBR)setslice_array_k},

    { "extendarray.ii", S(_AA), 0, 1, "", "i[]i[]", (SUBR)extendArray_i},
    { "extendarray.ki", S(_AA), 0, 3, "", "k[]i[]", (SUBR)extendArray_init, (SUBR)extendArray_k},
    { "extendarray.kk", S(_AA), 0, 3, "", "k[]k[]", (SUBR)extendArray_init, (SUBR)extendArray_k},
    { "extendarray.SS", S(_AA), 0, 1, "", "S[]S[]", (SUBR)extendArray_i },
    // itab, sr, nchnls, loopstart=0, basenote=60
    { "ftsetparams.i", S(FTSETPARAMS), 0, 1, "", "iiioj", (SUBR)ftsetparams },
    { "perlin3.k_kkk", S(PERLIN3), 0, 3, "k", "kkk", (SUBR)perlin3_init, (SUBR)perlin3_k_kkk},
    { "perlin3.a_aaa", S(PERLIN3), 0, 3, "a", "aaa", (SUBR)perlin3_init, (SUBR)perlin3_a_aaa},

    { "interp1d.k_kK", S(INTERPARR_x_xK), 0, 3, "k", "k.[]", (SUBR)interparr_k_kK_init, (SUBR)interparr_k_kK_kr},
    { "interp1d.k_kKS", S(INTERPARR_x_xK), 0, 3, "k", "k.[]SO", (SUBR)interparr_k_kK_init, (SUBR)interparr_k_kK_kr},

    { "interp1d.a_aKS", S(INTERPARR_x_xK), 0, 3, "a", "a.[]", (SUBR)interparr_k_kK_init, (SUBR)interparr_a_aK_kr},
    { "interp1d.a_aK", S(INTERPARR_x_xK), 0, 3, "a", "a.[]SO", (SUBR)interparr_k_kK_init, (SUBR)interparr_a_aK_kr},

    { "interp1d.i_iI", S(INTERPARR_x_xK), 0, 1, "i", "ii[]", (SUBR)interparr_k_kK_ir},
    { "interp1d.i_iIS", S(INTERPARR_x_xK), 0, 1, "i", "ii[]So", (SUBR)interparr_k_kK_ir},

    { "interp1d.K_KK", S(INTERPARR_K_KK), 0, 3, "k[]", "k[].[]", (SUBR)interparr_K_KK_init_simple, (SUBR)interparr_K_KK_kr},
    { "interp1d.K_KKS", S(INTERPARR_K_KK), 0, 3, "k[]", "k[].[]SO", (SUBR)interparr_K_KK_init, (SUBR)interparr_K_KK_kr},

    { "interp1d.I_II", S(INTERPARR_K_KK), 0, 1, "i[]", "i[]i[]", (SUBR)interparr_K_KK_ir},
    { "interp1d.I_IIS", S(INTERPARR_K_KK), 0, 1, "i[]", "i[]i[]So", (SUBR)interparr_K_KK_ir},

    // kout interptab kin, ktab, Smode='linear', kstep=1, koffset=0
    { "interp1d.k",  S(INTERPTAB), 0, 3, "k", "kkSPO", (SUBR)interptab_init_kkSkk, (SUBR)interptab_kr},
    { "interp1d.k",  S(INTERPTAB), 0, 3, "k", "kk", (SUBR)interptab_init_kk, (SUBR)interptab_kr},

    { "interp1d.i",  S(INTERPTAB), 0, 1, "i", "iiSpo", (SUBR)interptab_ir5},
    { "interp1d.i",  S(INTERPTAB), 0, 1, "i", "ii", (SUBR)interptab_ir2},

    { "interp1d.a",  S(INTERPTAB), 0, 3, "a", "akSPO", (SUBR)interptab_init_kkSkk, (SUBR)interptab_a_a_kr},
    { "interp1d.a",  S(INTERPTAB), 0, 3, "a", "ak", (SUBR)interptab_init_kkSkk, (SUBR)interptab_a_a_kr},


    // TODO : kout[] interp1d kin[], ktab, Smode=0,kstep=1, koffset=0


    { "bisect.k_k", S(BISECT), 0, 3, "k", "k.[]", (SUBR)bisect_init, (SUBR)bisect_kr},
    { "bisect.i_i", S(BISECT), 0, 1, "i", "i.[]", (SUBR)bisect_ir },
    { "bisect.a_a", S(BISECT), 0, 3, "a", "a.[]", (SUBR)bisect_init, (SUBR)bisect_a_a_kr},
    { "bisect.K_K", S(BISECTARR), 0, 3, "k[]", "k[].[]", (SUBR)bisectarr_init, (SUBR)bisectarr_kr},
    { "bisect.I_I", S(BISECTARR), 0, 1, "i[]", "i[].[]", (SUBR)bisectarr_ir},

    { "bisect.tab_k_k", S(BISECTTAB), 0, 3, "k", "kkOO", (SUBR)bisecttab_init, (SUBR)bisecttab_k_k_kr},
    { "bisect.tab_i_i", S(BISECTTAB), 0, 1, "i", "iioo", (SUBR)bisecttab_k_k_ir},
    { "bisect.tab_a_a", S(BISECTTAB), 0, 3, "a", "akOO", (SUBR)bisecttab_init, (SUBR)bisecttab_a_a_kr},
    { "bisect.tab_K_K", S(BISECTTAB_ARR), 0, 3, "k[]", "k[]kOO",
      (SUBR)bisecttabarr_init, (SUBR)bisecttabarr_kr},

    { "bisect.tab_I_I", S(BISECTTAB_ARR), 0, 1, "i[]", "i[]ioo",
      (SUBR)bisecttabarr_ir},

    { "ftfill.i", S(FILLTAB), 0, 1, "i", "m", (SUBR)filltab },
    { "ftnew.i", S(FTNEW), 0, 1, "i", "io", (SUBR)ftnew },
    //  "ftfill.arr_i", S(FTNEWARR), 0, 1, "i", "ii[]o", (SUBR)ftnew_arr },
    { "zeroarray.k", S(ZEROARR), 0, 2, "", "k[]", NULL, (SUBR)zeroarr_perf},
    { "zeroarray.a", S(ZEROARR), 0, 2, "", "a[]", NULL, (SUBR)zeroarr_perf},
    { "zeroarray.i", S(ZEROARR), 0, 1, "", "i[]", (SUBR)zeroarr_perf},
    { "zeroarray.masked_k", S(ZEROARR), 0, 2, "", "a[]k[]", NULL, (SUBR)zeroarr_masked_perf},
    { "zeroarray.masked_i", S(ZEROARR), 0, 2, "", "a[]i[]", NULL, (SUBR)zeroarr_masked_perf},
    { "zeroarray.masked_tab", S(ZEROARR_TAB), 0, 3, "", "a[]k", (SUBR)zeroarr_maskedtab_init, (SUBR)zeroarr_maskedtab_perf},

    { "mixarray.a", S(ZEROARR), 0, 2, "", "a[]ka", NULL, (SUBR)mixarray_perf},

    { "findarray.k", S(FINDARR), 0, 2, "k", ".[]kj", NULL, (SUBR)findarr_perf},
    { "findarray.i", S(FINDARR), 0, 1, "i", "i[]ij", (SUBR)findarr_perf},
    { "findarray.S_i", S(FINDARR_S), 0, 1, "i", "S[]S", (SUBR)findarr_s},
    { "findarray.S_k", S(FINDARR_S), 0, 2, "k", "S[]S", NULL, (SUBR)findarr_s},

    { "ftfind.k", S(FTINDEX), 0, 2, "k", "kkj", NULL, (SUBR)ftindex_perf},
    { "ftfind.i", S(FTINDEX), 0, 1, "i", "iij", (SUBR)ftindex_perf},
    { "loadnpy.k", S(loadnpy_ARR), 0, 1, "k[]", "S", (SUBR)loadnpy},
    { "loadnpy.i", S(loadnpy_ARR), 0, 1, "i[]", "S", (SUBR)loadnpy},
    { "detectsilence.k", S(DETECT_SILENCE), 0, 3, "k", "aJJ",
      (SUBR)detectSilence_init, (SUBR)detectSilence_k_a},
    { "panstereo", S(BALANCE2), 0, 3, "aa", "aakP", (SUBR)balance2_init, (SUBR)balance2_ak},
    { "zerocrossing", S(ZEROCROSSING), 0, 3, "a", "a", 
      (SUBR)zerocrossing_init, (SUBR)zerocrossing_a_a },
    // { "presetinterp", S(PRESETINTERP), 0, 3, "k[]", "kkk[]o",
    //  (SUBR)presetinterp_init, (SUBR)presetinterp_perf },

    { "presetinterp", S(PRESETINTERP), 0, 3, "k[]", "kkk[]o",
      (SUBR)presetinterpw_init, (SUBR)presetinterp_perf },

    { "presetinterp", S(PRESETINTERP), 0, 3, "k[]", "kki[]o",
      (SUBR)presetinterpw_init, (SUBR)presetinterp_perf },

    { "weightedsum", S(ROWSWEIGHTEDSUM), 0, 3, "k[]", "k[]k[]",
      (SUBR)rowsweightedsum_init, (SUBR)rowsweightedsum_perf },
    { "weightedsum", S(ROWSWEIGHTEDSUM), 0, 3, "k[]", "i[]k[]",
      (SUBR)rowsweightedsum_init, (SUBR)rowsweightedsum_perf },

    { "weightedsum", S(ROWSWEIGHTEDSUM), 0, 1, "i[]", "i[]i[]",
      (SUBR)rowsweightedsum_i },

    { "vowelsdb", S(VOWELSDB), 0, 1, "i[]i[]i[]", "SS", (SUBR)vowelsdb_i },
    { "vowelsdb", S(VOWELSDB), 0, 1, "k[]k[]k[]", "SS", (SUBR)vowelsdb_i },
    { "mtro", S(MTRO), 0, 3, "k", "kp", (SUBR)mtro_init, (SUBR)mtro},
    { "nametoinstrnum.i", S(NAMETOINSTRNUM), 0, 1, "i", "S", (SUBR)nametoinstrnum },

    { "nametoinstrnum.k", S(NAMETOINSTRNUM), 0, 2, "k", "S",
        NULL, (SUBR)nametoinstrnum },
    { "cuetrig", S(CUETRIG), 0, 3, "k", "kM", (SUBR)cuetrig_init, (SUBR)cuetrig },

    { "gaintovel.i", S(GAINTOVEL), 0, 1, "i", "iiioo", (SUBR)gaintovel },
    { "gaintovel.k", S(GAINTOVEL), 0, 2, "k", "kkkOO", NULL, (SUBR)gaintovel },

    { "linexp.i", S(LINEXP), 0, 1, "i", "iiiiop", (SUBR)linexp },
    { "linexp.k", S(LINEXP), 0, 2, "k", "kkkkOP", NULL, (SUBR)linexp },

};

LINKAGE


