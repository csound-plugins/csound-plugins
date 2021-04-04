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

/*
  This file implements the opcode 'poly', which enables to create and
  control multiple versions of the same opcode, each with its state,
  inputs and outputs.

  In general, each opcode has a signature, given by the number and types of its
  output and input arguments. For example, the opcode "oscili", as used
  like "aout oscili kamp, kfreq", has a signature a/kk (a as output, kk as input)

  To follow the example, to create 10 parallel versions of this opcode, it is
  possible to use poly like this:

  kFreqs[] fillarray ...
  aOut[] poly 10, "oscili", 0.1, kFreqs[]

  * kFreqs holds the frequencies of the oscillators. Changing its contents will
    modify the frequency of the oscillators.
  * For each input argument it possible to define either an array of sufficient size
    (size >= num instances) or a scalar value, which will be used for all the
    instances of the opcode
  * default arguments are handled in the same way as when calling the opcode directly

 */

// This is needed to be built as built-in opcode :-(
/*
#ifndef __BUILDING_LIBCSOUND
#define __BUILDING_LIBCSOUND
#endif
*/

#include "csdl.h"
#include "arrays.h"
#include <ctype.h>


#define INITERR(m) (csound->InitError(csound, "%s", m))
#define INITERRF(fmt, ...) (csound->InitError(csound, fmt, __VA_ARGS__))
#define PERFERR(m) (csound->PerfError(csound, &(p->h), "%s", m))
#define PERFERRF(fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))

#define DBG(s)        do{printf("\n>>>  "s"\n"); fflush(stdout);} while(0)
#define DBGF(fmt,...) do{printf("\n>>>  "fmt"\n", __VA_ARGS__); fflush(stdout);}while(0)

#define POLY_MAXINPARAMS  31
#define POLY_MAXOUTPARAMS 15
#define POLY_MAXPARAMS    (POLY_MAXINPARAMS + 2 + POLY_MAXOUTPARAMS)



// assume called within a context where csound and p are defined
#define ARRAY_ENSURESIZE(arr, size) tabcheck(csound, arr, size, &(p->h))

typedef int32_t i32;
typedef uint32_t ui32;

#define register_deinit(csound, p, func) \
    csound->RegisterDeinitCallback(csound, p, (int32_t(*)(CSOUND*, void*))(func))


// --------------------------------------------
//               Utilities
// --------------------------------------------

static inline int max(int a, int b) {
    return a > b ? a : b;
}

/** put a copy of src in dest with all chars converted to uppercase */
static void str_tolower(char *dest, char *src) {
    ui32 c = 0;

    while (src[c] != '\0') {
        dest[c] = (char)tolower(src[c]);
        c++;
    }
    dest[c] = 0;
}

/** Return 1 if all characters in s are uppercase, 0 otherwise */
static int all_upper(char *s) {
    while (*s) {
        char c = *s;
        if(c >= 'a' && c <= 'z')
            return 0;
        s++;
    }
    return 1;
}

/** Return 1 if all characters in s are lowercase, 0 otherwise */
static int all_lower(char *s) {
    while (*s) {
        char c = *s;
        if(c >= 'A' && c <= 'Z')
            return 0;
        s++;
    }
    return 1;
}

/** Return the index of c in s, or -1 if c is not found */
static i32 findchar(char *s, char c) {
    i32 idx = 0;
    while(*s) {
        char _c = *s;
        if(_c == c)
            return idx;
        s++;
        idx++;
    }
    return -1;
}

/** return 1 if str in list, 0 otherwise
 *
 * (used to check blacklist)
 *
 */
static i32 str_in_list(char *str, const char **strlist) {
    for(ui32 i=0; i<100; ++i) {
        if(strlist[i] == NULL)
            return 0;
        if(strcmp(str, strlist[i]) == 0)
            return 1;
    }
    return 0;
}


#define str_for_each(s, charname, block) {  \
    char *_s = (s);                         \
    while(*_s) {                            \
        char charname = *_s;                \
        block;                              \
        _s++;                               \
    }}


/** A list of opcodes which do not work properly with poly */
static const char* poly_blacklist[] = {
    "specdisp",
    NULL
};


/**
  A structure to access the opcode state in a generic way. This
  is a struct valid for any opcode. Given that we now the number
  of outputs and inputs, we know the number of pointers to vars
  and the size of its internal state.
  We never create an OPCSTATE per se, we allocate a chunk of
  memory of the correct size as indicated by the opcode's OENTRY,
  and then pass this chunk to the registered i- and k- functions.
*/

typedef struct {
    OPDS h;
    void *args[];
} OPCSTATE;


/**
  This is a wrapper around an instance of the opcode instantiated by poly
  It holds the internal state (state), and a data array to hold the scalar
  values. We point the inputs of the state struct here to indata, so by
  changing indata we set the state of the opcode's instance. In a real
  opcode instance, the input arguments would point to a variable, but
  since we are handling all the state ourselves, there are no such variables.
  Pointing the state to memory we own allows to set default values.
*/

typedef struct {
    // the internal state of the opcode
    OPCSTATE *state;
    // memory ot hold k values for this instance
    MYFLT indata[POLY_MAXINPARAMS];
} OPCHANDLE;

/**
 * This is the main structure of the poly object
 *
 * aOut[] poly numinstances:i, opcodename:s, params ...
 * kOut[] poly numinstances:i, opcodename:s, params ...
 *
 * params can be anything: k, a, k[] or a[]
 *
 * For arrays, they need to be at least the size of `numinstances`
 * Scalar and audio inputs are "shared" by all instances
 *
 * The opcode used is derived of the types used as outputs and inputs
 * to the poly object. For example
 *
 *   kfreqs[] fillarray 500, 600, 700
 *   aOut[] poly 3, "oscili", 0.5, kfreqs
 *
 *   will search for an opcode "oscili" with signature "a", "ik"
 *
 * Any number of outputs and inputs are supported, as long as they match the
 * signature of the given opcode. All outputs need to be arrays. These arrays don't
 * need to be instantiated. They will be 1D arrays of size `numinstances`, holding
 * the output to the opcode instance corresponding to each index.
 *
 */
typedef struct {
    OPDS h;

    void *args[POLY_MAXPARAMS];

    // -------------------- internal ---------------------

    // this points to the input arg inside args holding the opcode name
    STRINGDAT *opcode_name;

    // points to p->args[p->num_output_args] (the start of the input args)
    void **inargs;

    // fixed version of inuminst
    ui32 num_instances;

    // opcode struct corresponding to opcode_name
    OENTRY *opc;

    // array of handles, size depends on num_instances. Each instance has a handle
    // which encapsulates its state. We allocate all handles at once, each handle
    // points to the corresponding item
    OPCHANDLE *handles;

    // an array of OPCSTATE. An OPCSTATE corresponds to the internal state of
    // each instance.
    OPCSTATE *states;

    // start idx of inargs which needs to be multiplexed.
    // firstcol=0 for poly, firstcol=num_output_args for polyseq
    ui32 firstcol;

    // number of input args passed to the opcode (discarding own args of poly)
    ui32 num_input_args;

    // number of output args passed to the opcode, which should correspond to
    // opc_numouts
    ui32 num_output_args;

    // input signature of the poly object (without numinstances and Sopcodename)
    // One char per input arg: a for audio, k for scalar, upper case indicates
    // an array (A or K).
    char in_signature[POLY_MAXINPARAMS+1];

    // the same, for output
    char out_signature[POLY_MAXOUTPARAMS+1];


    // the length of opc->intypes and opc->outypes. The indicate the size of the opc
    // struct we need to control (the rest is internal storage)
    ui32 opc_numouts;
    ui32 opc_numins;
    i32 is_parallel;

    // this holds the OPTXT created for the opcode, to be freed later (it is shared
    // by all instances, so it needs to be created/freed only once)
    OPTXT *optext;
} POLY1;



/** Get the number of arguments based on the types
 *
 * argtypes: either OPDS->intypes or OPDS->outypes. Arrays are not supported
 *
 * returns the number of arguments
 */
static ui32 get_numargs(char *argtypes) {
    // no array args or varargs supported
    return (ui32) strlen(argtypes);
}

/** Generates a signature from the type of the arguments
 *
 * args: a pointer to the first argument to be analyzed (cast as void**)
 * numargs: the number of arguments (as given, for example, by p->CS_INOCOUNT
 * dest: a char buffer to put the signature in (char sig[numargs+1])
 *
 * returns OK on success, NOTOK on failure
 *
 * NB: This works for any opcode.
*/
static i32
get_signature(CSOUND *csound, void **args, ui32 numargs, char *dest) {
    CS_TYPE *cstype;
    ARRAYDAT *arr;
    for(ui32 i=0; i < numargs; i++) {
        cstype = csound->GetTypeForArg(args[i]);
        char c = cstype->varTypeName[0];
        switch(c) {
        case 'S':
            dest[i] = 's';
            break;
        case 'i':
        case 'c':
        case 'j':
        case 'p':
        case 'q':
        case 'h':
        case 'v':
            dest[i] = 'i';
            break;
        case 'k':
        case 'O':
        case 'J':
        case 'P':
            dest[i] = 'k';
            break;
        case 'a':
            dest[i] = 'a';
            break;
        case '[':
            arr = (ARRAYDAT *)args[i];
            c = arr->arrayType->varTypeName[0];
            dest[i] = (char)toupper(c);
            break;
        case 'Z':
            // alternating k and a
            for(ui32 j=i; j < numargs; j++) {
                dest[j] = (j-i)%2 == 0 ? 'k' : 'a';
            }
            break;
        case 'z':
        case 'M':
            // multiple inputs of k-type
            // set this and all remaining args to 'k'
            for(ui32 j=i; j < numargs; j++) {
                dest[j] = 'k';
            }
            break;
        case 'm':
            // multiple inputs, i-type
            // set this and all remaining args to 'k'
            for(ui32 j=i; j < numargs; j++) {
                dest[j] = 'i';
            }
            break;

        default:
            return INITERRF("Could not parse signature. idx %d = '%c'", i, c);
        }
    }
    dest[numargs] = '\0';
    return OK;
}

static const i32 default_unset = -1000;
static const i32 default_fail  = -999;

/** Returns the default value corresponding to a given char in a signature */
MYFLT get_default_value(char c) {
    switch(c) {
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
    case 'a':
    case 'k':
    case 'i':
    case 'S':
    case 'm':
    case 'M':
    case 'N':
    case 'Z':
    case '*':
    case '?':
        return (MYFLT)default_unset;
    default:
        return (MYFLT)default_fail;
    }
}


/**
   Setup the internal state of the handle

   handleidx: which handle (by index) to setup
*/
static i32
handle_set_inputs(CSOUND *csound, POLY1 *p, ui32 handleidx) {
    ui32 firstcol = p->firstcol;
    ARRAYDAT *arr;
    ui32 col, nsmps = CS_KSMPS;
    OPCHANDLE *handle = &(p->handles[handleidx]);
    char c;
    MYFLT default_value;
    void **inargs = &(handle->state->args[p->num_output_args]);
    // opc_numins: the TOTAL number of inputs expected by opcode
    // num_input_args: the number of inputs actually passed to poly
    for(col = firstcol; col < p->opc_numins; col++) {
        // handle->state->inargs[col] = &(handle->indata[col]);
        inargs[col] = &(handle->indata[col]);
        c = p->opc->intypes[col];
        default_value = get_default_value(c);
        if((i32)default_value == default_fail)
            return INITERRF(Str("failed to parse signature (%s), char failed: '%c'"),
                            p->opc->intypes, c);
        if((i32)default_value != default_unset)
            handle->indata[col] = default_value;
    }

    for(col=firstcol; col<p->num_input_args; col++) {
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
            // handle->state->inargs[col] = p->inargs[col];
            inargs[col] = p->inargs[col];
            break;
        case 's':
            inargs[col] = p->inargs[col];
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
            // we assume that an audio array can't be resized--this MUST be documented
            arr = (ARRAYDAT*)(p->inargs[col]);
            inargs[col] = &(arr->data[handleidx*nsmps]);
            break;
        default:
            return INITERRF(Str("poly: unknown signature character %c (%s)"),
                            c, p->in_signature);
        }
    }
    return OK;
}

// check that the array is 1D and has at least the given size
#define CHECKARR1D(arr, minsize) do {                                             \
    if((arr)->dimensions != 1)                                                    \
      return INITERRF(Str("array dimensions (%d) should be 1"),                   \
                      (arr)->dimensions);                                         \
    if((ui32)(arr)->sizes[0] < minsize) {                                         \
      return INITERRF(Str("input array size (%d) must be >= num. instances (%d)"),\
                      (arr)->sizes[0], minsize);                                  \
    }} while(0)

#define CHECKALLOC(ptr, id) if(UNLIKELY((ptr)==NULL)) \
    return INITERRF(Str("Could not alloc %s"), (id))


static i32 poly1_deinit(CSOUND *csound, POLY1 *p);


static void _debug_dump_opc(OENTRY *opc) {
    printf("dsblksiz: %d\n", opc->dsblksiz);
    printf("opcname: %s\n", opc->opname);
    printf("intypes: %s\n", opc->intypes);
    printf("outtypes: %s\n", opc->outypes);
    OPCODINFO *inm = (OPCODINFO*) opc->useropinfo;
    printf("instno: %d\n", inm->instno);
    printf("intypes inm: %s\n", inm->intypes);
}

static void _dump_types(CSOUND *csound, void **args, int32_t numargs) {
    CS_TYPE *cstype;
    for(int i=0; i<numargs; i++) {
        DBGF("inspecting: %d / %d \n", i+1, numargs);
        cstype = csound->GetTypeForArg(args[i]);
        char *typename = cstype->varTypeName;
        DBGF("idx: %d / %d    typename: %s\n", i, numargs, typename);
    }
}


// check that signatures are valid to be used with poly
// own sig: outputs must be arrays of k or a type, one arg at least
// inputs must have no string
// opc out sig: a or k, no S or arrays, one arg at least
// opc in sig: no arrays, no String
static i32 poly_signature_check(CSOUND *csound, POLY1 *p) {
    // first check own sig
    // if(p->num_output_args < 1)
    //     return INITERRF(Str("at least 1 output arg. needed, got %d"), p->num_output_args);
    // we only accept arrays as out args, so out sig must be all uppercase
    if(!all_upper(p->out_signature))
        return INITERRF(Str("poly output args must be arrays, got %s"), p->out_signature);
    str_for_each(p->opc->intypes, c, {
        if(c=='[' || c==']')
            return INITERRF(Str("opcode's intype %c not supported"), c);
    });
    str_for_each(p->opc->outypes, c, {
        if(c != 'a' && c != 'k' && c != 'i')
            return INITERRF(Str("opcode's outtype %c not supported"), c);
    });
    return OK;
}

void handle_set_state(POLY1 *p, ui32 idx) {
    OPCHANDLE *handle = &(p->handles[idx]);
    OENTRY *opc = p->opc;
    char *state0 = (char *)p->states;
    OPCSTATE *state = (OPCSTATE *)(state0 + idx*opc->dsblksiz);
    state->h.iopadr = opc->iopadr;
    state->h.opadr = opc->kopadr;
    state->h.insdshead = p->h.insdshead;
    state->h.optext = p->optext;
    handle->state = state;
}

OPTXT* poly_make_optext(CSOUND *csound, POLY1 *p) {
    OPTXT *optext = csound->Calloc(csound, sizeof(OPTXT));
    optext->nxtop = NULL;
    optext->t.oentry = p->opc;
    optext->t.inArgCount = p->num_input_args;
    optext->t.outArgCount = p->num_output_args;
    optext->t.linenum = p->h.optext->t.linenum;
    optext->t.opcod = p->opc->opname;
    optext->t.intype = p->h.optext->t.intype;
    return optext;
}

static void
convert_multiplex_sig_to_single_sig(char *dest, char *src) {
    ui32 i = 0;
    char c;
    for(;;) {
        c = src[i];
        if (c == '\0')
            break;
        else if (c == 's')
            dest[i] = 'S';
        else
            dest[i] = (char)tolower(c);
        i++;
    }
    dest[i] = 0;
}


static i32 poly1_init(CSOUND *csound, POLY1 *p) {
    OENTRY *opc;
    i32 ret;
    ui32 i, nsmps = CS_KSMPS;
    char opc_outsig[64];    // in/out signature used to find the opcode
    char opc_insig[64];

    p->firstcol = 0;
    p->num_input_args = (ui32)csound->GetInputArgCnt(p) - 2;
    p->num_output_args = (ui32)csound->GetOutputArgCnt(p);
    p->num_instances = (ui32 ) *((MYFLT*)(p->args[p->num_output_args]));
    p->opcode_name = (STRINGDAT*) p->args[p->num_output_args + 1];
    p->inargs = &(p->args[p->num_output_args + 2]);
    p->is_parallel = 1;

    if(str_in_list(p->opcode_name->data, poly_blacklist))
        return INITERRF(Str("Opcode %s not supported"), p->opcode_name->data);

    if(get_signature(csound, p->inargs, p->num_input_args, p->in_signature) != OK)
        return INITERR(Str("could not parse input signature"));

    if(get_signature(csound, p->args, p->num_output_args, p->out_signature) != OK)
        return INITERR(Str("could not parse output signature"));

    // the target signature is just our signature without any arrays
    // str_tolower(opc_insig, p->in_signature);
    convert_multiplex_sig_to_single_sig(opc_insig, p->in_signature);
    // str_tolower(opc_outsig, p->out_signature);
    convert_multiplex_sig_to_single_sig(opc_outsig, p->out_signature);
    if(p->num_input_args != strlen(p->in_signature))
        return INITERRF(Str("arg. count mismatch (num input args: %d, signature: %s"),
                        p->num_input_args, p->in_signature);

    if(p->num_instances < 1)
        return INITERRF(Str("num. instances must be >= 1, got %d"), p->num_instances);

    opc = csound->find_opcode_new(csound, p->opcode_name->data, opc_outsig, opc_insig);
    if(opc == NULL)
        return INITERRF(Str("Opcode '%s' with signature '%s'/'%s' not found"),
                        p->opcode_name->data, opc_outsig, opc_insig);
    p->opc = opc;
    p->opc_numins = get_numargs(opc->intypes);
    p->opc_numouts = get_numargs(opc->outypes);

    if(p->opc_numins > POLY_MAXINPARAMS)
        return INITERRF(Str("opcode has too many input arguments (%d, max=%d"),
                        p->opc_numins, POLY_MAXINPARAMS);
    if(p->opc_numins > POLY_MAXOUTPARAMS)
        return INITERRF(Str("opcode has too many output arguments (%d, max=%d"),
                        p->opc_numouts, POLY_MAXOUTPARAMS);
    if(poly_signature_check(csound, p) != OK)
        return INITERR(Str("Signature not supported by poly"));

    // initialize output arrays
    for(i=0; i < p->num_output_args; ++i)
        tabinit(csound, (ARRAYDAT*)p->args[i], (int) p->num_instances);

    // create handles for each instance. we allocate all handles at once.
    p->handles = (OPCHANDLE*)csound->Calloc(csound, p->num_instances*sizeof(OPCHANDLE));
    CHECKALLOC(p->handles, "handles");

    for(i=0; i<p->num_input_args; i++) {
        char c = p->in_signature[i];
        // here we don't check k-arrays, since at this time a k-array is not populated
        if(c=='I' || c=='A' || c=='S') {
            ARRAYDAT *arr = (ARRAYDAT*) p->inargs[i];
            CHECKARR1D(arr, p->num_instances);
        }
    }

    p->optext = poly_make_optext(csound, p);
    OPCODINFO *inm = (OPCODINFO*) p->opc->useropinfo;
    if(inm != NULL)
        return INITERRF("UDOs are not supported (instno=%i)", inm->instno);

    // we allocate all states as an array
    p->states = (OPCSTATE*)csound->Calloc(csound, (p->num_instances)*(opc->dsblksiz));
    CHECKALLOC(p->states, "states for handle");
    for(i=0; i < p->num_instances; i++) {
        handle_set_state(p, i);
        OPCHANDLE *handle = &(p->handles[i]);
        for(ui32 j=0; j<p->num_output_args; j++) {
            char c = p->out_signature[j];
            ARRAYDAT *arrout = (ARRAYDAT *)(p->args[j]);
            if(c == 'A') {
                handle->state->args[j] = &(arrout->data[i*nsmps]);
            } else if(c == 'K') {
                handle->state->args[j] = &(arrout->data[i]);
            } else
                return INITERRF(Str("type not supported: %c"), c);
        }

        // setup input args
        if(OK != handle_set_inputs(csound, p, i))
            return INITERR(Str("poly: failed to setup handle"));
    }
    if(opc->iopadr != NULL) {
        for(i=0; i < p->num_instances; i++) {
            ret = opc->iopadr(csound, p->handles[i].state);
            if(ret != OK)
                return INITERRF("Error in opcode's init func (instance #%d)", i);
        }

    }
    csound->RegisterDeinitCallback(csound, p, (i32 (*)(CSOUND*, void*))poly1_deinit);
    return OK;
}

/** Deinit callback
 *
 * deallocates everything we allocated
 *
 */
static i32 poly1_deinit(CSOUND *csound, POLY1 *p) {
    if(p->handles != NULL) {
        csound->Free(csound, p->handles);
        p->handles = NULL;
    }
    if(p->states != NULL) {
        csound->Free(csound, p->states);
        p->states = NULL;
    }
    if(p->optext != NULL) {
        csound->Free(csound, p->optext);
        p->optext = NULL;
    }
    p->opc = NULL;
    return OK;
}

/** The performance loop
 *
 * calls kopadr for all instances of the opcode
 *
 */
static i32 poly1_perf(CSOUND *csound, POLY1 *p) {
    ARRAYDAT *arr;
    char *in_signature = p->in_signature;
    ui32 col, i, numcols = p->num_input_args;
    if(UNLIKELY(p->opc->kopadr == NULL))
        return PERFERRF(Str("opcode %s has no performance callback!"),
                        p->opcode_name->data);

    // check input k-arrays, they might have changed size
    for(col=p->firstcol; col < numcols; col++) {
        if(in_signature[col] == 'K') {
            arr = (ARRAYDAT*)p->inargs[col];
            CHECKARR1D(arr, p->num_instances);
        }
    }

    // if parallel, ensure size of output arrays
    if(p->is_parallel) {
        for(i=0; i < p->num_output_args; ++i)
            ARRAY_ENSURESIZE((ARRAYDAT*)p->args[i], (int) p->num_instances);
    }

    for(i=0; i<p->num_instances; i++) {
        OPCHANDLE *handle = &(p->handles[i]);
        if(UNLIKELY(handle == NULL))
            return PERFERRF(Str("handle %d is invalid"), i);
        for(col=p->firstcol; col<numcols; col++) {
            char c = in_signature[col];
            if(c == 'K' || c == 'I') {
                // a number array, copy data to internal storage
                arr = (ARRAYDAT*)p->inargs[col];
                handle->indata[col] = arr->data[i];
            } else if (c=='k') {
                // scalar, copy value to internal storage
                handle->indata[col] = *((MYFLT*)(p->inargs[col]));
            }
        }
        p->opc->kopadr(csound, (void*)handle->state);
    }
    return OK;
}

// --------------------------------------------------------------------------

/**
 * polyseq
 *
 * aout polyseq numinstances:i, opcodename:s, ain, params ...
 * kout polyseq numinstances:i, opcodename:s, kin, params ...
 *
 * params can be anything: k, a, k[] or a[]
 *
 * For arrays, they need to be at least the size of `numinstances`
 * Scalar and audio inputs are "shared" by all instances
 *
 * The opcode used is derived of the types used as outputs and inputs
 * to the poly object. For example, to implement a parametric eq:
 *
 *   kfreqs[] fillarray 500, 600, 700
 *   aout polyseq 10, "pareq", ain, kFreqs, kBoosts, 0.2
 *
 *   will search for an opcode "pareq" with signature "a", "akki"
 *
 * The number of outputs in the opcode defines the behaviour: if the opcode
 * is called with two audio outputs, like 'a0, a1 polyseq ...', then the input
 * arguments are expected to mirror this as
 * '... polyseq inuminst, "opcode", a0, a1, ...', and these inputs can't be arrays
 *
 * Mapping: the first instance is mapped to the inputs and outputs of the polyseq
 * opcode. The inputs and outputs of the remaining instances are mapped both to
 * the outputs of the polyseq. The sequential evaluation does the trick.
 *
 */

// check that signatures are valid to be used with polyseq
// own sig: outputs should have correlated inputs, outputs can't be arrays
// opc out sig: a or k, no S or arrays, one arg at least
// opc in sig: no arrays, no String
static i32 polyseq_signature_check(CSOUND *csound, POLY1 *p) {
    // first check own sig
    if(p->num_output_args < 1)
        return INITERRF(Str("at least 1 output arg. needed, got %d"), p->num_output_args);
    if(p->num_input_args < p->num_output_args)
        return INITERRF(Str("num. input args (%d) should be at least %d"),
                        p->num_input_args, p->num_output_args);
    // we don't accept arrays as out args, so out sig must be all lowercase
    if(!all_lower(p->out_signature))
        return INITERRF(Str("poly output args can't be arrays, got %s"), p->out_signature);
    if(findchar(p->in_signature, 's') >= 0 || findchar(p->in_signature, 'S') >= 0)
        return INITERRF(Str("no strings accepted as input (yet), got %s"), p->in_signature);
    // now check opcode's signature: no arrays there
    str_for_each(p->opc->intypes, c, {
        if(c=='[' || c==']' || c=='S')
            return INITERRF(Str("opcode's intype %c not supported"), c);
    });
    str_for_each(p->opc->outypes, c, {
        if(c != 'a' && c != 'k' && c != 'i')
            return INITERRF(Str("opcode's outtype %c not supported"), c);
    });
    for(ui32 idx=0; idx < p->num_output_args; idx++) {
        if(p->out_signature[idx] != p->in_signature[idx])
            return INITERRF("The output args (%s) should match the first input args (%s)",
                            p->out_signature, p->in_signature);
    }
    return OK;
}


static i32 polyseq_init(CSOUND *csound, POLY1 *p) {
    ui32 i;
    i32 ret;
    char opc_outsig[64];    // out signature used to find the opcode
    char opc_insig[64];     // in signature used to find opcode

    p->num_input_args = (ui32)csound->GetInputArgCnt(p) - 2;
    p->num_output_args = (ui32)csound->GetOutputArgCnt(p);
    p->num_instances = (ui32 ) *((MYFLT*)(p->args[p->num_output_args]));
    p->opcode_name = (STRINGDAT*) p->args[p->num_output_args + 1];
    p->inargs = &(p->args[p->num_output_args + 2]);
    p->firstcol = p->num_output_args;
    p->is_parallel = 0;

    if(str_in_list(p->opcode_name->data, poly_blacklist))
        return INITERRF(Str("Opcode %s not supported"), p->opcode_name->data);

    ret = get_signature(csound, p->inargs, p->num_input_args, p->in_signature);
    if(ret != OK)
        return INITERR(Str("Couldn't parse input signature"));

    ret = get_signature(csound, p->args, p->num_output_args, p->out_signature);
    if(ret != OK)
        return INITERR(Str("Couldn't parse output signature"));

    // the target signature is just our signature without any arrays (thus lowercase)
    str_tolower(opc_insig, p->in_signature);
    str_tolower(opc_outsig, p->out_signature);

    if(p->num_input_args != strlen(p->in_signature))
        return INITERRF(Str("arg. count mismatch (num input args: %d, signature: %s"),
                        p->num_input_args, p->in_signature);

    if(p->num_instances < 1)
        return INITERRF(Str("num. instances must be >= 1, got %d"), p->num_instances);

    OENTRY *opc = csound->find_opcode_new(csound, p->opcode_name->data,
                                          opc_outsig, opc_insig);
    if(opc == NULL)
        return INITERRF(Str("Opcode '%s' with signature '%s'/'%s' not found"),
                        p->opcode_name->data, opc_outsig, opc_insig);

    if(findchar(opc->intypes, '[') >= 0 || findchar(opc->outypes, '[') >= 0)
        return INITERR(Str("Opcodes with arrays as inputs or outputs are not supported"));

    p->opc = opc;
    p->opc_numins = get_numargs(opc->intypes);
    p->opc_numouts = get_numargs(opc->outypes);

    if(p->opc_numins > POLY_MAXINPARAMS)
        return INITERRF(Str("Opcode has too many input arguments (%d, max=%d"),
                        p->opc_numins, POLY_MAXINPARAMS);
    if(p->opc_numins > POLY_MAXOUTPARAMS)
        return INITERRF(Str("Opcode has too many output arguments (%d, max=%d"),
                        p->opc_numouts, POLY_MAXOUTPARAMS);

    if(OK != polyseq_signature_check(csound, p))
        return INITERR(Str("Signature not supported by polyseq"));

    // create handles for each instance. we allocate all handles at once.
    p->handles = (OPCHANDLE*)csound->Calloc(csound, p->num_instances*sizeof(OPCHANDLE));
    CHECKALLOC(p->handles, "handles");
    // check size of input arrays
    for(i=0; i<p->num_input_args; i++) {
        char c = p->in_signature[i];
        // here we don't check k-arrays, since at this time a k-array would not
        // be populated and might not have correct size
        if(c=='I' || c=='A' || c=='S') {
            ARRAYDAT *arr = (ARRAYDAT*) p->inargs[i];
            CHECKARR1D(arr, p->num_instances);
        }
    }

    p->optext = poly_make_optext(csound, p);
    OPCODINFO *inm = (OPCODINFO*) p->opc->useropinfo;
    if(inm != NULL)
        return INITERRF("UDOs are not supported (instno=%i)", inm->instno);

    // we allocate all states as an array
    p->states = (OPCSTATE*)csound->Calloc(csound, (p->num_instances)*(opc->dsblksiz));
    CHECKALLOC(p->states, "states for handle");
    // set each state
    for(i=0; i < p->num_instances; i++) {
        handle_set_state(p, i);
    }

    // polyseq has two types of args: chained args and multiplexed args
    // len(chained) == num. outputs of polyseq == polyseq.inputs[:len(polyseq.outputs)]
    // chained args are daisy chained:
    //     p.input = handle[0].input
    //     handle[-1].output = p.output
    //     handle[x].input = handle[x+1].output = p.output
    //     which results in all instances but the first having i/o set to p.output

    // first we map all handles I/O to p.output
    ui32 numchained = p->num_output_args;
    OPCHANDLE *handle;
    for(i=0; i<p->num_instances; i++) {
        handle = &(p->handles[i]);
        for(ui32 j=0; j < numchained; j++) {
            handle->state->args[numchained+j] = p->args[j];
            handle->state->args[j] = p->args[j];
        }
    }

    // then we single-case handle 0, mapping handle.input to p.input
    handle = &(p->handles[0]);
    for(ui32 j=0; j < numchained; j++) {
        handle->state->args[numchained + j] = p->inargs[j];
    }

    // now map the multiplexed args. This is the same as in poly
    for(i=0; i < p->num_instances; i++) {
        if(OK != handle_set_inputs(csound, p, i))
            return INITERR(Str("failed to setup handle"));
    }

    // finished! now we can call the opcode's init func
    if(opc->iopadr != NULL) {
        for(i=0; i < p->num_instances; i++) {
            if(OK != opc->iopadr(csound, p->handles[i].state))
                return INITERRF("Error in opcode's init func (instance #%d)", i);
        }
    }

    // register out deinit func to deallocate everything
    csound->RegisterDeinitCallback(csound, p, (i32 (*)(CSOUND*, void*))poly1_deinit);
    return OK;
}

// --------------------------------------------------------------------------
// defer "opcode", arg1, arg2, arg3
// opcode must be an i-time opcode.
// args can be i-, i[] or S-type and should not have any outargs

typedef struct {
    OPDS h;
    STRINGDAT *opcode_name;       // name of the opcode
    void *args[POLY_MAXPARAMS];   // the arguments passed to the opc
    i32 num_input_args;           // the length of args
    OENTRY *opc;                  // the pointer to the opcode struct
    i32 opc_numins;               // max. number of arguments expected by the opcode
    OPTXT *optext;                // OPTXT struct
    OPCSTATE *opc_state;
    char in_signature[POLY_MAXPARAMS];

} DEFER;

static i32 defer_deinit(CSOUND *csound, DEFER *p);

static i32 defer_init(CSOUND *csound, DEFER *p) {
    char *opc_outsig = "";
    char opc_insig[64];
    int num_input_args = csound->GetInputArgCnt(p) - 1;
    int num_output_args = csound->GetOutputArgCnt(p);
    if(num_output_args > 0)
        return INITERRF("No output arguments supported, got %d", num_output_args);
    get_signature(csound, p->args, num_input_args, opc_insig);
    convert_multiplex_sig_to_single_sig(opc_insig, opc_insig);
    strncpy(p->in_signature, opc_insig, POLY_MAXPARAMS-1);

    p->num_input_args = num_input_args;
    OENTRY *opc = csound->find_opcode_new(csound, p->opcode_name->data,
                                          opc_outsig, opc_insig);
    if(opc == NULL)
        return INITERRF(Str("Opcode '%s' with signature '%s' not found"),
                        p->opcode_name->data, opc_insig);
    p->opc = opc;
    p->opc_numins = get_numargs(opc->intypes);
    i32 opc_numouts = get_numargs(opc->outypes);

    if(opc_numouts > 0)
        return INITERRF("Opcode %s has output arguments. This is not allowed",
                        p->opcode_name->data);
    OPTXT *optext = csound->Calloc(csound, sizeof(OPTXT));
    optext->nxtop = NULL;
    optext->t.oentry = opc;
    optext->t.inArgCount = p->num_input_args;
    optext->t.outArgCount = 0;
    optext->t.linenum = p->h.optext->t.linenum;
    optext->t.opcod = opc->opname;
    optext->t.intype = p->h.optext->t.intype;
    p->optext = optext;

    OPCSTATE *state = csound->Calloc(csound, opc->dsblksiz);
    state->h.iopadr = opc->iopadr;
    state->h.opadr = opc->kopadr;
    state->h.insdshead = p->h.insdshead;
    state->h.optext = optext;
    p->opc_state = state;

    // copy args
    void **inargs = &(state->args[0]);
    // opc_numins: the number of inputs expected by the opcode according
    // to its own signature. NB: in the cases of m, M, *, etc, the opcode
    // can in fact take more arguments.

    int max_args = max(p->opc_numins, p->num_input_args);
    for(int i=0; i < max_args; i++) {
        inargs[i] = p->args[i];
        if(i >= p->opc_numins)
            continue;
        char c = p->opc->intypes[i];
        MYFLT default_value = get_default_value(c);

        if((i32)default_value == default_fail)
            return INITERRF(Str("failed to parse signature (%s), char failed: '%c'"),
                            p->opc->intypes, c);

        if((i32)default_value != default_unset) {
            printf("setting default # %d (%c) to %f \n", i, c, default_value);
            *(MYFLT*)inargs[i] = default_value;
        }
    }
    OPCODINFO *inm = (OPCODINFO*) opc->useropinfo;
    if(inm != NULL)
        return INITERRF("UDOs are not supported (instno=%i)", inm->instno);
    register_deinit(csound, p, defer_deinit);
    return OK;
}

static i32
defer_deinit(CSOUND *csound, DEFER *p) {
    if(p->opc->iopadr != NULL) {
        if (p->opc->iopadr(csound, p->opc_state) == NOTOK)
            return PERFERRF("Error in deferred opcode %s init func",
                            p->opcode_name->data);
    }
    if(p->opc->kopadr != NULL) {
        if (p->opc->kopadr(csound, p->opc_state) == NOTOK)
            return PERFERRF("Error in deferred opcode %s perf func",
                            p->opcode_name->data);
    }
    csound->Free(csound, p->opc_state);
    csound->Free(csound, p->optext);
    return OK;
}


// --------------------------------------------------------------------------


#define S(s) sizeof(s)

static OENTRY localops[] = {
    { "poly0", S(POLY1), 0, 3, "", "iS*", (SUBR)poly1_init, (SUBR)poly1_perf },

    { "poly", S(POLY1), 0, 3, "*", "iS*", (SUBR)poly1_init, (SUBR)poly1_perf },

    { "polyseq", S(POLY1), 0, 3, "*", "iS*", (SUBR)polyseq_init, (SUBR)poly1_perf },
    // not working yet
    { "defer", S(DEFER), 0, 1, "", "S*", (SUBR)defer_init}
};

LINKAGE
