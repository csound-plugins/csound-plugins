/*
  poly.c:

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

// This is needed to be built as built-in opcode :-(
#ifndef __BUILDING_LIBCSOUND
#define __BUILDING_LIBCSOUND
#endif

// #include "csdl.h"
#include "/usr/local/include/csound/csdl.h"
#include "find_opcode.h"
#include "arrays.h"
#include <ctype.h>


#define INITERR(m) (csound->InitError(csound, "%s", m))
#define INITERRF(fmt, ...) (csound->InitError(csound, fmt, __VA_ARGS__))
#define PERFERR(m) (csound->PerfError(csound, &(p->h), "%s", m))
#define PERFERRF(fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))

#define FLUSH fflush(stdout)
#define DBG(s)        do{printf("\n>>>  "s"\n"); FLUSH;} while(0)
#define DBGF(fmt,...) do{printf("\n>>>  "fmt"\n", __VA_ARGS__); FLUSH;}while(0)

#define POLY_MAXINPARAMS  20
#define POLY_MAXOUTPARAMS 16
#define POLY_MAXPARAMS    (POLY_MAXINPARAMS + 2 + POLY_MAXOUTPARAMS)


/* These are opcodes which do not work with poly, since they rely
   in different forms of introspection which are not possible here
*/
static const char* poly_blacklist[] = {
    "specdisp",
    NULL
};

// return 1 if str in list, else 0
static int32_t str_in_list(char *str, const char **strlist) {
    int32_t i = 0;
    for(i=0; i<100; ++i) {
        if(strlist[i] == NULL)
            return 0;
        if(strcmp(str, strlist[i]) == 0)
            return 1;
    }
    return 0;
}

/*
    We never create a POLYSTATE per se, we use this structure to access the real
    one expected by the opcode
*/
typedef struct {
    OPDS h;
    MYFLT *out;

    // MYFLT *inargs[POLY_MAXINPARAMS];
    MYFLT *inargs[];
} POLYSTATE1;


/*
    This is a wrapper around an instance of the opcode instantiated by poly
    It holds the internal state (state), and a data array to hold the scalar
    values
*/

typedef struct {
    // the internal state of the opcode
    POLYSTATE1 *state;
    // memory ot hold k values for this instance (state->inargs points here)
    MYFLT indata[POLY_MAXINPARAMS];
    // these mirrors the value in the POLY_a struct
    char *in_signature;
    int32_t num_input_args;
} OPCHANDLE;

/*
    aouts[] poly numinstances:i, opcodename:s, params ...

    params can be anything: k, a, k[] or a[]
    these will be passed directly to the opcode instance and are used to match
    the correct version of the opcode
*/
typedef struct {
    OPDS h;

    ARRAYDAT *outs;     // output array, a[]

    // number of instances of opcode to compute
    MYFLT *inuminst;

    // opcode name
    STRINGDAT *opcode_name;

    // arguments to be passed to the opcode. Each element is itself an array
    void *inargs[POLY_MAXINPARAMS];

    // --------------------- internal ---------------------

    // fixed version of inuminst
    uint32_t num_instances;

    // opcode struct corresponding to opcode_name
    OENTRY *opc;

    // dynamic array of handles, size depends on num_instances
    OPCHANDLE *handles;

    POLYSTATE1 *states;

    // own signature of the poly object, one char per input arg. a for audio,
    // k for scalar, upper case indicates an array (A or K).
    char in_signature[POLY_MAXINPARAMS+1];

    // the same, for output
    char out_signature[POLY_MAXOUTPARAMS];

    char out_type;

    // number of input args passed to the opcode (discarding own args of poly)
    // this indicates the actual size of inargs
    uint32_t num_input_args;

    // number of output args passed to the opcode, which should correspond to
    // opc_numouts
    uint32_t num_output_args;

    // the length of opc->intypes and opc->outypes. The indicate the size of the opc
    // struct we need to control (the rest is internal storage)
    uint32_t opc_numouts;
    uint32_t opc_numins;

    // this holds the OPTXT created for the opcode, to be freed later (it is shared
    // by all instances, so it needs to be created/freed only once)
    OPTXT *optext;
} POLY1;


/*
    Get the number of arguments based on the types

    argtypes: either OPDS->intypes or OPDS->outypes. Arrays are not supported

    returns the number of arguments
*/
static int32_t get_numargs(char *argtypes) {
    // no array args or varargs supported
    return strlen(argtypes);
}

/*
    Generates a signature from the type of the arguments

    args: a pointer to the first argument to be analyzed (cast as void**)
    numargs: the number of arguments (as given, for example, by p->CS_INOCOUNT
    dest: a char buffer to put the signature in (char sig[numargs+1])

    returns OK on success, NOTOK on failure

    NB: This works for any opcode.
*/
static int32_t get_signature(CSOUND *csound, void **args, int32_t numargs,
                             char *dest) {
    CS_TYPE *cstype;
    ARRAYDAT *arr;
    for(int i=0; i < numargs; i++) {
        cstype = csound->GetTypeForArg(args[i]);
        char c = cstype->varTypeName[0];
        switch(c) {
        case 'S':
            dest[i] = 's';
            break;
        case 'c':
            dest[i] = 'i';
            break;
        case 'i':
        case 'k':
        case 'a':
            dest[i] = c;
            break;
        case '[':
            arr = (ARRAYDAT *)args[i];
            c = arr->arrayType->varTypeName[0];
            dest[i] = toupper(c);
            break;
        default:
            return INITERRF("Could not parse signature. idx %d = '%c'", i, c);
        }
    }
    dest[numargs] = '\0';
    return OK;
}

const MYFLT default_unset = FL(-1000);
const MYFLT default_fail = FL(-999);

MYFLT get_default_value(char c) {
    switch(c) {
    case 'a':
    case 'k':
    case 'i':
        return default_unset;
    case 'o':
    case 'O':
    case 'x':
        return 0;
    case 'p':
    case 'P':
        return 1;
    case 'j':
    case 'J':
        return -1;
    case 'q':
        return 10;
    default:
        return default_fail;
    }
}


/*
   Setup the internal state of the handle

   handleidx: which handle (by index) to setup
*/
static int32_t handle_setup(CSOUND *csound, POLY1 *p, uint32_t handleidx) {
    ARRAYDAT *arr;
    uint32_t col, nsmps = CS_KSMPS;
    OPCHANDLE *handle = &(p->handles[handleidx]);
    char c;
    MYFLT default_value;

    for(col = 0; col < p->opc_numins; col++) {
        handle->state->inargs[col] = &(handle->indata[col]);
        c = p->opc->intypes[col];
        default_value = get_default_value(c);
        if(default_value == default_fail)
            return INITERRF(Str("failed to parse signature (%s), char failed: '%c'"),
                            p->opc->intypes, c);
        if(default_value != default_unset)
            handle->indata[col] = default_value;
    }

    for(col=0; col<p->num_input_args; col++) {
        c = p->in_signature[col];
        // point the state args to our internal storage. This gets rewriten by
        // the a-arguments, which point directly to var / array input
        // handle->state->inargs[col] = &(handle->indata[col]);
        switch(c) {
        case 'i':
        case 'k':
            handle->indata[col] = *((MYFLT*)(p->inargs[col]));
            break;
        case 'a':
            // we can safely point to the input arg. since this will not change
            handle->state->inargs[col] = p->inargs[col];
            break;
        case 'I':
            // copy the data to the internal storage
            arr = (ARRAYDAT*)(p->inargs[col]);
            handle->indata[col] = arr->data[handleidx];
        case 'K':
            // we can't set a pointer to a k-array because it might be reallocated
            // and the pointers would be invalid. We point to internal memory and
            // copy the values each k-cycle
            break;
        case 'A':
            // we assume that the an audio array can't be resized
            // (this MUST be documented).
            // TODO: cache the addr of the data pointer inside an array of pointers
            // in the handle. If it changes, data has been realloc'd, so we must
            // reset the pointers
            arr = (ARRAYDAT*)(p->inargs[col]);
            handle->state->inargs[col] = &(arr->data[handleidx*nsmps]);
            break;
        default:
            return INITERRF(Str("poly: unknown signature character %c (%s)"),
                            c, p->in_signature);
        }
    }
    return OK;
}

#define CHECKARR1D(arr, minsize) do {                                         \
    if((arr)->dimensions != 1)                                                \
      return INITERRF(Str("array dimensions (%d) should be 1"),               \
                      (arr)->dimensions);                                     \
    if((uint32_t)(arr)->sizes[0] < minsize) {                                 \
      return INITERRF(Str("input array size (%d) must be >= num. instances (%d)"), \
                      (arr)->sizes[0], minsize);                              \
    }} while(0)

#define CHECKALLOC(ptr, id) if(UNLIKELY((ptr)==NULL)) \
    return INITERRF(Str("Could not alloc %s"), (id))


static int32_t poly1_deinit(CSOUND *csound, POLY1 *p);

// for debugging only
static void debug_dump_opc(OENTRY *opc) {
    printf("dsblksiz: %d\n", opc->dsblksiz);
    printf("opcname: %s\n", opc->opname);
    printf("intypes: %s\n", opc->intypes);
    printf("outtypes: %s\n", opc->outypes);
    OPCODINFO *inm = (OPCODINFO*) opc->useropinfo;
    printf("instno: %d\n", inm->instno);
    printf("intypes inm: %s\n", inm->intypes);
}

static void debug_dump_state(POLYSTATE1 *state, int numargs) {
    for(int i=0; i < numargs; i++) {
        printf("state arg %d = %f \n", i, *state->inargs[i]);
    }
}

static void dump_types(CSOUND *csound, void **args, int32_t numargs) {
    CS_TYPE *cstype;
    for(int i=0; i<numargs; i++) {
        DBGF("inspecting: %d / %d \n", i+1, numargs);
        cstype = csound->GetTypeForArg(args[i]);
        char *typename = cstype->varTypeName;
        DBGF("idx: %d / %d    typename: %s\n", i, numargs, typename);
    }
}

static void str_tolower(char *dest, char *src) {
    uint32_t c = 0;

    while (src[c] != '\0') {
        dest[c] = tolower(src[c]);
        c++;
    }
    dest[c] = 0;
}

static int32_t find_test(CSOUND *csound) {
    OENTRY *opc = find_opcode_new(csound, "oscili", "a", "kk");
    if(opc != NULL) { printf("lo encontre 1 !! \n"); }
    opc = find_opcode_new(csound, "oscili", "a", "ik");
    if(opc != NULL) { printf("lo encontre 2 !! \n"); }
    return OK;
}

static int32_t poly1_init(CSOUND *csound, POLY1 *p) {
    OENTRY *opc;
    int32_t ret, nsmps = CS_KSMPS;
    uint32_t i;
    POLYSTATE1 *state;
    char opc_out_signature[32] = {0};
    char opc_in_signature[64];  // input signature used to find the opcode

    if(str_in_list(p->opcode_name->data, poly_blacklist))
        return INITERRF("Opcode %s not supported", p->opcode_name->data);

    if(p->out_type != 'k' && p->out_type != 'a')
        return INITERRF("Only k or a-type output supported, got %c", p->out_type);

    p->num_input_args = csound->GetInputArgCnt(p) - 2;

    ret = get_signature(csound, p->inargs, p->num_input_args, p->in_signature);
    if(ret == NOTOK)
        return INITERR("poly: could not parse input signature");
    ret = get_signature(csound, &(p->outs), p->num_output_args, p->out_signature);
    if(ret == NOTOK)
        return INITERR("poly: could not parse output signature");
    DBGF("out_signature: %s", p->out_signature);


    // the target signature is just our signature without any arrays (thus lowercase)
    str_tolower(opc_in_signature, p->in_signature);

    if(p->num_input_args != strlen(p->in_signature))
        return INITERRF(Str("arg. count mismatch (num input args: %d, signature: %s"),
                        p->num_input_args, p->in_signature);

    csound->Message(csound, ">>>> poly: own sig: %s len=%ld, sig to find: %s len=%ld\n",
                    p->in_signature, strlen(p->in_signature), opc_in_signature,
                    strlen(opc_in_signature));

    p->num_instances = (int32_t)*(p->inuminst);
    if(p->num_instances < 1)
        return INITERRF(Str("number of instances (%d) must be >= 1"), p->num_instances);

    opc_out_signature[0] = p->out_type;
    opc_out_signature[1] = '\0';
    opc = find_opcode_new(csound, p->opcode_name->data,
                          opc_out_signature, opc_in_signature);
    // find_test(csound);
    if(opc == NULL)
        return INITERRF(Str("Opcode '%s' not found. Sig='%s' len=%ld / '%s' len=%ld"),
                        p->opcode_name->data,
                        opc_out_signature, strlen(opc_out_signature),
                        opc_in_signature, strlen(opc_in_signature));
    p->opc = opc;
    p->opc_numins = get_numargs(opc->intypes);
    p->opc_numouts = get_numargs(opc->outypes);

    if(p->opc_numins > POLY_MAXINPARAMS)
        return INITERRF(Str("opcode has too many input arguments (%d, max=%d"),
                        p->opc_numins, POLY_MAXINPARAMS);

    if(p->opc_numouts != 1 || (opc->outypes[0] != 'a' && opc->outypes[0] != 'k'))
        return INITERRF(Str("opcode should have 1 output of a- or k-type, got %s"),
                        opc->outypes);

    tabensure(csound, p->outs, p->num_instances);

    // create handles for each instance
    p->handles = (OPCHANDLE*)csound->Calloc(csound, p->num_instances*sizeof(OPCHANDLE));
    CHECKALLOC(p->handles, "handles");
    ARRAYDAT *arr;
    for(i=0; i<p->num_input_args; i++) {
        char c = p->in_signature[i];
        // here we don't check k-arrays, since at this time a k-array would not
        // be populated and might not have dimensions/size
        if(c=='I' || c=='A' || c=='S') {
            arr = (ARRAYDAT*) p->inargs[i];
            CHECKARR1D(arr, p->num_instances);
        }
    }

#ifdef POLY_DEBUG
    dump_opc(csound, opc);
#endif

    OPTXT *optext = csound->Calloc(csound, sizeof(OPTXT));
    optext->nxtop = NULL;
    optext->t.oentry = opc;
    optext->t.inArgCount = p->num_input_args;
    optext->t.outArgCount = p->num_output_args;
    optext->t.linenum = p->h.optext->t.linenum;
    optext->t.opcod = opc->opname;
    optext->t.intype = p->h.optext->t.intype;
    p->optext = optext;

    // we allocate all states as an array
    p->states = (POLYSTATE1*)csound->Calloc(csound, (p->num_instances) * (opc->dsblksiz));
    char *states0 = (char *)p->states;
    CHECKALLOC(p->states, "states for handle");

    for(i=0; i < p->num_instances; i++) {
        OPCHANDLE *handle = &(p->handles[i]);
        handle->in_signature = p->in_signature;
        handle->num_input_args = p->num_input_args;

        // state = &(p->states[i]);
        state = (POLYSTATE1 *)(states0 + i*opc->dsblksiz);
        state->h.iopadr = opc->iopadr;
        state->h.opadr = opc->kopadr;
        state->h.insdshead = p->h.insdshead;
        state->h.optext = optext;
        handle->state = state;

        if(p->out_type == 'a') {
            state->out = &(p->outs->data[i*nsmps]);
        } else if(p->out_type == 'k') {
            state->out = &(p->outs->data[i]);
        }

        ret = handle_setup(csound, p, i);
        if(UNLIKELY(ret == NOTOK))
            return INITERR(Str("poly: failed to setup handle"));

#ifdef POLY_DEBUG
        debug_dump_state(handle->state, p->opc_numins);
#endif
        if(opc->iopadr != NULL)
            opc->iopadr(csound, (void*)handle->state);
    }
    return OK;
}

static int32_t poly_1a_init(CSOUND *csound, POLY1 *p) {
    p->num_output_args = 1;
    p->out_type = 'a';
    return poly1_init(csound, p);
}

static int32_t poly_1k_init(CSOUND *csound, POLY1 *p) {
    p->num_output_args = 1;
    p->out_type = 'k';
    return poly1_init(csound, p);
}

// Deinit callback, deallocates everything we allocated
static int32_t poly1_deinit(CSOUND *csound, POLY1 *p) {
    csound->Free(csound, p->handles); p->handles = NULL;
    csound->Free(csound, p->states);  p->states = NULL;
    csound->Free(csound, p->optext);  p->optext = NULL;
    p->opc = NULL;
    return OK;
}

/*
   The performance loop - calls kopadr for all instances of the opcode
*/
static int32_t poly1_perf(CSOUND *csound, POLY1 *p) {
    ARRAYDAT *arr;
    char *own_signature = p->in_signature;
    char c;
    uint32_t col, i, numcols = p->num_input_args;
    if(UNLIKELY(p->opc->kopadr == NULL))
        return PERFERRF(Str("opcode %s has no performance callback!"),
                        p->opcode_name->data);

    // check k-arrays
    for(col=0; col < numcols; col++) {
        c = own_signature[col];
        if(c == 'K') {
            arr = (ARRAYDAT*) p->inargs[col];
            CHECKARR1D(arr, p->num_instances);
        }
    }

    for(i=0; i<p->num_instances; i++) {
        OPCHANDLE *handle = &(p->handles[i]);
        if(UNLIKELY(handle == NULL))
            return PERFERR("poly: handle is NULL!");
        for(col=0; col<numcols; col++) {
            c = own_signature[col];
            if(c == 'K' || c == 'I') {
                // a number array, copy data to internal storage
                arr = (ARRAYDAT*)p->inargs[col];
                handle->indata[col] = arr->data[i];
            } else if (c=='k') {
                handle->indata[col] = *((MYFLT*)(p->inargs[col]));
            }
        }
        p->opc->kopadr(csound, (void*)handle->state);
    }
    return OK;
}


// ----------------------------------------------------------------------------------


typedef struct {
    OPDS h;
    void *args[POLY_MAXPARAMS];

    // --------------------- internal ---------------------

    // fixed version of inuminst
    uint32_t num_instances;

    // opcode struct corresponding to opcode_name
    OENTRY *opc;

    // dynamic array of handles, size depends on num_instances
    OPCHANDLE *handles;

    POLYSTATE1 *states;

    // own signature of the poly object, one char per input/output arg. a for audio,
    // k for scalar, upper case indicates an array (A or K).
    char out_signature[POLY_MAXOUTPARAMS];
    char in_signature[POLY_MAXINPARAMS+1];

    // number of input args passed to the opcode (discarding own args of poly)
    // this indicates the actual size of in_signature. num_input_args <= opc_numins
    uint32_t num_input_args;

    // number of output args passed to the opcode, which should correspond to
    // opc_numouts
    uint32_t num_output_args;

    // the length of opc->intypes and opc->outypes. The indicate the size of the opc
    // struct we need to control (the rest is internal storage)
    uint32_t opc_numouts;
    uint32_t opc_numins;

    // this holds the OPTXT created for the opcode, to be freed later (it is shared
    // by all instances, so it needs to be created/freed only once)
    OPTXT *optext;
} POLY;



#define S(s) sizeof(s)

static OENTRY localops[] = {
    { "poly", S(POLY1), 0, 3, "a[]", "iS*", (SUBR)poly_1a_init, (SUBR)poly1_perf },
    { "poly", S(POLY1), 0, 3, "k[]", "iS*", (SUBR)poly_1k_init, (SUBR)poly1_perf },
};

LINKAGE
