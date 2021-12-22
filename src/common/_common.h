#ifndef _COMMON_H
#define _COMMON_H

#include "csdl.h"


#define INITERR(m) (csound->InitError(csound, "%s", m))
#define INITERRF(fmt, ...) (csound->InitError(csound, fmt, __VA_ARGS__))
// VL this is needed for -Wformat-security
#define MSG(m) (csound->Message(csound, "%s", m))
#define MSGF(fmt, ...) (csound->Message(csound, fmt, __VA_ARGS__))
#define PERFERR(m) (csound->PerfError(csound, &(p->h), "%s", m))
#define PERFERRF(fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))


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


#endif
