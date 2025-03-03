#ifndef _COMMON_H
#define _COMMON_H

#include "csdl.h"
#include "arrays.h"


#define INITERR(m) (csound->InitError(csound, "%s", m))
#define INITERRF(fmt, ...) (csound->InitError(csound, fmt, __VA_ARGS__))
// VL this is needed for -Wformat-security
#define MSG(m) (csound->Message(csound, "%s", m))
#define MSGF(fmt, ...) (csound->Message(csound, fmt, __VA_ARGS__))
#define PERFERR(m) (csound->PerfError(csound, &(p->h), "%s", m))
#define PERFERRF(fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))

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
int em_isnan(MYFLT d) {
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
int em_isinf(MYFLT d) {
  union {
    unsigned long long l;
    double d;
  } u;
  u.d=d;
  return (u.l==0x7FF0000000000000ll?1:u.l==0xFFF0000000000000ll?-1:0);
}

static inline
int em_isinfornan(MYFLT d) {
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


// Functions to deal with both csound6 and csound7 in the same codebase

static inline FUNC * FTFind(CSOUND *csound, MYFLT *num) {
#ifdef CSOUNDAPI7
    return csound->FTFind(csound, num);
#else
    return csound->FTnp2Find(csound, num);
#endif
}

static inline int32_t _GetInputArgCnt(CSOUND *csound, void *p) {
#ifdef CSOUNDAPI7
    IGN(csound);
    return GetInputArgCnt((OPDS *)p);
#else
    return csound->GetInputArgCnt((OPDS*)p);
#endif
}

static inline int32_t _GetOutputArgCnt(CSOUND *csound, void *p) {
#ifdef CSOUNDAPI7
    IGN(csound);
    return GetOutputArgCnt((OPDS *)p);
#else
    return csound->GetOutputArgCnt((OPDS *)p);
#endif
}

static inline char* _GetInputArgName(CSOUND *csound, void *p, uint32_t idx) {
#ifdef CSOUNDAPI7
    IGN(csound);
    return GetInputArgName((OPDS *)p, idx);
#else
    return csound->GetInputArgName((OPDS *)p, idx);
#endif
}

// static inline MYFLT _currentTime(CSOUND *csound, OPDS *ctx) {
//     size_t numcycles = CS_KCNT;
// }

static inline void InsertScoreEventNow(CSOUND *csound, EVTBLK *evt, OPDS *ctx) {
#ifdef CSOUNDAPI7
    MYFLT sr = GetLocalSr(ctx);
    csound->InsertScoreEvent(csound, evt, csound->GetCurrentTimeSamples(csound) / sr);
#else
    IGN(ctx);
    csound->insert_score_event_at_sample(csound, evt, csound->GetCurrentTimeSamples(csound));
#endif
}

static inline int32_t InsertScoreEventAtTime(CSOUND *csound, EVTBLK *evt, MYFLT time) {
#ifdef CSOUNDAPI7
    return csound->InsertScoreEvent(csound, evt, time);
#else
    uint64_t sample = time * csound->GetSr(csound);
    return csound->insert_score_event_at_sample(csound, evt, sample);
#endif

}


static inline void tabinit_compat(CSOUND *csound, ARRAYDAT *p, int32_t size, OPDS *ctx) {
#ifdef CSOUNDAPI7
    tabinit(csound, p, size, ctx->insdshead);
#else
    IGN(ctx);
    tabinit(csound, p, size);
#endif
}

static inline int32_t
_createTable(CSOUND *csound, FUNC **ftp, const EVTBLK * ftevt, int32_t n) {
#ifdef CSOUNDAPI7
    return csound->FTCreate(csound, ftp, ftevt, n);
#else
    return csound->hfgens(csound, ftp, ftevt, n);
#endif
}

static inline MYFLT _GetLocalSr(CSOUND *csound, OPDS *ctx) {
#ifdef CSOUNDAPI7
    IGN(csound);
    return GetLocalSr(ctx);
#else
    IGN(ctx);
    return csound->GetSr(csound);
#endif
}

static inline MYFLT _GetLocalKsmps(CSOUND *csound, OPDS *ctx) {
#ifdef CSOUNDAPI7
    IGN(csound);
    return GetLocalKsmps(ctx);
#else
    IGN(ctx);
    return csound->GetKsmps(csound);
#endif
}


#ifdef CSOUNDAPI7
#define LOCAL_SR(p) (GetLocalSr(&(p->h)))
#define LOCAL_KR(p) (GetLocalKr(&(p->h)))
#define LOCAL_KSMPS(p) (GetLocalKsmps(&(p->h)))
#define GET_INPUT_ARG_CNT (GetInputArgCnt(&(p->h)))
#define OPDS_INITFUNC(opds) (opds->init)
#define OPDS_PERFFUNC(opds) (opds->perf)
#else
#define LOCAL_SR(p) (csound->GetSr(csound))
#define LOCAL_KR(p) (csound->GetKr(csound))
#define LOCAL_KSMPS(p) (csound->GetKsmps(csound))
#define GET_INPUT_ARG_CNT (csound->GetInputArgCnt(p))
#define OPDS_INITFUNC(opds) (opds->iopadr)
#define OPDS_PERFFUNC(opds) (opds->kopadr)
#endif

static inline int32_t _StringArg2Insno(CSOUND *csound, char *arg, int32_t isstr) {
#ifdef CSOUNDAPI7
    return csound->StringArg2Insno(csound, arg, isstr);
#else
    return csound->strarg2insno(csound, arg, isstr);
#endif
}
static inline CS_VARIABLE* arrayCreateVariableSameType(CSOUND *csound, ARRAYDAT *arr, OPDS *ctx) {
#ifdef CSOUNDAPI7
    return arr->arrayType->createVariable(csound, NULL, ctx->insdshead);
#else
    IGN(ctx);
    return arr->arrayType->createVariable(csound, NULL);
#endif
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

static inline CS_TYPE *_GetTypeForArg(CSOUND *csound, void *argptr) {
#ifdef CSOUNDAPI7
    IGN(csound);
    return GetTypeForArg(argptr);
#else
    return csound->GetTypeForArg(argptr);
#endif
}

static inline const OENTRY* _FindOpcode(CSOUND *csound, char *name, char *outsig, char *insig) {
#ifdef CSOUNDAPI7
    // exact = 0
    return csound->FindOpcode(csound, 0, name, outsig, insig);
#else
    // not exact
    return csound->find_opcode_new(csound, name, outsig, insig);
#endif
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
// LOSCAL is 1/LOFACT as MYFLT
// #define LOSCAL     FL(0.0009765625)
// #define LOMASK     1023

#ifndef LOBITS
#define LOBITS  10
#endif

#ifndef LOFACT
#define LOFACT 1024
#endif

#endif
