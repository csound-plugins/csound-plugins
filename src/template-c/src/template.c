/*
   libname.c

  Copyright (C) 2019 Name Surname

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

  Description
  
  # opcodeA

    ihandle dict_new Stype [, iglobal]

    lorem ipsum
    
  # opcodeB
  
    signature
    
    lorem ipsum
    
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

// #define DEBUG

#ifdef DEBUG
    #define DBG(fmt, ...) printf(">>>> "fmt"\n", __VA_ARGS__); fflush(stdout);
    #define DBG_(m) DBG("%s", m)
#else
    #define DBG(fmt, ...)
    #define DBG_(m)
#endif


// ----------------- opcode A ------------------

typedef struct {
    OPDS h;
    // outputs
    // MYFLT *out;
    // inputs
    // MYFLT *in;
    
    // internal
    // MYFLT data;
} OPCODEA;

#define S(x) sizeof(x)

static OENTRY localops[] = {
	// { "opcodeA", S(OPCODEA), 0, 3, "a", "aS", (SUBR)opcodeA_0, (SUBR)opcodeA },
    
};

LINKAGE
