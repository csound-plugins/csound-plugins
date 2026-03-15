/*
   risset.c

  Copyright (C) 2020 Eduardo Moguillansky

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


*/

#include "csdl.h"
#include <math.h>

#include "../../common/_common.h"
#include "pyinlib.h"


#if defined WIN32 || defined __MINGW32__ || defined _WIN32 || defined _WIN64
    #define OS_WINDOWS
#endif


// -------------------------------------------------------------------------------------

// kfreq, kconf, kvoiced pyin asig, iframesize=2048, ihop=0, if0min=70, if0max=1000, itransweight=0.1,
// kfreq, kconf, kvoiced pyin asig, "framesize", 2048, "hop", 512, "fmin", 70, "fmax", 1000,
static const char *pyin_params[] = {
    "framesize",                      // 0
    "hop",                            // 1
    "fmin",                           // 2
    "fmax",                           // 3
    "bins",                           // 4
    "transition_weight",              // 5
    "beta_a",                         // 6
    "beta_b",                         // 7
    "minrms",                         // 8
    "voiced_obs_floor",               // 9
    "octave_cost",                    // 10
    "subharmonic_thresh",             // 11
    "drift",                          // 12
    NULL
};

static int _key_index(char *key, const char **options) {
    const char *param;
    for(int i=0; i<256; i++) {
        param = options[i];
        if(param == NULL)
            break;
        if(!strcmp(key, param))
            return i;
    }
    return -1;
}

typedef struct {
    OPDS h;
    MYFLT *kfreq;
    MYFLT *kconf;
    MYFLT *kvoiced;

    MYFLT *asig;
    void *ctrls[30];
    PYINContext *pyinctx;

    MYFLT last_freq;
    MYFLT last_conf;
    MYFLT last_voiced;

} PYIN_OPCODE;


static int32_t pyin_init(CSOUND *csound, PYIN_OPCODE *p) {
    PYINConfig cfg = pyin_config_default();
    cfg.sample_rate = LOCAL_SR(p);
    cfg.block_size = LOCAL_KSMPS(p);
    STRINGDAT *key;
    CS_TYPE *cstype;
    int numargs = _GetInputArgCnt(csound, p) - 1;
    if(numargs % 2) {
        INITERRF("Expected event number of arguments, got %d\n", numargs);
        return NOTOK;
    }
    p->last_freq = 0.;
    p->last_conf = 0.;
    p->last_voiced = 0.;
    if(numargs > 0) {
        for(int i=0; i < numargs / 2; i++) {
            cstype = _GetTypeForArg(csound, p->ctrls[i*2]);
            if(cstype->varTypeName[0] != 'S') {
                INITERRF("Expected a string for arg %d, got %s\n", i+1, cstype->varTypeName);
                return NOTOK;
            }
            key = (STRINGDAT*)(p->ctrls[i*2]);
            int paramindex = _key_index(key->data, pyin_params);
            if(paramindex < 0) {
                INITERRF("Unknown parmeter %s. Known parameters: ", key->data);
                for(uint32_t j=0; j < sizeof(pyin_params) / sizeof(pyin_params[0]); j++) {
                    INITERRF("%s, ", pyin_params[j]);
                }
                return NOTOK;
            }
            MYFLT value = *(MYFLT *)(p->ctrls[i*2+1]);
            switch(paramindex) {
            case 0:   // framesize
                cfg.frame_size = (int)value;
                break;
            case 1:   // hop
                cfg.hop_size = (int)value;
                break;
            case 2:   // fmin
                cfg.f0_min = value;
                break;
            case 3:   // fmax
                cfg.f0_max = value;
                break;
            case 4:   // bins
                cfg.cents_per_semitone = (int)value;
                break;
            case 5:   // transition_weight
                cfg.voiced_transition_weight = value;
                break;
            case 6:   // beta_a
                cfg.beta_a = value;
                break;
            case 7:   // beta_b
                cfg.beta_b = value;
                break;
            case 8:   // minrms
                cfg.energy_gate_rms = value;
                break;
            case 9:   // voiced_obs_floor
                cfg.voiced_obs_floor = value;
                break;
            case 10:  // octave_cost
                cfg.octave_cost_weight = value;
                break;
            case 11:  // subharmonic_thresh
                cfg.octave_subharmonic_threshold = value;
                break;
            case 12:  // drift
                cfg.pitch_sigma_cents = value;
                break;
            default:
                INITERRF("Invalid parameter index: %d", paramindex);
                return NOTOK;
            }
        }
        if(cfg.hop_size == 0)
            cfg.hop_size = cfg.frame_size / 4;
    }
    PYINContext *ctx = pyin_create(cfg, NULL, NULL);
    if(!ctx) {
        INITERR("Error while creating PYIN context");
        return NOTOK;
    }
    p->pyinctx = ctx;
    return OK;
}

static int32_t pyin_deinit(CSOUND *csound, PYIN_OPCODE *p) {
    pyin_destroy(p->pyinctx);
    return OK;
}

static int32_t pyin_perf(CSOUND *csound, PYIN_OPCODE *p) {
    PYINResult res;
    float block[512];
    MYFLT *asig = p->asig;
    for(uint32_t i=0; i < LOCAL_KSMPS(p); i++) {
        block[i] = (float)asig[i];
    }
    int hasresult = pyin_process_block(p->pyinctx, block, &res);
    if(hasresult) {
        p->last_freq = res.pitch_hz;
        p->last_conf = res.confidence;
        p->last_voiced = (MYFLT)res.voiced;
    }
    *p->kfreq = p->last_freq;
    *p->kconf = p->last_conf;
    *p->kvoiced = p->last_voiced;
    return OK;
}




// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#define S(x) sizeof(x)

#ifdef CSOUNDAPI6
#else
static OENTRY localops[] = {
  { "pyin2.s", S(PYIN_OPCODE), 0, "kkk", "a*", (SUBR)pyin_init, (SUBR)pyin_perf, (SUBR)pyin_deinit, NULL, 0}
};
#endif

LINKAGE
