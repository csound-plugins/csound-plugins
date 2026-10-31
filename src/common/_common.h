#ifndef _COMMON_H
#define _COMMON_H

#include "csound_compat.h"
#include "arrays.h"


#define INITERR(m) (csound->InitError(csound, "%s", m))
#define INITERRF(fmt, ...) (csound->InitError(csound, fmt, __VA_ARGS__))
// VL this is needed for -Wformat-security
#define MSG(m) (csound->Message(csound, "%s", m))
#define MSGF(fmt, ...) (csound->Message(csound, fmt, __VA_ARGS__))
#define PERFERR(m) (csound->PerfError(csound, &(p->h), "%s", m))
#define PERFERRF(fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))

// Report an error from a helper that can run at init (mode == 1) or at
// performance time (any other mode), so the message is written only once.
#define INITPERFERR(mode, m) ((mode) == 1 ? INITERR(m) : PERFERR(m))
#define INITPERFERRF(mode, fmt, ...) \
    ((mode) == 1 ? INITERRF(fmt, __VA_ARGS__) : PERFERRF(fmt, __VA_ARGS__))

#define DBG(s)        do{printf("\n>>>  " s "\n"); fflush(stdout);} while(0)
#define DBGF(fmt,...) do{printf("\n>>>  " fmt "\n", __VA_ARGS__); fflush(stdout);}while(0)


#define CHECKARR1D(arr)           \
    if((arr)->dimensions != 1)    \
        return INITERRF(Str("Array should be of 1D, but has %d dimensions"), \
                        (arr)->dimensions);

// This must be called for each array at each perf pass
// (at init call tabinit)
#define ARRAY_ENSURESIZE_PERF(csound, arr, size) tabcheck(csound, arr, size, &(p->h))


/*
#define TABENSURE_PERF(csound, arr, size) {
  if(UNLIKELY(arr->sizes[0] != size)) {                                 \
        size_t _ss = arr->arrayMemberSize*size;                           \
        if (_ss > arr->allocated) {                                       \
            return PERFERRF(Str("Array is too small (allocated %zu "      \
                                "< needed %zu), but cannot allocate  "    \
                                "during performance pass. Allocate a "    \
                                "bigger array "at init time"),            \
                            arr->allocated, _ss);                         \
        }                                                                 \
        arr->sizes[0] = size;                                             \
    }}

*/


static inline
int em_isnan(cs_float d) {
  union {
    unsigned long long l;
    double d;
  } u;
  u.d=d;
  return (u.l==0x7FF8000000000000ll ||
          u.l==0x7FF0000000000000ll ||
          u.l==0xfff8000000000000ll);
}

static inline
int em_isinf(cs_float d) {
  union {
    unsigned long long l;
    double d;
  } u;
  u.d=d;
  return (u.l==0x7FF0000000000000ll?1:u.l==0xFFF0000000000000ll?-1:0);
}

static inline
int em_isinfornan(cs_float d) {
    union {
      unsigned long long l;
      double d;
    } u;
    u.d=d;
    return (u.l==0x7FF8000000000000ll ||
            u.l==0x7FF0000000000000ll ||
            u.l==0xfff8000000000000ll ||
            u.l==0xFFF0000000000000ll);
}


// Helpers for the Csound 7 API

static inline FUNC * FTFind(CSOUND *csound, cs_float *num) {
    return csound->FTFind(csound, num);
}

static inline int32_t _GetInputArgCnt(CSOUND *csound, void *p) {
    IGN(csound);
    return GetInputArgCnt((OPDS *)p);
}

static inline int32_t _GetOutputArgCnt(CSOUND *csound, void *p) {
    IGN(csound);
    return GetOutputArgCnt((OPDS *)p);
}

static inline char* _GetInputArgName(CSOUND *csound, void *p, uint32_t idx) {
    IGN(csound);
    return GetInputArgName((OPDS *)p, idx);
}

// static inline cs_float _currentTime(CSOUND *csound, OPDS *ctx) {
//     size_t numcycles = CS_KCNT;
// }

static inline void InsertScoreEventNow(CSOUND *csound, EVTBLK *evt, OPDS *ctx) {
    cs_float sr = GetLocalSr(ctx);
    // csound->InsertScoreEvent(csound, evt, csound->GetCurrentTimeSamples(csound) / sr);
    csound->Event(csound, 0, evt->p, evt->pcnt);
}


static inline void tabinit_compat(CSOUND *csound, ARRAYDAT *p, int32_t size, const OPDS *ctx) {
    tabinit(csound, p, size, ctx->insdshead);
}

static inline int32_t
_createTable(CSOUND *csound, FUNC **ftp, const EVTBLK * ftevt, int32_t n) {
    return csound->FTCreate(csound, ftp, ftevt, n);
}

static inline cs_float _GetLocalSr(CSOUND *csound, OPDS *ctx) {
    IGN(csound);
    return GetLocalSr(ctx);
}

static inline cs_float _GetLocalKsmps(CSOUND *csound, OPDS *ctx) {
    IGN(csound);
    return GetLocalKsmps(ctx);
}


#define LOCAL_SR(p) (GetLocalSr(&(p->h)))
#define LOCAL_KR(p) (GetLocalKr(&(p->h)))
#define LOCAL_KSMPS(p) (GetLocalKsmps(&(p->h)))
#define GET_INPUT_ARG_CNT (GetInputArgCnt(&(p->h)))
#define OPDS_INITFUNC(opds) (opds->init)
#define OPDS_PERFFUNC(opds) (opds->perf)

static inline int32_t _StringArg2Insno(CSOUND *csound, char *arg, int32_t isstr) {
    return csound->StringArg2Insno(csound, arg, isstr);
}
static inline CS_VARIABLE* arrayCreateVariableSameType(CSOUND *csound, const ARRAYDAT *arr, const OPDS *ctx) {
    return arr->arrayType->createVariable(csound, arr->arrayType, NULL, ctx->insdshead);
}

INSTRTXT *GetInstrumentByName(CSOUND *csound, const char *name) {
    INSTRTXT **list = csound->GetInstrumentList(csound);
    INSTRTXT *instr = list[0];
    while((instr = instr->nxtinstxt) != NULL) {
        if(strcmp(name, instr->insname) == 0)
            return instr;
    }
    return NULL;
}

static inline INSTRTXT *GetInstrumentByNumber(CSOUND *csound, int32_t n) {
    INSTRTXT **list = csound->GetInstrumentList(csound);

    return list[n];
}

static inline const CS_TYPE *_GetTypeForArg(CSOUND *csound, void *argptr) {
    IGN(csound);
    return GetTypeForArg(argptr);
}

static inline const OENTRY* _FindOpcode(CSOUND *csound, char *name, char *outsig, char *insig) {
    // exact = 0
    return csound->FindOpcode(csound, 0, name, outsig, insig);
}


static inline float fastlog2 (float x) {
  union { float f; uint32_t i; } vx = { x };
  union { uint32_t i; float f; } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
  float y = vx.i;
  y *= 1.1920928955078125e-7f;

  return y - 124.22551499f
           - 1.498030302f * mx.f
           - 1.72587999f / (0.3520887068f + mx.f);
}

static inline float fastlog2b(float x) {
    uint32_t ix;
    memcpy(&ix, &x, sizeof(ix));
    float y  = (float)ix * 1.1920928955078125e-7f - 126.94269504f;
    float mx = (float)((ix & 0x007FFFFFu) | 0x3f000000u);
    mx -= 1.0f;
    y  += mx * (1.0f + mx * (-0.5f + mx * 0.333333f));
    return y;
}

static inline float fastlogf (float x) {
    return 0.69314718f * fastlog2 (x);
}

static inline cs_float fastlog(cs_float x) {
    return FL(0.6931471805599453) * fastlog2(x);
}



char * _strncpy(char *dst, const char *src, size_t siz) {
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit or until NULL */
    if (n != 0) {
        while (--n != 0) {
            if ((*d++ = *s++) == '\0')
                break;
        }
    }

    /* Not enough room in dst, add NUL */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';                /* NUL-terminate dst */

        //while (*s++) ;
    }
    return dst;        /* count does not include NUL */
}


// #define LOBITS     10
// #define LOFACT     1024
// LOSCAL is 1/LOFACT as cs_float
// #define LOSCAL     FL(0.0009765625)
// #define LOMASK     1023

#ifndef LOBITS
#define LOBITS  10
#endif

#ifndef LOFACT
#define LOFACT 1024
#endif

#endif
