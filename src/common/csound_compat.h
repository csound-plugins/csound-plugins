#ifndef CSOUND_COMPAT_H
#define CSOUND_COMPAT_H

#include "csdl.h"

#ifdef CSOUNDAPI6
  #define cs_float MYFLT
  #define cs_double double
  #define cs_modf modf
#endif

#endif
