/*
   kdict.c

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

  This file implements opcodes for a hashtable (dict)

  * A dict can be created by a note or by instr 0.
  * lifetime: either tied to the lifetime of the note, or it can
    survive the note until dict_free is called later. When a dict
    is not tied to a note, its handle can be passed around.
    By default, a dict is local (is freed with the end of the note)
  * key can be either a string or an integer
  * value can be either be a string or a float (maybe in the future also audio
    and arrays?)
  * the type of the hashtable is indicated via a string code:
    - idict dict_new "sf"  -> key of type string, value of type float
    - idict dict_new "is"  -> key:int, value:string
    Possible types are: sf, ss, is, if

  Implemented opcodes:

  dict_new
  ========

    ihandle dict_new Stype [, iglobal]

    Stype: the type definition, a 2-char string defining the type of
           the key (either string or int) and the value (either string or float)
           s = string
           i = int
           f = float
    iglobal: if 1, dict survives the note and is freed at the end of the performance or
             by calling dict_free

    idict dict_new "sf"    ; create a hashtable of type string -> number
                           ; hashtable is freed at note end
    idict dict_new "if", 1 ; create a hashtable of type int -> number
                           ; hahstable is global, survives the note

    It is also possible to initialize key:value pairs at init time, for example:

    ihandle dict_new "sf", 0, "foo", 10, "bar", 20, "baz", 0.5

  dict_free
  =========

    dict_free idict [, iwhen]

    frees the hashtable either at init time or at the end of the note
    (similar to ftfree)
    This is needed when passing a hashtable from one instrument to another

    iwhen = 0  -> free now
    iwhen = 1  -> free at the end of this note

  dict_set
  ========

    dict_set idict, Skey, kvalue        ; k-time
    dict_set idict, kkey, kvalue        ; k-time
    dict_set idict, ikey, ivalue        ; i-time

    Set a key:value pair

    idict dict_new "sf"
    dict_set idict, "key", kvalue

    If a value is not given, the key:value pair is deleted (if it exists)

  dict_get
  ========

    Get the value at a given key. For string keys, an empty string
    is returned when the key is not found. For int keys, either 0
    or a default value given by the user are returned when the
    key is not found.

    kvalue dict_get idict, "key" [, kdefault]

  dict_query
  ==========

    iout dict_query idict, Scmd

    Where Scmd can be one of:

    * size: get the number of key:value pairs. Will return -1 if the dict does not exist
    * type: get the type of the dict, as integer (see table below)
    * exists: returns 1 if idict exists

    Signature  Type
           11  int -> float
           12  int -> string
           21  string -> float
           22  string -> string

  dict_iter
  =========

    xkey, xvalue, kstop  dict_iter idict [, kreset=-1]

    Iterates over the key:value pairs. Whenever kreset is 1, iteration starts over
    If kreset is -1 (the default), iteration is autotriggered when it reaches
    the end of the collection.
    This opcode is meant to be used in a loop at k-time

    kidx = 0
    while kidx < dict_size(idict)-1 do
      Skey, kvalue, kidx dict_iter idict
      ; do something with Skey / kvalue
    od

  dict_size
  =========

    ksize dict_size kdict

    This is the same as dict_query idict, "size", but as it is used often
    together with dict_iter, it is better to have it as an opcode

  All opcodes work at k-time
  The hashtables with int-keys work also at i-time whenever key and value are
  of i-type (for both set and get actions)
  
*/

#include "csdl.h"
#include "arrays.h"

#include "khash.h"
#include "ukstring.h"
#include <ctype.h>

typedef int32_t i32;
typedef uint32_t ui32;
typedef int64_t i64;
typedef uint64_t ui64;

#define KHASH_STRKEY_MAXSIZE 63
#define HANDLES_INITIAL_SIZE 200

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

#define ERR_NOINSTANCE Str("dict: instance doesn't exist")
#define GLOBALS_NAME "em.khash_globals"

#define CHECK_HASHTAB_EXISTS(h) {if(UNLIKELY((h)==NULL)) return PERFERR(ERR_NOINSTANCE);}
#define CHECK_HANDLE(handle) {if((handle)->hashtab==NULL) return PERFERR(ERR_NOINSTANCE);}

#define CHECK_HASHTAB_TYPE(handletype, expectedtype)  \
    if(UNLIKELY((handletype) != (expectedtype))) { \
        csound->InitError(csound, Str("Expected a dict of type '%s', got '%s'"), \
        intdef_to_stringdef(expectedtype), intdef_to_stringdef(handletype));     \
        return NOTOK; }

// s: a STRINGDAT* key
#define CHECK_KEY_SIZE(s)                                               \
    if(UNLIKELY((s)->size > KHASH_STRKEY_MAXSIZE))                      \
        return PERFERRF(Str("dict: key too long (%d > %d)"),            \
                        (s)->size, KHASH_STRKEY_MAXSIZE)

// p: an opcode struct with a member 'g':KHASH_GLOBALS* and input handleidx:MYFLT*
#define get_handle_check(p)  ((ui32)*(p)->handleidx < p->g->maxhandles ? &((p)->g->handles[(ui32)*(p)->handleidx]) : NULL)

#define get_handle(p) (&((p)->g->handles[(ui32)*(p)->handleidx]))


// #define CSOUND_MULTICORE

#ifdef CSOUND_MULTICORE
    #define LOCK(x) csound->LockMutex(x->mutex_)
    #define UNLOCK(x) csound->LockMutex(x->mutex_)
#else
    #define LOCK(x) do {} while(0)
    #define UNLOCK(x) do {} while(0)
#endif

// ---------------------- Utilities -----------------------

/**
 * Adapted from kstring/ks_setn but with csound->ReAlloc as allocator
 */
static inline void
kstr_setn(CSOUND *csound, kstring_t *ks, const char *src, ui32 srclen) {
    if(srclen >= ks->m) {
        char *tmp;
        ks->m = srclen + 1;
        kroundup32(ks->m);
        if ((tmp = (char*)csound->ReAlloc(csound, ks->s, ks->m)))
            ks->s = tmp;
        else {
            ks->l = 0;
            return;
        }
    }
    ks->l = srclen;
    memcpy(ks->s, src, ks->l);
    ks->s[ks->l] = 0;
}

// set ks to s, reallocating if necessary. A kstring never shrinks down, only grows
static inline void
kstr_set_from_stringdat(CSOUND *csound, kstring_t *ks, STRINGDAT *s) {
    kstr_setn(csound, ks, s->data, (ui32)strlen(s->data));
}

// init ks to s
static inline void
kstr_init_from_stringdat(CSOUND *csound, kstring_t *ks, STRINGDAT *s) {
    ks->s = csound->Strdup(csound, s->data);
    ks->l = strlen(ks->s);
    ks->m = ks->l + 1;
}

// like strncpy but really makes sure that the dest str is 0 terminated
static inline
void strncpy0(char *dest, const char *src, size_t n) {
    strncpy(dest, src, n);
    dest[n] = '\0';
}

// Set a STRINGDAT* from a source string and its length
static inline i32
stringdat_set(CSOUND *csound, STRINGDAT *s, char *src, ui32 srclen) {
    if(s->data == NULL) {
        s->data = csound->Strdup(csound, src);
        s->size = (i32) srclen + 1;
    } else if ((ui32)s->size <= srclen) {
        csound->Free(csound, s->data);
        s->data = csound->Strdup(csound, src);
        s->size = (i32) srclen + 1;
    } else
        strcpy(s->data, src);
    return OK;
}

// float=1, str=2
enum {
    khStrFlt = 21,
    khStrStr = 22,
    khIntStr = 12,
    khIntFlt = 11
};

// Initialize all possible types
KHASH_MAP_INIT_STR(khStrFlt, MYFLT)
KHASH_MAP_INIT_STR(khStrStr, kstring_t)
KHASH_MAP_INIT_INT(khIntFlt, MYFLT)
KHASH_MAP_INIT_INT(khIntStr, kstring_t)


typedef struct {
    CSOUND *csound;
    int khtype;
    ui64 counter;
    void *hashtab;
    int isglobal;
    void *mutex_;
    spin_lock_t lock;
} HANDLE;


// handle: a KHASH_HANDLE, code: the code to run with the hashtabele h
#define with_hashtable(handle, code) {               \
    i32 _khtype = handle->khtype;                    \
    if(_khtype == khStrStr) {                        \
        khash_t(khStrStr) *h = handle->hashtab;      \
        code;                                        \
    } else if (_khtype == khStrFlt) {                \
        khash_t(khStrFlt) *h = handle->hashtab;      \
        code;                                        \
    } else if (_khtype == khIntStr) {                \
        khash_t(khIntStr) *h = handle->hashtab;      \
        code;                                        \
    } else {                                         \
        khash_t(khIntFlt) *h = handle->hashtab;      \
        code;                                        \
    }                                                \
} \


#define kh_foreach_key(h, kvar, code) { khint_t __i;   \
    for (__i = kh_begin(h); __i != kh_end(h); ++__i) { \
            if (!kh_exist(h,__i)) continue;            \
            (kvar) = kh_key(h,__i);                    \
            code;                                      \
    }}


typedef struct {
    CSOUND *csound;
    HANDLE *handles;
    ui32 maxhandles;
    void *mutex_;
} HASH_GLOBALS;


static inline HANDLE *get_handle_by_idx(HASH_GLOBALS *g, ui32 idx) {
    if(idx > g->maxhandles)
        return NULL;
    return &(g->handles[idx]);
}


// ----------------- dict_new ------------------

typedef struct {
    OPDS h;
    // outputs
    MYFLT *handleidx;
    // inputs
    STRINGDAT *keyvaltype;
    MYFLT *isglobal;
    void  *inargs[VARGMAX];
    // internal
    HASH_GLOBALS *g;
    int khtype;
} DICT_NEW;

// forward declarations
static i32 dict_reset(CSOUND *csound, HASH_GLOBALS *g);
static i32 _dict_free(CSOUND *csound, HASH_GLOBALS *g, ui32 idx);

static i32 set_many_ss(CSOUND *cs, void** inargs, ui32 numargs, HANDLE *handle);
static i32 set_many_sf(CSOUND *cs, void** inargs, ui32 numargs, HANDLE *handle);
static i32 set_many_is(CSOUND *cs, void** inargs, ui32 numargs, HANDLE *handle);
static i32 set_many_if(CSOUND *cs, void** inargs, ui32 numargs, HANDLE *handle);

/**
 * Create the globals struct. Will be called once, the first time one of the
 * dict_ opcodes is called
 */
static HASH_GLOBALS* create_globals(CSOUND *csound) {
    int err = csound->CreateGlobalVariable(csound, GLOBALS_NAME, sizeof(HASH_GLOBALS));
    if (err != 0) {
        INITERR(Str("dict: failed to allocate globals"));
        return NULL;
    };
    HASH_GLOBALS *g = (HASH_GLOBALS*)csound->QueryGlobalVariable(csound, GLOBALS_NAME);
    g->csound = csound;
    g->maxhandles = HANDLES_INITIAL_SIZE;
    g->handles = csound->Calloc(csound, sizeof(HANDLE)*g->maxhandles);
    g->mutex_ = csound->Create_Mutex(0);
    csound->RegisterResetCallback(csound, (void*)g, (i32(*)(CSOUND*, void*))dict_reset);
    return g;
}

/**
 * get the globals struct
 */
static inline HASH_GLOBALS* dict_globals(CSOUND *csound) {
    HASH_GLOBALS *g = (HASH_GLOBALS*)csound->QueryGlobalVariable(csound, GLOBALS_NAME);
    if(LIKELY(g != NULL)) return g;
    return create_globals(csound);
}

/**
 * find a free index to create a new dict. Will grow the internal array if more
 * room is needed. Returns 0 if failed
 */
static inline ui32
dict_getfreeslot(HASH_GLOBALS *g) {
    HANDLE *handles = g->handles;
    for(ui32 i=1; i<g->maxhandles; i++) {
        if(handles[i].hashtab == NULL)
            return i;
    }
    // no free slots, we need to grow!
    CSOUND *csound = g->csound;
    ui32 numhandles = g->maxhandles * 2;
    LOCK(g);
    g->handles = csound->ReAlloc(csound, g->handles, sizeof(HANDLE)*numhandles);
    if (g->handles == NULL)
        return 0;
    ui32 idx = g->maxhandles;
    g->maxhandles = numhandles;
    UNLOCK(g);
    return idx;
}

/**
 *  Creates a new dict, sets its pointer at a free index and returns this index
 *  Returns 0 if failed
 *
 * g: globals as returned by dict_globals
 * khtype: the int signature of this dict
 * isglobal: should this dict be deallocated together with the instrument or should
 *           it outlive the instr
 */
static ui32
dict_make(CSOUND *csound, HASH_GLOBALS *g, int khtype, int isglobal) {
    ui32 idx = dict_getfreeslot(g);
    if(idx == 0)
        return 0;
    void *hashtab = NULL;
    switch(khtype) {
    case khStrFlt:
        hashtab = (void *)kh_init(khStrFlt);
        break;
    case khStrStr:
        hashtab = (void *)kh_init(khStrStr);
        break;
    case khIntStr:
        hashtab = (void *)kh_init(khIntStr);
        break;
    case khIntFlt:
        hashtab = (void *)kh_init(khIntFlt);
        break;
    }
    HANDLE *handle = &(g->handles[idx]);
    handle->hashtab = hashtab;
    handle->khtype = khtype;
    handle->counter = 0;
    handle->isglobal = isglobal;
    handle->mutex_ = csound->Create_Mutex(0);
    return idx;
}

/**
 * dict_reset: function called when exiting performance
 * @param g: globals
 */
static i32
dict_reset(CSOUND *csound, HASH_GLOBALS *g) {
    ui32 i;
    DBG("%s", "hashtab_reset");
    for(i = 0; i < g->maxhandles; i++) {
        if(g->handles[i].hashtab != NULL)
            _dict_free(csound, g, i);
        if(g->handles[i].mutex_ != NULL)
            csound->DestroyMutex(g->handles[i].mutex_);
    }
    csound->DestroyMutex(g->mutex_);
    csound->Free(csound, g->handles);
    csound->DestroyGlobalVariable(csound, GLOBALS_NAME);
    return OK;
}

/**
 * Free all memory associated with a dict at given idx, free the slot
 * NB: a slot is free when the handle's hashtab is NULL
 */
static i32
_dict_free(CSOUND *csound, HASH_GLOBALS *g, ui32 idx) {
    HANDLE *handle = &(g->handles[idx]);
    khint_t k;
    kstring_t *ks;
    int khtype = handle->khtype;
    DBG("dict: freeing idx=%d, type=%d\n", idx, khtype);
    if(khtype == khStrFlt) {
        khash_t(khStrFlt) *h = handle->hashtab;
        // we need to free all keys
        for (k = 0; k < kh_end(h); ++k) {
            if (kh_exist(h, k))
                csound->Free(csound, kh_key(h, k));
        }
        kh_destroy(khStrFlt, h);
    } else if (khtype == khStrStr) {
        khash_t(khStrStr) *h = handle->hashtab;
        // we need to free all keys and values
        for (k = 0; k < kh_end(h); ++k) {
            if (kh_exist(h, k)) {
                ks = &(kh_val(h, k));
                if(ks->s != NULL)
                    csound->Free(csound, ks->s);
                csound->Free(csound, kh_key(h, k));
            }
        }
        kh_destroy(khStrStr, h);
    } else if (khtype == khIntFlt) {
        khash_t(khIntFlt) *h = handle->hashtab;
        kh_destroy(khIntFlt, h);
    } else if (khtype == khIntStr) {
        khash_t(khIntStr) *h = handle->hashtab;
        // we need to free all values
        for (k = 0; k < kh_end(h); ++k) {
            if (kh_exist(h, k)) {
                ks = &(kh_val(h, k));
                csound->Free(csound, ks->s);
            }
        }
        kh_destroy(khIntStr, h);
    } else
        return NOTOK;
    handle->hashtab = NULL;
    handle->counter = 0;
    handle->khtype = 0;
    handle->isglobal = 0;
    return OK;
}

/**
 * function called when deiniting a dict (for example, when a local dict
 * was created and the note it belongs to is released)
 */
static i32
dict_deinit_callback(CSOUND *csound, DICT_NEW* p) {
    DBG("%s", "deinit callback");
    ui32 idx = (ui32)*p->handleidx;
    HASH_GLOBALS *g = p->g;
    if(g->handles[idx].hashtab == NULL) {
        csound->Message(csound, "%s\n", Str("dict already freed"));
        return OK;
    }
    return _dict_free(csound, g, idx);
}

/**
 * A dict has a type defined by a string of two characters. The first defined
 * the type of the key, the second of the value. f=float, i=int, s=string
 * This string representation has also a numeric representation, in base 10,
 * where 1 = number, 2 = string
 *
 * This numeric representation is internal and we don't need to make the
 * differentiation between int and float because keys can't be float.
 * returns NULL if error
 */
static char*
intdef_to_stringdef(i32 intdef) {
    switch(intdef) {
    case khIntFlt:
        return "if";
    case khIntStr:
        return "is";
    case khStrFlt:
        return "sf";
    case khStrStr:
        return "ss";
    }
    return NULL;
}

/**
 * convert a type name to its int representation
 * 1 = number, 2 = string (12 = number->string)
 */
static inline i32 type_char_to_int(char c) {
    switch(c) {
    case 'S':
        return 2;
    case 'c':
    case 'i':
    case 'k':
        return 1;
    default:
        return -1; // signal an error
    }
}


/**
 * convert a str definition of a dict type to its numeric representation
 * returns -1 in case of error
 */
static i32
stringdef_to_intdef(STRINGDAT *s) {
    if(strlen(s->data) != 2)
        return -1;
    char *sdata = (char *)s->data;
    if(strcmp("ss", sdata) == 0) return khStrStr;
    else if(strcmp("sf", sdata) == 0) return khStrFlt;
    else if(strcmp("is", sdata) == 0) return khIntStr;
    else if(strcmp("if", sdata) == 0) return khIntFlt;
    else return -1;
}

/**
 * dict_new opcode, works at i-time only
 *
 * idict  dict_new Stype, [isglobal=0, key, value, key, value, ...]
 *
 * Stype: the type of the dict
 *   sf: str -> float
 *   ss: str -> str
 *   if: int -> float
 *   is: int -> str
 *
 *  isglobal: if 1, the dict is not deleted after the note is freed
 */
static i32
dict_new(CSOUND *csound, DICT_NEW *p) {
    p->g = dict_globals(csound);
    i32 khtype = stringdef_to_intdef(p->keyvaltype);
    if(khtype < 0)
        return INITERR(Str("dict: type not understood."
                           ". Expected one of 'sf', 'ss', 'if', 'is'"));
    ui32 idx = dict_make(csound, p->g, khtype, (i32) *p->isglobal);
    if(idx == 0)
        return INITERR(Str("dict: failed to allocate the hashtable"));
    *p->handleidx = (MYFLT)idx;
    p->khtype = khtype;
    if ((i32)*p->isglobal == 0) {
        // dict should not survive this note
        csound->RegisterDeinitCallback(csound, p,
                                       (i32 (*)(CSOUND*, void*))dict_deinit_callback);
    }
    return OK;
}


/**
 * check a multi-arg signature of the form: key, value, key, value, ...
 * @param args: an array of arguments
 * @param numargs: the size of args
 * @param khtype: the integer representation of the type of a pair
 */
static i32
check_multi_signature(CSOUND *csound, void **args, ui32 numargs, int khtype) {
    CS_TYPE *cstype;
    int pairtype = 0;
    if(numargs % 2 != 0)
        return INITERRF("number of arguments should be even, got %d", numargs);
    for(ui32 i=0; i<numargs; i+=2) {
        cstype = csound->GetTypeForArg(args[i]);
        char c0 = cstype->varTypeName[0];
        cstype = csound->GetTypeForArg(args[i+1]);
        char c1 = cstype->varTypeName[0];
        pairtype = type_char_to_int(c0) * 10 + type_char_to_int(c1);
        if(pairtype < 0)
            return INITERRF("problem parsing arguments, expected pairs of type %s",
                            intdef_to_stringdef(khtype));
        if(pairtype != khtype)
            return INITERRF("type mismatch, expected pairs of type %s, but got a"
                            "pair with type %s",
                            intdef_to_stringdef(khtype),
                            intdef_to_stringdef(pairtype));
    }
    return OK;
}

/**
 * version of the opcode to allow to set initial values for
 * a new dict
 *
 * Example:
 *
 *      idict  dict_new "sf", 0, "foo", 10, "bar", 20.5, "baz", 45
 */
static i32
dict_new_many(CSOUND *csound, DICT_NEW *p) {
    i32 ret = dict_new(csound, p);
    if (ret == NOTOK) return NOTOK;
    // our signature is: idict dict_new Stype, isglobal, args passed to opcode
    // So the number of args to be passed to opcode is our num. args - 2
    ui32 numargs = p->INOCOUNT - 2;
    i32 idx = (i32)(*p->handleidx);
    HANDLE *handle = &(p->g->handles[idx]);
    CHECK_HANDLE(handle);
    check_multi_signature(csound, p->inargs, numargs, handle->khtype);
    switch(p->khtype) {
    case khStrFlt:
        return set_many_sf(csound, p->inargs, numargs, handle);
    case khStrStr:
        return set_many_ss(csound, p->inargs, numargs, handle);
    case khIntStr:
        return set_many_is(csound, p->inargs, numargs, handle);
    case khIntFlt:
        return set_many_if(csound, p->inargs, numargs, handle);
    }
    return OK;  // this will never be reached
}


// ------------------------------------------------------
//                         SET
// ------------------------------------------------------

/**
 * dict_set: set the value of an existing key, or create a new
 * key:value pair
 */
typedef struct {
    OPDS h;
    // out
    MYFLT *handleidx;
    // in
    MYFLT *outkey;
    MYFLT *outval;
    // internal
    HASH_GLOBALS *g;
    ui32 lastidx;
    ui32 lastkey;
    ui64 counter;
} DICT_SET_if;

// init func for dict_set int->float
static i32
dict_set_if_0(CSOUND *csound, DICT_SET_if *p) {
    p->g = dict_globals(csound);
    p->lastidx = UI32MAX;   // mark this idx as invalid
    p->lastkey = UI32MAX;
    p->counter = 0;
    return OK;
}

// perf func for dict_set int->float
static i32
dict_set_if(CSOUND *csound, DICT_SET_if *p) {
    khint_t k;
    HANDLE *handle = get_handle(p);
    CHECK_HANDLE(handle);
    CHECK_HASHTAB_TYPE(handle->khtype, khIntFlt);
    khash_t(khIntFlt) *h = handle->hashtab;
    ui32 key = (ui32) *p->outkey;

    if(handle->counter == p->counter && key == p->lastkey) {
        k = p->lastidx;
    } else {
        int absent;    
        p->lastidx = k = kh_put(khIntFlt, h, key, &absent);
        p->lastkey = key;
        if (absent) {
            handle->counter++;
            if(handle->isglobal) {
                LOCK(handle);
                    kh_key(h, k) = key;
                UNLOCK(handle);
            } else {
                kh_key(h, k) = key;
            }      
        }
        p->counter = handle->counter;
    }
    kh_value(h, k) = *p->outval;
    return OK;
}

// init func to dict_set int->float at i-time
static i32
dict_set_if_i(CSOUND *csound, DICT_SET_if *p) {
    dict_set_if_0(csound, p);
    return dict_set_if(csound, p);
}

typedef struct {
    OPDS h;
    // out
    MYFLT *handleidx;
    // in
    STRINGDAT *outkey;
    MYFLT *outval;
    // internal
    HASH_GLOBALS *g;
    ui64 counter;
    khiter_t lastidx;
    i32 lastkey_size;
    char lastkey_data[KHASH_STRKEY_MAXSIZE+1];
} DICT_SET_sf;

// init func for dict_set str->float
static i32
dict_set_sf_0(CSOUND *csound, DICT_SET_sf *p) {
    HASH_GLOBALS *g = dict_globals(csound);
    p->g = g;
    p->lastkey_size = -1;
    p->lastidx = 0;
    p->counter = 0;
    p->lastkey_data[0] = '\0';
    return OK;
}

// perf func for dict_set str->float
static i32
dict_set_sf(CSOUND *csound, DICT_SET_sf *p) {
    HANDLE *handle = get_handle(p);
    CHECK_HANDLE(handle);
    CHECK_HASHTAB_TYPE(handle->khtype, khStrFlt);
    khash_t(khStrFlt) *h = handle->hashtab;
    int absent;
    khiter_t k;
    char *key;
    // test fastpath
    if(p->counter == handle->counter &&
       p->lastkey_size == p->outkey->size &&
       strcmp(p->lastkey_data, p->outkey->data)==0) {
        kh_value(h, p->lastidx) = *p->outval;
        return OK;
    }
    CHECK_KEY_SIZE(p->outkey);
    p->lastidx = k = kh_put(khStrFlt, h, p->outkey->data, &absent);
    if (absent) {
        key = csound->Strdup(csound, p->outkey->data);
        if(handle->isglobal) {
            LOCK(handle);
                kh_key(h, k) = key;
            UNLOCK(handle);
        } else
            kh_key(h, k) = key;
        handle->counter++;
    }
    kh_value(h, k) = *p->outval;
    strncpy0(p->lastkey_data, p->outkey->data, strlen(p->outkey->data));
    p->lastkey_size = p->outkey->size;
    p->counter = handle->counter;
    return OK;
}

// i-time dict_set str->float
static i32
dict_set_sf_i(CSOUND *csound, DICT_SET_sf *p) {
    dict_set_sf_0(csound, p);
    return dict_set_sf(csound, p);
}

// hashtab_set ihandle, Skey, kvalue
typedef struct {
    OPDS h;
    // out
    MYFLT *handleidx;
    // in
    STRINGDAT *outkey;
    STRINGDAT *outval;
    // internal
    HASH_GLOBALS *g;
    ui64 counter;
    khiter_t lastidx;
    i32 lastkey_size;
    char lastkey_data[KHASH_STRKEY_MAXSIZE+1];
} DICT_SET_ss;

// init func for dict_set str->str
static i32
dict_set_ss_0(CSOUND *csound, DICT_SET_ss *p) {
    p->g = dict_globals(csound);
    p->lastkey_size = -1;
    p->lastidx = 0;
    p->counter = 0;
    p->lastkey_data[0] = '\0';
    return OK;
}

static i32
dict_set_ss(CSOUND *csound, DICT_SET_ss *p) {
    HASH_GLOBALS *g = p->g;
    i32 idx = (i32)*p->handleidx;
    HANDLE *handle = &(g->handles[idx]);
    khash_t(khStrStr) *h = handle->hashtab;
    khint_t k;
    int absent;
    kstring_t *ks;
    CHECK_HASHTAB_EXISTS(h);
    CHECK_HASHTAB_TYPE(handle->khtype, khStrStr);

    // fastpath: dict was not changed and this key is unchanged, last index is valid
    if(p->counter == handle->counter &&
       p->outkey->size == p->lastkey_size &&
       !strcmp(p->outkey->data, p->lastkey_data)) {
        k = p->lastidx;
    } else {
        CHECK_KEY_SIZE(p->outkey);
        p->lastidx = k = kh_put(khStrStr, h, p->outkey->data, &absent);
        strncpy0(p->lastkey_data, p->outkey->data, (size_t)(p->outkey->size - 1));
        if (absent) {
            kh_key(h, k) = csound->Strdup(csound, p->outkey->data);
            ks = &(h->vals[k]);
            kstr_init_from_stringdat(csound, ks, p->outval);
            handle->counter++;
            p->counter = handle->counter;
            return OK;
        }
    }
    p->counter = handle->counter;
    ks = &(h->vals[k]);
    kstr_set_from_stringdat(csound, ks, p->outval);
    return OK;
}

// hashtab_set ihandle, Skey, kvalue
typedef struct {
    OPDS h;
    // out
    MYFLT *handleidx;
    // in
    MYFLT *outkey;
    STRINGDAT *outval;
    
    // internal
    HASH_GLOBALS *g;
    khint_t lastidx;
    ui32 lastkey;
    ui64 counter;
} DICT_SET_is;

static i32
dict_set_is_0(CSOUND *csound, DICT_SET_is *p) {
    p->g = dict_globals(csound);
    p->lastidx = UI32MAX;
    p->lastkey = UI32MAX;
    p->counter = 0;
    return OK;
}

static i32
dict_set_is(CSOUND *csound, DICT_SET_is *p) {
    HASH_GLOBALS *g = p->g;
    i32 idx = (i32)*p->handleidx;
    HANDLE *handle = &(g->handles[idx]);
    khash_t(khIntStr) *h = handle->hashtab;
    CHECK_HASHTAB_EXISTS(h);
    CHECK_HASHTAB_TYPE(handle->khtype, khIntStr);
    ui32 key = (ui32) *p->outkey;
    kstring_t *ks;
    khint_t k;
    if(handle->counter == p->counter && key == p->lastkey) {
        k = p->lastidx;
    } else {
        int absent;    
        p->lastidx = k = kh_put(khIntStr, h, key, &absent);
        p->lastkey = key;
        if (absent) {
            handle->counter++;
            p->counter = handle->counter;
            LOCK(handle);
                kh_key(h, k) = key;
                ks = &(h->vals[k]);
                kstr_init_from_stringdat(csound, ks, p->outval);
            UNLOCK(handle);
            return OK;
        }
    }
    p->counter = handle->counter;
    ks = &(h->vals[k]);
    kstr_set_from_stringdat(csound, ks, p->outval);
    return OK;
}

// -----------------------------------------------------
//                      DEL
// -----------------------------------------------------

/*
 * This removes a key:value pair from a dict. It is called
 * as dict_set without a value
 *
 */

typedef struct {
    OPDS h;
    MYFLT *handleidx;
    STRINGDAT *outkey;
} DICT_DEL_s;

static i32
dict_del_s(CSOUND *csound, DICT_DEL_s *p) {
    HASH_GLOBALS *g = dict_globals(csound);
    i32 idx = (i32)*p->handleidx;
    HANDLE *handle = &(g->handles[idx]);
    i32 khtype = handle->khtype;
    khiter_t k;
    kstring_t *ks;
    if(khtype == khStrFlt) {
        khash_t(khStrFlt) *h = handle->hashtab;
        CHECK_HASHTAB_EXISTS(h);
        k = kh_get(khStrFlt, h, p->outkey->data);
        if(k != kh_end(h)) {
            // key exists, free key str and remove item
            LOCK(handle);
            csound->Free(csound, kh_key(h, k));
            kh_del(khStrFlt, h, k);
            handle->counter++;
            UNLOCK(handle);
        }
    } else if(khtype == khStrStr) {
        khash_t(khStrStr) *h = handle->hashtab;
        CHECK_HASHTAB_EXISTS(h);
        k = kh_get(khStrStr, h, p->outkey->data);
        if(k != kh_end(h)) {
            // key exists, free key str and value, remove item
            LOCK(handle);
            csound->Free(csound, kh_key(h, k));
            ks = &(h->vals[k]);
            csound->Free(csound, ks->s);
            kh_del(khStrStr, h, k);
            handle->counter++;
            UNLOCK(handle);
        }
    }
    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *handleidx;
    MYFLT *outkey;
} DICT_DEL_i;

static i32
dict_del_i(CSOUND *csound, DICT_DEL_i *p) {
    HASH_GLOBALS *g = dict_globals(csound);
    i32 idx = (i32)*p->handleidx;
    HANDLE *handle = &(g->handles[idx]);
    i32 khtype = handle->khtype;
    khiter_t k;
    ui32 key = (ui32)*p->outkey;
    if(khtype == khIntFlt) {
        khash_t(khIntFlt) *h = g->handles[idx].hashtab;
        CHECK_HASHTAB_EXISTS(h);
        k = kh_get(khIntFlt, h, key);
        if(k != kh_end(h)) {
            // key exists, remove item
            kh_del(khIntFlt, h, k);
            handle->counter++;
        }
    } else if(khtype == khIntStr) {
        khash_t(khIntStr) *h = g->handles[idx].hashtab;
        CHECK_HASHTAB_EXISTS(h);
        k = kh_get(khIntStr, h, key);
        if(k != kh_end(h)) {
            // key exists, free value, remove item
            LOCK(handle);
            kstring_t *ks = &(kh_val(h, k));
            if(ks->s != NULL)
                csound->Free(csound, ks->s);
            kh_del(khIntStr, h, k);
            UNLOCK(handle);
            handle->counter++;
        }
    } else
        return PERFERRF(Str("dict: wrong type, expected 'if' or 'is', got %s"),
                        intdef_to_stringdef(khtype));
    return OK;
}

// ------------------------------------------------------
//                         GET
// ------------------------------------------------------

// kvalue dict_get ihandle, kkey, kdefault=0

typedef struct {
    OPDS h;
    MYFLT *kout;

    // inputs
    MYFLT *handleidx;
    MYFLT *outkey;
    MYFLT *defaultval;

    // internal
    HASH_GLOBALS *g;
    ui64 counter;
    khiter_t lastidx;
    ui32 lastkey;
} DICT_GET_if;

static i32
dict_get_if_0(CSOUND *csound, DICT_GET_if *p) {
    p->g = dict_globals(csound);
    p->lastidx = UI32MAX;
    p->lastkey = 0;
    p->counter = 0;
    return OK;
}

static i32
dict_get_if(CSOUND *csound, DICT_GET_if *p) {
    HASH_GLOBALS *g = p->g;
    i32 idx = (i32)*p->handleidx;
    HANDLE *handle = &(g->handles[idx]);
    khash_t(khIntFlt) *h = handle->hashtab;
    CHECK_HASHTAB_EXISTS(h);
    CHECK_HASHTAB_TYPE(handle->khtype, khIntFlt);
    khiter_t k;
    ui32 key = (ui32)*p->outkey;
    if(p->counter == handle->counter &&        // fast path
       p->lastkey == key) {
        k = p->lastidx;
        *p->kout = kh_val(h, k);
        return OK;
    } 
    p->lastidx = k = kh_get(khIntFlt, h, key);
    p->lastkey = key;
    *p->kout = k != kh_end(h) ? kh_val(h, k) : *p->defaultval;
    p->counter = handle->counter;
    return OK;
}

static i32
dict_get_if_i(CSOUND *csound, DICT_GET_if *p) {
    dict_get_if_0(csound, p);
    return dict_get_if(csound, p);
}

// kvalue dict_get ihandle, Skey (can be changed), kdefault=0

typedef struct {
    OPDS h;
    MYFLT *kout;
    // inputs
    MYFLT *handleidx;
    STRINGDAT *outkey;
    MYFLT *defaultval;
    // internal
    HASH_GLOBALS *g;
    khiter_t lastidx;
    ui32 _handleidx;
    i32 lastkey_size;
    ui64 counter;
    char lastkey_data[KHASH_STRKEY_MAXSIZE+1];
} DICT_GET_sf;

static i32
dict_get_sf_0(CSOUND *csound, DICT_GET_sf *p) {
    ui32 idx;
    p->g = dict_globals(csound);
    p->lastkey_size = 0;
    p->lastidx = 0;
    p->counter = 0;
    p->_handleidx = idx = (ui32)*p->handleidx;
    CHECK_HASHTAB_TYPE(p->g->handles[idx].khtype, khStrFlt);
    return OK;
}

static i32
dict_get_sf(CSOUND *csound, DICT_GET_sf *p) {
    /*
    // this implements changing the dict handle at k-time
    if(idx != p->_handleidx) {
        // the dict handle changed, so invalidate cache
        p->handleidx = idx;
        p->lastidx = -1;
        p->lastkey_size = -1;
        p->counter = 0;
        CHECK_HASHTAB_TYPE(p->khashglobals->handles[idx].khtype, khStrFlt);
    }
    */
    // use cached idx, not really necessary since we declared ihandle as i-var
    HANDLE *handle = &(p->g->handles[p->_handleidx]);
    khash_t(khStrFlt) *h = handle->hashtab;
    khiter_t k;
    // test fast path
    if(p->counter == handle->counter &&         \
       p->outkey->size == p->lastkey_size &&       \
       memcmp(p->outkey->data, p->lastkey_data, (size_t)p->lastkey_size)==0) {
       // strcmp(p->key->data, p->lastkey_data)==0) {
        k = p->lastidx;
        *p->kout = k != kh_end(h) ? kh_val(h, k) : *p->defaultval;
        return OK;
    } 
    CHECK_KEY_SIZE(p->outkey);
    p->lastidx = k = kh_get(khStrFlt, h, p->outkey->data);
    DBG("slow path: k=%d", k);
    p->lastkey_size = p->outkey->size;
    strncpy0(p->lastkey_data, p->outkey->data, (size_t)(p->outkey->size-1));
    *p->kout = k != kh_end(h) ? kh_val(h, k) : *p->defaultval;
    p->counter = handle->counter;
    return OK;
}

static i32
dict_get_sf_i(CSOUND *csound, DICT_GET_sf *p) {
    dict_get_sf_0(csound, p);
    return dict_get_sf(csound, p);
}

// kvalue dict_get idict, Skey (Skey can change)

typedef struct {
    OPDS h;
    // out
    STRINGDAT *outstr;

    // inputs
    MYFLT *handleidx;
    STRINGDAT *outkey;

    // internal
    HASH_GLOBALS *g;
    ui64 counter;
    khiter_t lastidx;
    i32 lastkey_size;
    char lastkey_data[KHASH_STRKEY_MAXSIZE+1];
} DICT_GET_ss;

static i32
dict_get_ss_0(CSOUND *csound, DICT_GET_ss *p) {
    p->g = dict_globals(csound);
    p->lastidx = 0;
    p->counter = 0;
    p->lastkey_size = 0;
    p->lastkey_data[0] = '\0';
    return OK;
}

static i32
hashtab_get_ss(CSOUND *csound, DICT_GET_ss *p) {
    // if the key is not found, an empty string is returned
    HASH_GLOBALS *g = p->g;
    i32 idx = (i32)*p->handleidx;
    HANDLE *handle = &(g->handles[idx]);
    khash_t(khStrStr) *h = g->handles[idx].hashtab;
    kstring_t *ks;
    khiter_t k;

    CHECK_HASHTAB_EXISTS(h);
    CHECK_HASHTAB_TYPE(handle->khtype, khStrStr);

    // test fast path
    if(p->counter == handle->counter &&
       p->lastkey_size == p->outkey->size &&
       !strcmp(p->lastkey_data, p->outkey->data)) {
        k = p->lastidx;
    } else {
        CHECK_KEY_SIZE(p->outkey);
        k = kh_get(khStrStr, h, p->outkey->data);
        if(k == kh_end(h)) {
            // key not found, set out to empty string
            p->outstr->data[0] = '\0';
            p->outstr->size = 1;
            return OK;
        }

        // key found, update cache
        p->lastidx = k;   // save last key index
        p->counter = handle->counter;
        strncpy0(p->lastkey_data, p->outkey->data, (size_t) p->outkey->size-1);
        p->lastkey_size = p->outkey->size;
    } 
    ks = &(kh_val(h, k));
    return stringdat_set(csound, p->outstr, ks->s, ks->l);
}


// kvalue dict_get ihandle, kkey, kdefault=0

typedef struct {
    OPDS h;
    STRINGDAT *outstr;
    // inputs
    MYFLT *handleidx;
    MYFLT *outkey;

    // internal
    HASH_GLOBALS *g;
    ui64 counter;
    khiter_t lastidx;
    ui32 lastkey;
} DICT_GET_is;


static i32
hashtab_get_is_0(CSOUND *csound, DICT_GET_is *p) {
    p->g = dict_globals(csound);
    p->lastidx = UI32MAX;
    p->lastkey = 0;
    p->counter = 0;
    HANDLE *handle = get_handle(p);
    CHECK_HANDLE(handle);
    CHECK_HASHTAB_TYPE(handle->khtype, khIntStr);
    return OK;
}

static i32
hashtab_get_is(CSOUND *csound, DICT_GET_is *p) {
    HANDLE *handle = get_handle(p);
    khash_t(khIntStr) *h = handle->hashtab;
    ui32 key = (ui32)*p->outkey;
    khiter_t k;
    kstring_t *ks;
    if(p->counter == handle->counter && p->lastkey == key) {  // fast path
        k = p->lastidx;
    } else {
        k = kh_get(khIntStr, h, key);
        if(k == kh_end(h)) {   // key not found
            p->outstr->data[0] = '\0';
            p->outstr->size = 1;
            return OK;
        }
        p->lastidx = k;
        p->lastkey = key;
        p->counter = handle->counter;
    } 
    ks = &(kh_val(h, k));
    return stringdat_set(csound, p->outstr, ks->s, ks->l);
}


// ------------------------------
//           FREE
// -------------------------------

/**
 * Free (destroy) a dict now or at the end of this note
 *
 * dict_free ihandle, iwhen (0=at init time, 1=end of note)
 *
 * modelled after ftfree
 *
 * NB: dict_free can only be called with a global dict (isglobal=1)
 *
 */

typedef struct {
    OPDS h;
    MYFLT *handleidx;
    MYFLT *iwhen;
} DICT_FREE;

static i32
dict_free_callback(CSOUND *csound, DICT_FREE *p) {
    ui32 idx = (ui32)*p->handleidx;
    HASH_GLOBALS *g = dict_globals(csound);
    if(g->handles[idx].hashtab == NULL)
        return PERFERR(Str("dict free: instance does not exist!"));
    return _dict_free(csound, g, idx);
}

/**
 * modelled after ftfree - works at init time
 * Only global dicts can be freed in this way. Local dicts are deallocated
 * automatically at the end of the note
 */
static i32
dict_free(CSOUND *csound, DICT_FREE *p) {
    ui32 idx = (ui32)*p->handleidx;
    HASH_GLOBALS *g = dict_globals(csound);
    HANDLE *handle = &(g->handles[idx]);
    if(handle->hashtab == NULL)
        return PERFERR(Str("dict free: instance does not exist"));
    if(handle->isglobal == 0)
        return PERFERR(Str("dict free: only global dicts can be freed"));
    if((i32)*p->iwhen == 0) {
        return _dict_free(csound, g, idx);
    }
    // free at the end of the note
    csound->RegisterDeinitCallback(csound, p,
                                   (i32 (*)(CSOUND*, void*))dict_free_callback);
    return OK;
}

// -----------------------------------
//            PRINT
// -----------------------------------

// dict_print ihandle, [ktrig]

typedef struct {
    OPDS h;
    MYFLT *handleidx;
    MYFLT *ktrig;
    HASH_GLOBALS *g;
    MYFLT lasttrig;
} DICT_PRINT;


static i32
_dict_print(CSOUND *csound, DICT_PRINT *p, HANDLE *handle) {
    int khtype = handle->khtype;
    khint_t k;
    i32 chars = 0;
    // linelength could be changed dynamically in the future
    const i32 linelength = 80;
    char line[256];
    if(khtype == khIntFlt) {
        khash_t(khIntFlt) *h = handle->hashtab;
        for(k = kh_begin(h); k != kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;            
            chars += sprintf(line+chars, "%d: %.5f", kh_key(h, k), kh_value(h, k));
            if(chars < linelength) {
                line[chars++] = '\t';
            } else {
                line[chars+1] = '\0';
                csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)line);
                chars = 0;
            }
        }
    } else if(khtype == khIntStr) {
        khash_t(khIntStr) *h = handle->hashtab;
        for(k = kh_begin(h); k != kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;
            chars += sprintf(line+chars, "%d: %s", kh_key(h, k), kh_val(h, k).s);
            if(chars < linelength) {
                line[chars++] = '\t';
            } else {
                line[chars+1] = '\0';
                csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)line);
                chars = 0;
            }
        }
    } else if(khtype == khStrFlt) {
        khash_t(khStrFlt) *h = handle->hashtab;
        for(k = kh_begin(h); k != kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;                      
            chars += sprintf(line+chars, "%s: %.5f", kh_key(h, k), kh_val(h, k));
            if(chars < linelength) {
                line[chars++] = '\t';
            } else {
                line[chars+1] = '\0';
                csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)line);
                chars = 0;
            }
        }
    } else if(khtype == khStrStr) {
        khash_t(khStrStr) *h = handle->hashtab;
        for(k = kh_begin(h); k != kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;                      
            chars += sprintf(line+chars, "%s: %s", kh_key(h, k), kh_val(h, k).s);
            if(chars < linelength) {
                line[chars++] = '\t';
            } else {
                line[chars+1] = '\0';
                csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)line);
                chars = 0;
            }
        }
    } else
        return PERFERR(Str("dict: format not supported"));
    // last line
    if(chars > 0) {
        line[chars] = '\0';
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)line);
    }
    return OK;
}

static i32
dict_print_i(CSOUND *csound, DICT_PRINT *p) {
    HASH_GLOBALS *g = dict_globals(csound);
    HANDLE *handle = &(g->handles[(int)*p->handleidx]);
    _dict_print(csound, p, handle);
    return OK;
}

static i32
dict_print_k_0(CSOUND *csound, DICT_PRINT *p) {
    p->g = dict_globals(csound);
    p->lasttrig = 0;
    return OK;
}

static i32
dict_print_k(CSOUND *csound, DICT_PRINT *p) {
    HANDLE *handle = get_handle(p);
    i32 trig = (i32) *p->ktrig;
    if(handle->hashtab == NULL)
        return PERFERR(Str("dict does not exist"));
    if(trig == -1 || (trig > 0 && trig > p->lasttrig)) {
        p->lasttrig = trig;
        _dict_print(csound, p, handle);
    }
    return OK;
}

// --------------------------------------------------
//                    QUERY
// --------------------------------------------------

/*
 * kout dict_query idict, Scmd
 *
 * Query groups together different commands to get some information
 * about the dictionary without having to create a specific opcode
 *
 *  Commands     Action
 *
 *   size        the number of key:value pairs
 *   exists      does this idx point to a valid slot?
 *   type        the type of this dict as an int signature
 *   keys        the keys of this dict, as an array (k or S, depending on the type)
 *   values      the values of this dict, as an array (k or S, depending on the type)
 *
 */


typedef struct {
    OPDS h;
    MYFLT *out;

    MYFLT *handleidx;
    STRINGDAT *cmdstr;

    HASH_GLOBALS *g;
    i32 cmd;
} DICT_QUERY1;

static i32 dict_size(CSOUND *csound, DICT_QUERY1 *p);

static i32 dict_query(CSOUND *csound, DICT_QUERY1 *p) {
    HANDLE *handle;
    switch(p->cmd) {
    case 0:  // size
        return dict_size(csound, p);
    case 1:  // type
        handle = get_handle_check(p);
        *p->out = (handle == NULL || handle->hashtab == NULL) ? 0 : handle->khtype;
        return OK;
    case 2:  // exists
        handle = get_handle_check(p);
        *p->out = (handle == NULL || handle->hashtab == NULL) ? 0 : 1;
        return OK;
    }
    return OK;
}

// init func for dict_query
static i32 dict_query_0(CSOUND *csound, DICT_QUERY1 *p) {
    p->g = dict_globals(csound);
    char *data = p->cmdstr->data;
    if(strcmp(data, "size")==0) {
        p->cmd = 0;
    } else if(strcmp(data, "type")==0) {
        p->cmd = 1;
    } else if(strcmp(data, "exists")==0) {
        p->cmd = 2;
    } else
        return INITERRF("dict query: cmd %s not supported", p->cmdstr->data);
    dict_query(csound, p);
    return OK;
}

// dict_size: returns the number of key:valie pairs, -1 if dict does not exist

static i32 dict_size(CSOUND *csound, DICT_QUERY1 *p) {
    IGN(csound);
    HANDLE *handle = get_handle(p);
    if(handle->hashtab == NULL)
        *p->out = -1;
    with_hashtable(handle, {
        *p->out = kh_size(h);
    })
    return OK;
}

static i32 dict_size_0(CSOUND *csound, DICT_QUERY1 *p) {
    p->g = dict_globals(csound);
    // works also at init time
    return dict_size(csound, p);
}

// used for queries which return an array: "keys", "values"
typedef struct {
    OPDS h;
    ARRAYDAT *out;

    MYFLT *handleidx;
    STRINGDAT *cmdstr;

    i32 cmd;
    HASH_GLOBALS *g;
} DICT_QUERY_ARR;


// return the string keys as an array
static i32
dict_query_arr_keys_s(CSOUND *csound, HANDLE *handle, ARRAYDAT *out) {
    STRINGDAT *outdata = (STRINGDAT*)(out->data);
    ui32 counter = 0;
    i32 khtype = handle->khtype;
    const char *key;
    if(khtype == khStrFlt) {
        khash_t(khStrFlt) *h = handle->hashtab;
        kh_foreach_key(h, key, {
            stringdat_set(csound, &(outdata[counter++]), key, strlen(key));
        });
    } else {
        khash_t(khStrStr) *h = handle->hashtab;
        kh_foreach_key(h, key, {
            stringdat_set(csound, &(outdata[counter++]), key, strlen(key));
        });
    }
    return OK;
}

// return the integer keys as an array
static i32
dict_query_arr_keys_i(CSOUND *csound, HANDLE *handle, ARRAYDAT *out) {
    IGN(csound);
    MYFLT *outdata = (MYFLT*)(out->data);
    ui32 counter = 0;
    i32 khtype = handle->khtype;
    ui32 key;
    if(khtype == khIntFlt) {
        khash_t(khIntFlt) *h = handle->hashtab;
        kh_foreach_key(h, key, { outdata[counter++] = key; });
    } else {
        khash_t(khIntStr) *h = handle->hashtab;
        kh_foreach_key(h, key, { outdata[counter++] = key; });
    }
    return OK;
}

// return the string values as an array
static i32
dict_query_arr_values_s(CSOUND *csound, HANDLE *handle, ARRAYDAT *out) {
    STRINGDAT *outdata = (STRINGDAT*)(out->data);
    i32 counter=0, khtype=handle->khtype;
    kstring_t *ks;
    if(khtype == khIntStr) {
        khash_t(khIntStr) *h = handle->hashtab;
        for(khiter_t k=0; k!=kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;
            ks = &(kh_val(h, k));
            stringdat_set(csound, &(outdata[counter++]), ks->s, ks->l);
        }
    } else {
        khash_t(khStrStr) *h = handle->hashtab;
        for(khiter_t k=0; k!=kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;
            ks = &(kh_val(h, k));
            stringdat_set(csound, &(outdata[counter++]), ks->s, ks->l);
        }
    }
    return OK;
}

// return the numeric values as an array
static i32
dict_query_arr_values_f(CSOUND *csound, HANDLE *handle, ARRAYDAT *out) {
    IGN(csound);
    MYFLT *outdata = (MYFLT*)(out->data);
    MYFLT val;
    i32 counter=0, khtype=handle->khtype;
    if(khtype == khIntFlt) {
        khash_t(khIntFlt) *h = handle->hashtab;
        kh_foreach_val(h, val, { outdata[counter++] = val; });
    } else {
        khash_t(khStrFlt) *h = handle->hashtab;
        kh_foreach_val(h, val, { outdata[counter++] = val; });
    }
    return OK;
}

static ui32 handle_get_hashtable_size(HANDLE *handle) {
    with_hashtable(handle, {
        return kh_size(h);
    })
}


// dict_query_arr: query operatins returning an array (keys, values)
static i32
dict_query_arr(CSOUND *csound, DICT_QUERY_ARR *p) {
    ui32 idx = (ui32)*(p->handleidx);
    HANDLE *handle = &(p->g->handles[idx]);
    CHECK_HANDLE(handle);

    ui32 size = handle_get_hashtable_size(handle);
    tabensure(csound, p->out, (i32) size);

    switch(p->cmd) {
    case 0:  // keys, string
        return dict_query_arr_keys_s(csound, handle, p->out);
    case 1:  // keys, numeric
        return dict_query_arr_keys_i(csound, handle, p->out);
    case 2:  // values, string
        return dict_query_arr_values_s(csound, handle, p->out);
    case 3:  // values, numeric
        return dict_query_arr_values_f(csound, handle, p->out);
    default:
        return PERFERRF("internal error: invalid cmd (%d)", p->cmd);
    }
}


// init func for dict_query when expecting an array
static i32
dict_query_arr_0(CSOUND *csound, DICT_QUERY_ARR *p) {
    p->g = dict_globals(csound);
    char *data = p->cmdstr->data;
    HANDLE *handle = get_handle(p);
    char *vartypename = p->out->arrayType->varTypeName;
    if(strcmp(data, "keys")==0) {
        if(handle->khtype == 21 || handle->khtype == 22) {
            // str keys, check output array
            if(vartypename[0] != 'S')
                return INITERRF(Str("Expected out array of type S, got %s"),
                                vartypename);
            p->cmd = 0;
        } else {
            // integer keys, check
            if(vartypename[0] != 'k')
                return INITERRF(Str("Expected out array of type k, got %s"),
                                vartypename);
            p->cmd = 1;
        }
    } else if(strcmp(data, "values")==0) {
        if(handle->khtype == 12 || handle->khtype == 22) {
            // string values, check
            if(vartypename[0] != 'S')
                return INITERRF(Str("Expected out array of type S, got %s"),
                                vartypename);
            p->cmd = 2;
        } else {
            // integer values, check
            if(vartypename[0] != 'k')
                return INITERRF(Str("Expected out array of type k, got %s"),
                                vartypename);
            p->cmd = 3;
        }
    } else
        return INITERRF("dict query: cmd %s not supported", p->cmdstr->data);
    dict_query_arr(csound, p);
    return OK;
}


// ----------------------------------------------
//                    ITER
// ----------------------------------------------

/**
 * dict_iter
 *
 * xkey, xvalue, kidx  dict_iter idict, kreset=1
 *
 *   kreset: the reseting policy.
 *     0: do not reset. This will iterate at most once over the pairs, and stop
 *     1: reset at the beginning of every k-cycle (the default)
 *     2: reset at the end of iteration
 *
 * xkey: the key of this pair
 * xvalue: the value of this pair
 * kidx: the index of this pair (will be -1 when reached end of iteration)
 *
 */

typedef struct {
    OPDS h;
    // out
    void *outkey;
    void *outval;
    MYFLT *kidx;
    // in
    MYFLT *handleidx;
    MYFLT *kreset;
    // internal
    HASH_GLOBALS *g;
    ui32 _handleidx;
    i32 numyields;    // number of yields since last reset
    khiter_t nextk;        // iteration index
    ui64 kcounter;
    char signature[3];
} DICT_ITER;

// dict_iter init function common to all types
static i32
dict_iter_init_common(CSOUND *csound, DICT_ITER *p) {
    p->_handleidx = (ui32)*p->handleidx;
    p->g = dict_globals(csound);
    p->nextk = 0;
    p->numyields = 0;
    p->kcounter = 999;
    HANDLE *handle = &(p->g->handles[p->_handleidx]);
    khash_t(khStrStr) *h = handle->hashtab;
    CHECK_HASHTAB_EXISTS(h);
    char *dictsig = intdef_to_stringdef(handle->khtype);
    if(strcmp(dictsig, p->signature) != 0)
        return INITERRF("Own signature is %s, but the dict has a type %s",
                        p->signature, dictsig);
    return OK;
}

static i32 dict_iter_sf_0(CSOUND *csound, DICT_ITER *p) {
    strcpy(p->signature, "sf");
    return dict_iter_init_common(csound, p);
}

static i32 dict_iter_ss_0(CSOUND *csound, DICT_ITER *p) {
    strcpy(p->signature, "ss");
    return dict_iter_init_common(csound, p);
}

static i32 dict_iter_is_0(CSOUND *csound, DICT_ITER *p) {
    strcpy(p->signature, "is");
    return dict_iter_init_common(csound, p);
}

static i32 dict_iter_if_0(CSOUND *csound, DICT_ITER *p) {
    strcpy(p->signature, "if");
    return dict_iter_init_common(csound, p);
}

static i32
dict_iter_perf(CSOUND *csound, DICT_ITER *p) {
    i32 kreset = (i32) *p->kreset;
    if(kreset == 1) {
        // reset at every new cycle
        if(p->h.insdshead->kcounter != p->kcounter) {
            p->kcounter = p->h.insdshead->kcounter;
            p->numyields = 0;
            p->nextk = 0;
        }
    }
    HANDLE *handle = &(p->g->handles[p->_handleidx]);
    kstring_t *kstr;
    i32 khtype = handle->khtype;
    if(khtype == khStrStr) {
        khash_t(khStrStr) *h = handle->hashtab;
        CHECK_HASHTAB_EXISTS(h);
        for(khiter_t k=p->nextk; k != kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;
            const char *key = kh_key(h, k);
            stringdat_set(csound, (STRINGDAT*)p->outkey, key, strlen(key));
            kstr = &(kh_val(h, k));
            stringdat_set(csound, (STRINGDAT*)p->outval, kstr->s, kstr->l);
            p->nextk = k + 1;
            *p->kidx = p->numyields;
            p->numyields++;
            return OK;
        }
    } else if (khtype == khStrFlt) {
        khash_t(khStrFlt) *h = handle->hashtab;
        CHECK_HASHTAB_EXISTS(h);
        for(khiter_t k=p->nextk; k != kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;
            const char *key = kh_key(h, k);
            stringdat_set(csound, (STRINGDAT*)p->outkey, key, strlen(key));
            *((MYFLT*)p->outval) = kh_val(h, k);
            p->nextk = k+1;
            *p->kidx = p->numyields;
            p->numyields++;
            return OK;
        }
    } else if(khtype == khIntStr ) {
        khash_t(khIntStr) *h = handle->hashtab;
        CHECK_HASHTAB_EXISTS(h);
        for(khiter_t k=p->nextk; k != kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;
            *((MYFLT*)p->outkey) = kh_key(h, k);
            kstr = &(kh_val(h, k));
            stringdat_set(csound, (STRINGDAT*)p->outval, kstr->s, kstr->l);
            p->nextk = k+1;
            *p->kidx = p->numyields;
            p->numyields++;
            return OK;
        }
    } else if(khtype == khIntFlt) {
        khash_t(khIntFlt) *h = handle->hashtab;
        CHECK_HASHTAB_EXISTS(h);
        for(khiter_t k=p->nextk; k != kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;
            *((MYFLT*)p->outkey) = kh_key(h, k);
            *((MYFLT*)p->outval) = kh_val(h, k);
            p->nextk = k+1;
            *p->kidx = p->numyields;
            p->numyields++;
            return OK;
        }
    } else
        return PERFERRF("dict: type %d not supported", khtype);
    // finished iteration! signal stop and reset if autoreset (kreset == -1)
    *p->kidx = -1;
    if (kreset == 2) {  // reset at the end of iteration
        p->nextk = 0;
        p->numyields = 0;
    }
    return OK;
}


static i32
set_many_ss(CSOUND *csound, void** inargs, ui32 numargs, HANDLE *handle) {
    khash_t(khStrStr) *h = handle->hashtab;
    int absent;
    kstring_t *ks;
    STRINGDAT *key, *val;
    for(ui32 argidx=0; argidx < numargs; argidx+=2) {
        key = (STRINGDAT *)inargs[argidx];
        val = (STRINGDAT *)inargs[argidx+1];
        khiter_t k = kh_put(khStrStr, h, key->data, &absent);
        ks = &(h->vals[k]);
        if(absent) {
            kh_key(h, k) = csound->Strdup(csound, key->data);
            kstr_init_from_stringdat(csound, ks, val);
        } else
            kstr_set_from_stringdat(csound, ks, val);
    }
    handle->counter++;
    return OK;
}

static i32
set_many_sf(CSOUND *csound, void** inargs, ui32 numargs, HANDLE *handle) {
    khash_t(khStrFlt) *h = handle->hashtab;
    int absent;
    for(ui32 argidx=0; argidx < numargs; argidx+=2) {
        STRINGDAT *key = (STRINGDAT *)inargs[argidx];
        khiter_t k = kh_put(khStrFlt, h, key->data, &absent);
        if(absent) {
            kh_key(h, k) = csound->Strdup(csound, key->data);
        }
        kh_value(h, k) = *((MYFLT*)(inargs[argidx+1]));
    }
    handle->counter++;
    return OK;
}

static i32
set_many_if(CSOUND *csound, void** inargs, ui32 numargs, HANDLE *handle) {
    IGN(csound);
    khash_t(khIntFlt) *h = handle->hashtab;
    int absent;
    for(ui32 argidx=0; argidx < numargs; argidx+=2) {
        ui32 key = (ui32) *((MYFLT*)(inargs[argidx]));
        khiter_t k = kh_put(khIntFlt, h, key, &absent);
        if(absent) {
            kh_key(h, k) = key;
        }
        kh_value(h, k) = *((MYFLT*)(inargs[argidx+1]));
    }
    handle->counter++;
    return OK;
}

static i32
set_many_is(CSOUND *csound, void** inargs, ui32 numargs, HANDLE *handle) {
    khash_t(khIntStr) *h = handle->hashtab;
    int absent;
    kstring_t *ks;
    STRINGDAT *val;
    for(ui32 argidx=0; argidx < numargs; argidx+=2) {
        ui32 key = (ui32) *((MYFLT*)(inargs[argidx]));
        val = (STRINGDAT *)inargs[argidx+1];
        khiter_t k = kh_put(khIntStr, h, key, &absent);
        ks = &(h->vals[k]);
        if(absent) {
            kh_key(h, k) = key;
            kstr_init_from_stringdat(csound, ks, val);
        } else
            kstr_set_from_stringdat(csound, ks, val);
    }
    handle->counter++;
    return OK;
}

#define S(x) sizeof(x)

static OENTRY localops[] = {
    { "dict_new", S(DICT_NEW), 0, 1, "i", "So", (SUBR)dict_new },
    { "dict_new", S(DICT_NEW), 0, 1, "i", "Si*", (SUBR)dict_new_many },

    { "dict_free",S(DICT_FREE),   0, 1, "", "io",   (SUBR)dict_free},
    
    { "dict_get", S(DICT_GET_sf), 0, 3, "k", "iSO", (SUBR)dict_get_sf_0, (SUBR)dict_get_sf },
    { "dict_get", S(DICT_GET_ss), 0, 3, "S", "iS",  (SUBR)dict_get_ss_0, (SUBR)hashtab_get_ss },
    { "dict_get", S(DICT_GET_sf), 0, 1, "i", "iSo", (SUBR)dict_get_sf_i},
    { "dict_get", S(DICT_GET_if), 0, 3, "k", "ikO", (SUBR)dict_get_if_0, (SUBR)dict_get_if },
    { "dict_get", S(DICT_GET_is), 0, 3, "S", "ik",  (SUBR)hashtab_get_is_0, (SUBR)hashtab_get_is },
    { "dict_get", S(DICT_GET_if), 0, 1, "k", "iio", (SUBR)dict_get_if_i},

    { "dict_set", S(DICT_SET_ss), 0, 3, "",  "iSS",  (SUBR)dict_set_ss_0, (SUBR)dict_set_ss },
    { "dict_set", S(DICT_SET_sf), 0, 3, "",  "iSk",  (SUBR)dict_set_sf_0, (SUBR)dict_set_sf },
    { "dict_set", S(DICT_SET_sf), 0, 1, "",  "iSi",  (SUBR)dict_set_sf_i},
    { "dict_set", S(DICT_SET_if), 0, 3, "",  "ikk",  (SUBR)dict_set_if_0, (SUBR)dict_set_if },
    { "dict_set", S(DICT_SET_if), 0, 1, "",  "iii",  (SUBR)dict_set_if_i},
    { "dict_set", S(DICT_SET_is), 0, 3, "",  "ikS",  (SUBR)dict_set_is_0, (SUBR)dict_set_is },

    // { "dict_del", S(DICT_DEL_i),   0, 2, "", "ik",   NULL, (SUBR)dict_del_i },
    { "dict_set", S(DICT_DEL_i),   0, 2, "", "ik",   NULL, (SUBR)dict_del_i },
    // { "dict_del", S(DICT_DEL_i),   0, 1, "", "ii", (SUBR)dict_del_i },
    { "dict_set", S(DICT_DEL_i),   0, 1, "", "ii",   (SUBR)dict_del_i },
    // { "dict_del", S(DICT_DEL_s),   0, 2, "", "iS",   NULL, (SUBR)dict_del_s },
    { "dict_set", S(DICT_DEL_s),   0, 2, "", "iS",   NULL, (SUBR)dict_del_s },


    { "dict_print", S(DICT_PRINT), 0, 1, "", "i",  (SUBR)dict_print_i},
    { "dict_print", S(DICT_PRINT), 0, 3, "", "ik", (SUBR)dict_print_k_0, (SUBR)dict_print_k},

    { "dict_query", S(DICT_QUERY1), 0, 3, "k", "iS", (SUBR)dict_query_0, (SUBR)dict_query },
    { "dict_query.k[]", S(DICT_QUERY_ARR), 0, 3, "k[]", "iS", (SUBR)dict_query_arr_0, (SUBR)dict_query_arr},
    { "dict_query.S[]", S(DICT_QUERY_ARR), 0, 3, "S[]", "iS", (SUBR)dict_query_arr_0, (SUBR)dict_query_arr},


    { "dict_iter", S(DICT_ITER), 0, 3, "SSk", "iP", (SUBR)dict_iter_ss_0, (SUBR)dict_iter_perf},
    { "dict_iter", S(DICT_ITER), 0, 3, "Skk", "iP", (SUBR)dict_iter_sf_0, (SUBR)dict_iter_perf},
    { "dict_iter", S(DICT_ITER), 0, 3, "kSk", "iP", (SUBR)dict_iter_is_0, (SUBR)dict_iter_perf},
    { "dict_iter", S(DICT_ITER), 0, 3, "kkk", "iP", (SUBR)dict_iter_if_0, (SUBR)dict_iter_perf},

    { "dict_size", S(DICT_QUERY1), 0, 3, "k", "k", (SUBR)dict_size_0, (SUBR)dict_size},

};

LINKAGE
