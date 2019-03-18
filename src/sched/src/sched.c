/*
   sched.c

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

  Description
  
  # atstop

    atstop "instrname", idelay, idur [, pargs...]
    atstop instrnum, idelay, idur [, pargs...]

    Schedule an instrument when this note stops


*/

#include "csdl.h"
// #include "arrays.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

#define INITERR(m) (csound->InitError(csound, "%s", m))
#define INITERRF(fmt, ...) (csound->InitError(csound, fmt, __VA_ARGS__))
#define PERFERR(m) (csound->PerfError(csound, &(p->h), "%s", m))
#define PERFERRF(fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))

#define register_deinit(csound, p, func) \
    csound->RegisterDeinitCallback(csound, p, (int32_t(*)(CSOUND*, void*))(func))

#define UI32MAX 0x7FFFFFFF

#define DEBUG

#ifdef DEBUG
    #define DBG(fmt, ...) printf(">>>> "fmt"\n", __VA_ARGS__); fflush(stdout);
    #define DBG_(m) DBG("%s", m)
#else
    #define DBG(fmt, ...)
    #define DBG_(m)
#endif

typedef int32_t i32;
typedef uint32_t ui32;
typedef int64_t i64;
typedef uint64_t ui64;


/** ----------------- atstop ---------------

  schedule an instrument when the note stops

  NB: this is scheduled not at release start, but when the
  note is deallocated

  atstop Sintr, idelay, idur, pfields...
  atstop instrnum, idelay, idur, pfields...

*/

#define MAXPARGS 31

typedef struct {
    OPDS h;
    // outputs

    // inputs
    void *instr;
    MYFLT *pargs[MAXPARGS];

    // internal
    MYFLT instrnum;   // cached instrnum

} SCHED_DEINIT;


static i32
atstop_deinit(CSOUND *csound, SCHED_DEINIT *p) {
    EVTBLK evt;
    memset(&evt, 0, sizeof(EVTBLK));
    evt.opcod = 'i';
    evt.strarg = NULL;
    evt.scnt = 0;
    evt.pinstance = NULL;
    evt.p2orig = *p->pargs[0];
    evt.p3orig = *p->pargs[1];
    evt.pcnt = p->INOCOUNT;
    evt.p[1] = p->instrnum;
    for(i32 i=0; i < evt.pcnt-1; i++) {
        evt.p[2+i] = *p->pargs[i];
    }
    csound->insert_score_event_at_sample(csound, &evt, csound->GetCurrentTimeSamples(csound));
    return OK;
}

static i32
atstop_(CSOUND *csound, SCHED_DEINIT *p, MYFLT instrnum) {
    p->instrnum = instrnum;
    register_deinit(csound, p, atstop_deinit);
    return OK;
}

static i32
atstop_i(CSOUND *csound, SCHED_DEINIT *p) {
    MYFLT instrnum = *((MYFLT*)p->instr);
    return atstop_(csound, p, instrnum);
}

static i32
atstop_s(CSOUND *csound, SCHED_DEINIT *p) {
    STRINGDAT *instrname = (STRINGDAT*) p->instr;
    i32 instrnum = csound->strarg2insno(csound, instrname->data, 1);
    if (UNLIKELY(instrnum == NOT_AN_INSTRUMENT)) return NOTOK;
    return atstop_(csound, p, (MYFLT) instrnum);
}

#define S(x) sizeof(x)

static OENTRY localops[] = {
    {"atstop.s", S(SCHED_DEINIT), 0, 1, "", "Siiz", (SUBR)atstop_s },
    {"atstop.i", S(SCHED_DEINIT), 0, 1, "", "iiiz", (SUBR)atstop_i }
    
};

LINKAGE
