#include "csdl.h"


typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *in0;
    MYFLT *in1;
    MYFLT *in2;
} TESTOPCODE;


static int32_t testopcode_init(CSOUND *csound, TESTOPCODE *p) {
    IGN(csound);
    printf("Addresses in0: %p, in1: %p, in2: %p\n", (void*)p->in0, (void*)p->in1, (void*)p->in2);
    printf("Values in0: %f, in1: %f, in2: %f\n", *p->in0, *p->in1, *p->in2);
    *p->out = (*p->in0 + *p->in1) * *p->in2;
    return OK;
}


#define S(x) sizeof(x)

static OENTRY localops[] = {
#ifdef CSOUNDAPI6
    {"testopcode.i", S(TESTOPCODE), 0, 1, "i", "iii", (SUBR)testopcode_init, NULL, NULL, NULL}

#else
    {"testopcode.i", S(TESTOPCODE), 0, "i", "iii", (SUBR)testopcode_init, NULL, NULL, NULL}
#endif
};

LINKAGE

