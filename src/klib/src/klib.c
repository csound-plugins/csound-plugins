/*
   klib.c

  Copyright (C) 2019 Eduardo Moguillansky

  This file is part of Csound.

  The Csound Library is free software; you can rediibute it
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

    ihandle dict_new Stype

    Stype: the type definition, a 2-char string defining the type of
           the key (either string or int) and the value (either string or float)
           s = string
           i = int
           f = float
           Alternatively the signature can be spelled out as "str:float", "str:str",
           "int:float", "int:str"

           By default all dictionaries are local and are freed at note release. If
           Stype is preceded by a "*", the dict is global and needs to be manually
           deallocated.

    idict dict_new "sf"    ; create a hashtable of type string -> number
                           ; hashtable is freed at note end
    idict dict_new "*if"   ; create a hashtable of type int -> number
                           ; hahstable is global, survives the note

    It is also possible to initialize key:value pairs at init time, for example:

    ihandle dict_new "*str:float", "foo", 10, "bar", 20, "baz", 0.5

  dict_free
  =========

    dict_free idict [, iwhen=1]

    frees the hashtable either at init time or at the end of the note
    (similar to ftfree)
    This is needed when passing a hashtable from one instrument to another

    iwhen = 0  -> free now
    iwhen = 1  -> (default) free at the end of this note

  dict_set
  ========

    dict_set idict, Skey, kvalue        ; k-time
    dict_set idict, kkey, kvalue        ; k-time
    dict_set idict, ikey, ivalue        ; i-time

    Set a key:value pair

    idict dict_new "sf"
    dict_set idict, "key", kvalue

    If a value is not given, the key:value pair is deleted (if it exists)

    NB: to delete a value at stop, use dict_del

  dict_get
  ========

    Get the value at a given key. For string keys, an empty string
    is returned when the key is not found. For int keys, either 0
    or a default value given by the user are returned when the
    key is not found. This default value is also returned if the
    dict does not exist, so dict_get never fails in performance
    (see dict_size or dict_query to see how to check if the dict
    is valid)

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

  dict_del
  ========

    dict_del idict, Skey
    dict_del idict, ikey

    Delete the key:pair at the end of this note

  dict_clear
  ==========

    dict_clear idict
    dict_clear kdict

  TODO: make the dicts share the keys, by interning all keys into a global set (a hash-table
  without values). This would make dict cloning much easier and efficient, making it able
  to use it as an object system

  cache opcodes
  =============

  The cache opcodes implement an efficient string cache, usable to pass string
  between instruments, as symbols, etc.

  The following operations are defined:

  * cacheput: put a string in the string cache. Return the unique identifier idx

    idx cacheput Ss

  * cacheget: get a string from the cache

    Ss cacheget idx

*/

#include "csdl.h"
#include "arrays.h"

#include "khash.h"
#include "ukstring.h"
#include <ctype.h>

#include "../../common/_common.h"


typedef int32_t i32;
typedef uint32_t ui32;
typedef int64_t i64;
typedef uint64_t ui64;

#define KHASH_STRKEY_MAXSIZE 127
#define HANDLES_INITIAL_SIZE 200
#define DICT_INITIAL_SIZE 8
#define FLOAT_FMT "%.10g"

/*
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

#define INITERR(m) (csound->InitError(csound, "%s", m))
#define INITERRF(fmt, ...) (csound->InitError(csound, fmt, __VA_ARGS__))
#define PERFERR(m) (csound->PerfError(csound, &(p->h), "%s", m))
#define PERFERRF(fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))
#define MSGF(fmt, ...) (csound->Message(csound, fmt, __VA_ARGS__))
*/

#define ARRAYCHECK(arr, size) (tabcheck(csound, arr, size, &(p->h)))

#define UI32MAX 0x7FFFFFFF

// #define DEBUG

#ifdef DEBUG
    #define DBG(fmt, ...) printf("  "fmt"\n", __VA_ARGS__); fflush(stdout);
    #define DBG_(m) DBG("%s", m)
#else
    #define DBG(fmt, ...)
    #define DBG_(m)
#endif

#define ERR_NOINSTANCE Str("dict: instance doesn't exist")
#define GLOBALS_NAME "em.khash_globals"

#define CHECK_HASHTAB_EXISTS(h) {if(UNLIKELY((h)==NULL)) return PERFERR(ERR_NOINSTANCE);}

#define CHECK_HANDLE(handle) {if((handle)->hashtab==NULL) return PERFERR(ERR_NOINSTANCE);}
#define CHECK_HANDLE_INIT(handle) {if((handle)->hashtab==NULL) return INITERR(ERR_NOINSTANCE);}

#define CHECK_INDEX(idx, globals) {if((idx) < 0 || (idx) >= (globals->maxhandles)) return INITERRF("Handle index invalid: %d", (idx));  }

#define CHECK_HASHTAB_TYPE(handletype, expectedtype)                                       \
    if(UNLIKELY((handletype) != (expectedtype))) {                                         \
        csound->InitError(csound, Str("Expected a dict of type '%s', got '%s'"),           \
        intdef_to_strdef(expectedtype), intdef_to_strdef(handletype));                     \
        return NOTOK; }

#define CHECK_HASHTAB_TYPE2(handletype, type1, type2)                                      \
    if(UNLIKELY((handletype) != (type1) && (handletype) != (type2))) {                     \
        csound->InitError(csound, Str("Expected a dict of type '%s' or '%s, got '%s'"),    \
        intdef_to_strdef(type1), intdef_to_strdef(type2), intdef_to_strdef(handletype));   \
        return NOTOK; }


// s: a STRINGDAT* key
#define CHECK_KEY_SIZE(s)                                               \
    if(UNLIKELY(strlen((s)->data) > KHASH_STRKEY_MAXSIZE))                      \
        return PERFERRF(Str("dict: key too long (%d > %d)"),            \
                        (int)strlen((s)->data), KHASH_STRKEY_MAXSIZE)

// p: an opcode struct with a member 'g':KHASH_GLOBALS* and input handleidx:MYFLT*
#define get_handle_check(p)  ((ui32)*(p)->handleidx < p->g->maxhandles ? &((p)->g->handles[(ui32)*(p)->handleidx]) : NULL)

#define get_handle(p) (&((p)->g->handles[(ui32)*(p)->handleidx]))

#ifdef CSOUNDAPI6
#define register_deinit(csound, p, func) \
    csound->RegisterDeinitCallback(csound, p, (int32_t(*)(CSOUND*, void*))(func))
#endif


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

static inline void
kstr_from_cstr(CSOUND *csound, kstring_t *ks, char *s) {
    // this assumes that the ks string is not initialized,
    ks->s = csound->Strdup(csound, s);
    ks->l = strlen(s);
    ks->m = ks->l + 1;
}

// init ks to s
static inline void
kstr_init_from_stringdat(CSOUND *csound, kstring_t *ks, STRINGDAT *s) {
    kstr_from_cstr(csound, ks, s->data);
}

// like strncpy but really makes sure that the dest str is 0 terminated
static inline
void strncpy0(char *dest, const char *src, size_t n) {
    strncpy(dest, src, n);
    dest[n] = '\0';
}

// Set a STRINGDAT* from a source string and its length
static inline i32
stringdat_set(CSOUND *csound, STRINGDAT *s, const char *src, size_t srclen) {
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

static inline i32
stringdat_view(CSOUND *csound, STRINGDAT *s, char *src, size_t allocatedsize) {
    if(s->data != NULL)
        csound->Free(csound, s->data);
    s->data = src;
    s->size = allocatedsize;
    return OK;
}

static inline i32
stringdat_view_init(CSOUND *csound, STRINGDAT *s) {
    if(s->data != NULL) {
        // The outstring has been already initialized
        return INITERRF("Reusing a string here is not allowed. Previous value was: %s", s->data);

    }
    // s->data = NULL;
    s->size = 0;
    return OK;
}


// move src to s, s now owns src allocated storage
static inline i32
stringdat_move(CSOUND *csound, STRINGDAT *s, char *src, size_t allocatedsize) {
    if(s->data != NULL)
        csound->Free(csound, s->data);
    s->data = src;
    s->size = (i32) allocatedsize;
    return OK;
}

// float=1, str=2
enum {
    khStrFlt = 21,
    khStrStr = 22,
    khIntStr = 12,
    khIntFlt = 11,
    khStrInt = 20,  // used by the cache opcodes
    khStrAny = 23
};

// Initialize all possible types
KHASH_MAP_INIT_STR(khStrFlt, MYFLT)
KHASH_MAP_INIT_STR(khStrStr, kstring_t)
KHASH_MAP_INIT_INT(khIntFlt, MYFLT)
KHASH_MAP_INIT_INT(khIntStr, kstring_t)

// this is used by the cache opcodes
KHASH_MAP_INIT_STR(khStrInt, i64)


typedef struct {
    CSOUND *csound;
    int khtype;
    ui64 counter;
    void *hashtab;
    void *hashtab2;   // used by the "any" type to support both float and string values
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
    ui32 *slots;
    ui32 maxslots;
    ui32 lastslot;
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
    MYFLT *out_handleidx;
    // inputs
    STRINGDAT *keyvaltype;
    void  *inargs[VARGMAX];
    // internal
    HASH_GLOBALS *g;
    i32 idx;
    int khtype;
    int global;
} DICT_NEW;


// forward declarations
static i32 dict_reset(CSOUND *csound, HASH_GLOBALS *g);
static i32 _dict_free(CSOUND *csound, HASH_GLOBALS *g, ui32 idx);

static i32 set_many_ss(CSOUND *cs, void** inargs, ui32 numargs, HANDLE *handle);
static i32 set_many_sf(CSOUND *cs, void** inargs, ui32 numargs, HANDLE *handle);
static i32 set_many_is(CSOUND *cs, void** inargs, ui32 numargs, HANDLE *handle);
static i32 set_many_if(CSOUND *cs, void** inargs, ui32 numargs, HANDLE *handle);
static i32 set_many_sa(CSOUND *cs, void** inargs, ui32 numargs, HANDLE *handle);
static inline void _set_ss(CSOUND *csound, khash_t(khStrStr) *h, const char*key, char *val);
static inline void _set_sf(CSOUND *csound, khash_t(khStrFlt) *h, const char*key, MYFLT val);
static void _set_sa_s(CSOUND *csound, HANDLE *handle, char *key, char *value);
static void _set_sa_f(CSOUND *csound, HANDLE *handle, char *key, MYFLT value);



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
    g->slots = csound->Calloc(csound, sizeof(ui32)*g->maxhandles);
    g->maxslots = g->maxhandles;
    g->lastslot = g->maxhandles;
    for(ui32 i=0;i < g->maxslots; i++) {
        g->slots[i] = i;
    }
    g->mutex_ = csound->Create_Mutex(0);
    csound->RegisterResetCallback(csound, (void*)g, (i32(*)(CSOUND*, void*))dict_reset);
    return g;
}

/**
 * get the globals struct
 */

static HASH_GLOBALS *_globals = NULL;

static inline HASH_GLOBALS* dict_globals(CSOUND *csound) {
    if(_globals != NULL)
        return _globals;
    HASH_GLOBALS *g = (HASH_GLOBALS*)csound->QueryGlobalVariable(csound, GLOBALS_NAME);
    if(g == NULL)
        g = create_globals(csound);
    _globals = g;
    return g;
}


static inline void
_init_handle(HANDLE *h) {
    memset((void*)h, 0, sizeof(HANDLE));
}

static i32
dict_expand_pool(HASH_GLOBALS *g) {
    CSOUND *csound = g->csound;
    if(g->lastslot > 0) {
        MSGF("dict_expand_pool: dict is not empty! (dict size: %d)\n", g->lastslot);
        return NOTOK;
    }
    ui32 oldsize = g->maxhandles;
    ui32 newsize = g->maxhandles * 2;
    g->handles = csound->ReAlloc(csound, g->handles, sizeof(HANDLE)*newsize);
    for(size_t i=oldsize; i<newsize; i++) {
        _init_handle(&(g->handles[i]));
    }
    g->slots = csound->ReAlloc(csound, g->slots, sizeof(ui32)*newsize);
    if(g->handles == NULL || g->slots == NULL) {
        MSGF("%s\n", "dict_expand_pool: memory allocation error");
        return NOTOK;
    }
    // fill the pool with new values, from maxhandles to numhandles
    ui32 i = 0;
    for(ui32 slot=oldsize; slot<newsize; slot++) {
        g->slots[i++] = slot;
    }
    g->lastslot = oldsize;
    g->maxslots = newsize;
    g->maxhandles = newsize;
    DBG("Expanded pool. New capacity: %d, size: %d\n", newsize, g->lastslot);
    return OK;
}

static i32
dict_getfreeslot(HASH_GLOBALS *g) {
    CSOUND *csound = g->csound;
    if(g->lastslot == 0) {
        int res = dict_expand_pool(g);
        if(res == NOTOK) {
            csound->Message(csound, "dict_getfreeslot: pool is empty, could not expand it\n");
            return -1;
        }
    }
    ui32 slot = g->slots[g->lastslot-1];
    if(slot > g->maxhandles - 1) {
        MSGF("Internal error: got an index %d from pool, but there are "
             "only %d handles\n", (int)slot, g->maxhandles);
        return -1;
    }
    HANDLE *handle = &(g->handles[slot]);
    if(handle == NULL) {
        MSGF("Internal error: handle is null for idx %d\n", slot);
        return -1;
    }
    if(handle->hashtab != NULL) {
        MSGF("Internal error: the slot %d is signaled as empty, but the "
             "hashtab is not NULL\n", slot);
        return -1;
    }
    g->lastslot -= 1;
    return slot;
}

/**
 * Mark a slot as free
 */
static i32
dict_release_slot(HASH_GLOBALS *g, i32 slot) {
    if(g->lastslot >= g->maxslots) {
        CSOUND *csound = g->csound;
        MSGF("Trying to return slot %d to pool, but pool is full!", slot);
        MSGF("Pool capacity: %d, last element: %d\n", g->maxslots, g->lastslot);
        return NOTOK;
    }
    g->slots[g->lastslot] = slot;
    g->lastslot += 1;
    return OK;
}

/**
 *  Creates a new dict, sets its pointer at a free index and returns this index
 *  Returns OK or NOTOK
 *
 * g: globals as returned by dict_globals
 * khtype: the int signature of this dict
 *
 * a dict of type str:any has two hashtables
 *  hashtab: str:str
 *  hashtab2: str:float
 */
static i32
dict_make_strany(CSOUND *csound, HASH_GLOBALS *g, ui32 idx) {
    khash_t(khStrFlt) *hsf = kh_init(khStrFlt);
    khash_t(khStrStr) *hss = kh_init(khStrStr);
    HANDLE *handle = &(g->handles[idx]);
    handle->hashtab = hss;
    handle->hashtab2 = hsf;
    handle->khtype = khStrAny;
    handle->counter = 0;
    handle->mutex_ = csound->Create_Mutex(0);
    return OK;
}


static i32
dict_make(CSOUND *csound, HASH_GLOBALS *g, int khtype, ui32 initialsize) {
    i32 idx = dict_getfreeslot(g);
    DBG("Creating dict with index %d\n", idx);
    if(idx < 0) {
        MSGF("%s\n", "Couldn't get a free slot");
        return -1;
    }
    void *hashtab = NULL;
    if(khtype == khStrAny) {
        // initialsize is not taken into account
        int res = dict_make_strany(csound, g, idx);
        if(res == NOTOK) {
            MSGF("Error when calling dict_make_strany (idx: %d)\n", idx);
            return NOTOK;
        }
        return idx;
    }

    switch(khtype) {
    case khStrFlt:
        hashtab = (void *)kh_init(khStrFlt);
        if(initialsize > 4) kh_resize(khStrFlt, hashtab, initialsize);
        break;
    case khStrStr:
        hashtab = (void *)kh_init(khStrStr);
        if(initialsize > 4) kh_resize(khStrStr, hashtab, initialsize);
        break;
    case khIntStr:
        hashtab = (void *)kh_init(khIntStr);
        if(initialsize > 4) kh_resize(khIntStr, hashtab, initialsize);
        break;
    case khIntFlt:
        hashtab = (void *)kh_init(khIntFlt);
        if(initialsize > 4) kh_resize(khIntFlt, hashtab, initialsize);
        break;
    }
    HANDLE *handle = &(g->handles[idx]);
    handle->hashtab = hashtab;
    handle->hashtab2 = NULL;
    handle->khtype = khtype;
    handle->counter = 0;
    handle->mutex_ = csound->Create_Mutex(0);
    return idx;
}


/*
static i32
dict_copy(CSOUND *csound, HASH_GLOBALS *g, ui32 index) {
    // TODO!
    i32 newidx = dict_getfreeslot(g);
    if(newidx < 0) return -1;
    return (ui32)newidx;
}
*/


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
    csound->Free(csound, g->slots);
    csound->DestroyGlobalVariable(csound, GLOBALS_NAME);
    _globals = NULL;
    return OK;
}

static i32
_hashtable_free_sf(CSOUND *csound, void *hashtab) {
    khash_t(khStrFlt) *h = hashtab;
    khint_t k;
    for (k = 0; k < kh_end(h); ++k) {
        if (kh_exist(h, k))
            csound->Free(csound, kh_key(h, k));
    }
    kh_destroy(khStrFlt, h);
    return OK;
}

static i32
_hashtable_free_ss(CSOUND *csound, void *hashtab) {
    khash_t(khStrStr) *h = hashtab;
    kstring_t *ks;
    for (khint_t k = 0; k < kh_end(h); ++k) {
        if (kh_exist(h, k)) {
            ks = &(kh_val(h, k));
            if(ks->s != NULL)
                csound->Free(csound, ks->s);
            csound->Free(csound, kh_key(h, k));
        }
    }
    kh_destroy(khStrStr, h);
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
    if(handle->hashtab == NULL) {
        MSGF("dict_free: trying to free handle (idx: %d), but its hashtable is NULL!\n", idx);
        return NOTOK;
    }
    int khtype = handle->khtype;
    DBG("dict: freeing idx=%d, type=%d\n", idx, khtype);
    if(khtype == khStrFlt) {
        _hashtable_free_sf(csound, handle->hashtab);
        /*
        khash_t(khStrFlt) *h = handle->hashtab;
        // we need to free all keys
        for (k = 0; k < kh_end(h); ++k) {
            if (kh_exist(h, k))
                csound->Free(csound, kh_key(h, k));
        }
        kh_destroy(khStrFlt, h);
        */
    } else if (khtype == khStrStr) {
        _hashtable_free_ss(csound, handle->hashtab);
        /*
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
        */
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
    } else if(khtype == khStrAny) {
        if(handle->hashtab == NULL) {
            MSGF("Internal error: hashtab is NULL (idx: %d)\n", (i32)idx);
        } else
            _hashtable_free_ss(csound, handle->hashtab);
        if(handle->hashtab2 == NULL) {
            MSGF("Internal error: hashtab2 is NULL (idx: %d)\n", (i32)idx);
        } else
            _hashtable_free_sf(csound, handle->hashtab2);
        // TODO
    } else {
        MSGF("dict_free: dict type unknown: %d\n", khtype);
        return NOTOK;
    }
    handle->hashtab = NULL;
    handle->hashtab2 = NULL;
    handle->counter = 0;
    handle->khtype = 0;
    dict_release_slot(g, idx);
    return OK;
}

/**
 * function called when deiniting a dict (for example, when a local dict
 * was created and the note it belongs to is released)
 */

/*
static i32
dict_deinit_callback(CSOUND *csound, DICT_NEW *p) {
    i32 idx = p->idx;
    DBG("deinit callback idx: %d", idx);
    HASH_GLOBALS *g = dict_globals(csound);
    if(idx < 0 || idx >= g->maxhandles) {
        MSGF("dict deinit error: index out of range (idx:%d, max handles: %d)\n",
             idx, g->maxhandles);
        return NOTOK;
    }
    if(g->handles[idx].hashtab == NULL) {
        MSGF("dict(idx=%d) already freed\n", idx);
        return OK;
    }
    // We need to free the captured copy of the opcode
    csound->Free(csound, p);
    return _dict_free(csound, g, idx);

}
*/


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
intdef_to_strdef(i32 intdef) {
    switch(intdef) {
    case khIntFlt:
        return "if";
    case khIntStr:
        return "is";
    case khStrFlt:
        return "sf";
    case khStrStr:
        return "ss";
    case khStrAny:
        return "sa";
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
    case 'a':
        return 3;
    default:
        return -1; // signal an error
    }
}


/**
 * convert a str definition of a dict type to its numeric representation
 * returns -1 in case of error
 * We accept two formats: a 2 letter signature and a long signature
 */
static i32
strdef_to_intdef(STRINGDAT *s) {
    size_t l = strlen(s->data);
    char *sdata = (char *)s->data;
    // skip global identifier, if present
    if (strncmp(sdata, "*", 1)==0) {
        sdata = &(sdata[1]);
        l = l -1;
    }
    if(l == 2) {
        if(!strcmp("ss", sdata)) return khStrStr;
        else if(!strcmp("sf", sdata)) return khStrFlt;
        else if(!strcmp("is", sdata)) return khIntStr;
        else if(!strcmp("if", sdata)) return khIntFlt;
        else if(!strcmp("sa", sdata)) return khStrAny;
        else return -1;
    } else if (!strncmp("str:", sdata, 4)) {
        sdata = &(sdata[4]);
        if(!strncmp("str", sdata, 3))   return khStrStr;
        if(!strncmp("float", sdata, 5)) return khStrFlt;
        if(!strncmp("any", sdata, 3))   return khStrAny;
        return -1;
    } else if (!strncmp("int:", sdata, 4)) {
        sdata = &(sdata[4]);
        if(!strncmp("str", sdata, 3))   return khIntStr;
        if(!strncmp("float", sdata, 5)) return khIntFlt;
        return -1;
    } else {
        return -1;
    }
}

/**
 * dict_new opcode, works at i-time only
 *
 * idict  dict_new Stype, [key, value, key, value, ...]
 * idict  dict_new Stype, icapacity=0
 * 
 * Stype: the type of the dict
 *   sf or str:float   str -> float
 *   ss or str:str     str -> str
 *   if or int:float   int -> float
 *   is or int:str     int -> str
 * 
 */


static i32
dict_new(CSOUND *csound, DICT_NEW *p) {
    p->g = dict_globals(csound);
    // All dicts are global now
    p->global = 1;
    i32 khtype = strdef_to_intdef(p->keyvaltype);
    if(khtype < 0)
        return INITERR(Str("dict: type not understood."
                           " Expected one of 'str:float', 'str:str', 'int:float', "
                           "'int:str', 'str:any'"));
    int capacity = DICT_INITIAL_SIZE;
    if(_GetInputArgCnt(csound, p) == 2) {
        // called as idict dict_new "sf", icapacity
        capacity = (int)*(MYFLT*)p->inargs[0];
    }
    i32 idx = dict_make(csound, p->g, khtype, capacity);
    if(idx < 0)
        return INITERR(Str("dict_new: failed to create a new dict"));
    p->idx = idx;
    *p->out_handleidx = (MYFLT)idx;
    p->khtype = khtype;
    return OK;
}


typedef struct {
    OPDS h;
    MYFLT *idx;
    STRINGDAT *str;
} DICT_LOADSTR;


i32 _strcount(const char *s, char c) {
    i32 i;
    for (i=0; s[i]; s[i]==c ? i++ : *s++);
    return i;
}

i64 stridx(const char *s, char c, size_t start) {
    char *subs = strchr(&s[start], c);
    return subs == NULL ? -1 : (subs - s);
}

i64 str_first_char_after(const char *s, size_t slen, size_t start) {
    for(i64 i=start; i<slen; i++) {
        char c = s[i];
        if(c!=' ' && c!='\t' && c!='\n') {
            return i;
        }
    }
    // am empty string
    return -1;
}

i64 str_last_char_before(const char *s, size_t slen, size_t end) {
    for(i64 i=end-1; i >= 0; i--) {
        char c = s[i];
        if(c!=' ' && c!='\t' && c!='\n') {
            return i;
        }
    }
    // am empty string
    return -1;
}

// idict dict_loadstr "foo: 10, bar: 'barvalue'"
static i32
dict_loadstr(CSOUND *csound, DICT_LOADSTR *p) {
    i32 numkeys = _strcount(p->str->data, ',');
    HASH_GLOBALS *g = dict_globals(csound);
    i32 idx = dict_make(csound, g, khStrAny, numkeys);
    if(idx < 0) {
        return INITERRF("Failed to create a dict handle for str %s", p->str->data);
    }
    *p->idx = idx;
    HANDLE *handle = &(g->handles[idx]);
    CHECK_HANDLE(handle);

    // now parse the str and set the pairs
    int insidestr = 0;
    const char *s = p->str->data;
    size_t itemstart = 0;
    size_t itemend = 0;
    size_t starts[100];
    size_t ends[100];
    size_t numitems = 0;
    char skey[80];
    char valuebuf[1000];
    char *svalue = valuebuf;
    size_t slen = strlen(s);
    char quotetype = ' ';
    for(size_t i=0; i<slen; i++) {
        char c = s[i];
        if(insidestr && (c == '\'' || c == '"') && c == quotetype) {
            insidestr = 0;
        } else if(c == ',') {
            itemend = i;
            starts[numitems] = itemstart;
            ends[numitems] = itemend;
            numitems++;
            itemstart = i+1;
        } else if(c=='\'') {
            insidestr = 1;
            quotetype = '\'';
        } else if(c=='"') {
            insidestr = 1;
            quotetype = '"';
        }
    }
    // check last item
    if(itemstart > itemend && itemstart < slen) {
        starts[numitems] = itemstart;
        ends[numitems] = slen;
        numitems++;
    }
    for(size_t i=0; i<numitems; i++) {
        size_t start = starts[i];
        size_t sepidx = stridx(s, ':', start);
        if(sepidx < 0) {
            return INITERRF("Item idx %lu in %s does not have separator :", i, s);
        }

        size_t keystart = str_first_char_after(s, slen, start);
        size_t keyend = str_last_char_before(s, slen, sepidx);
        if(keystart < 0 || keyend < 0) {
            return INITERRF("Malformed key %lu (%s)", i, s);
        }

        strncpy0(skey, &s[keystart], keyend - keystart + 1);
        size_t valuestart = str_first_char_after(s, slen, sepidx+1);
        size_t valueend = str_last_char_before(s, slen, ends[i]);
        if(valuestart < 0 || valueend < 0) {
            return INITERRF("Malformed value %lu (%s)", i, s);
        }
        size_t lenvalue = valueend - valuestart + 1;
        strncpy0(valuebuf, &s[valuestart], lenvalue);
        int is_string_value = 0;
        if(valuebuf[0] == '\'' || valuebuf[0] == '"') {
            if(valuebuf[lenvalue-1] != valuebuf[0])
                return INITERRF("Malformed string value %s", valuebuf);
            is_string_value = 1;
            svalue[lenvalue-1] = '\0';
            svalue++;
        } else if(isalpha(valuebuf[0])) {
            is_string_value = 1;
        }

        if(is_string_value) {
            _set_sa_s(csound, handle, skey, svalue);
            svalue = valuebuf;
        } else {
            _set_sa_f(csound, handle, skey, strtod(valuebuf, NULL));
        }
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
        // cstype = csound->GetTypeForArg(args[i]);
        cstype = _GetTypeForArg(csound, args[i]);
        char c0 = cstype->varTypeName[0];
        // cstype = csound->GetTypeForArg(args[i+1]);
        cstype = _GetTypeForArg(csound, args[i+1]);
        char c1 = cstype->varTypeName[0];
        pairtype = type_char_to_int(c0) * 10 + type_char_to_int(c1);
        if(pairtype < 0)
            return INITERRF("problem parsing arguments, expected pairs of type %s",
                            intdef_to_strdef(khtype));
        if(khtype == khStrAny) {
            if(c0 != 'S') {
                return INITERRF("type mismatch, expected a str key, got %c", c0);
                // TODO : error
            }
        } else if(pairtype != khtype) {
            return INITERRF("type mismatch, expected pairs of type %s, but got a"
                            "pair with type %s",
                            intdef_to_strdef(khtype),
                            intdef_to_strdef(pairtype));
        }
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
    if (ret == NOTOK) {
        MSGF("%s\n", "Error when creating new dictionary");
        return NOTOK;
    }

    // our signature is: idict dict_new Stype, args passed to opcode
    // So the number of args to be passed to opcode is our num. args - 1
    ui32 numargs = p->INOCOUNT - 1;

    i32 idx = (i32)(*p->out_handleidx);

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
    case khStrAny:
        return set_many_sa(csound, p->inargs, numargs, handle);
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
            LOCK(handle);
            kh_key(h, k) = key;
            UNLOCK(handle);
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

static i32 dict_set_sf_any(CSOUND *csound, DICT_SET_sf *p, HANDLE *handle);
static i32 dict_set_sf_(CSOUND *csound, DICT_SET_sf *p, HANDLE *handle, khash_t(khStrFlt) *h);


// perf func for dict_set str->float
static i32
dict_set_sf(CSOUND *csound, DICT_SET_sf *p) {
    HANDLE *handle = get_handle(p);
    CHECK_HANDLE(handle);
    if(handle->khtype == khStrAny)
        return dict_set_sf_any(csound, p, handle);
    CHECK_HASHTAB_TYPE(handle->khtype, khStrFlt);
    khash_t(khStrFlt) *h = handle->hashtab;
    return dict_set_sf_(csound, p, handle, h);
}

static i32
dict_set_sf_any(CSOUND *csound, DICT_SET_sf *p, HANDLE *handle) {
    khash_t(khStrFlt) *h = handle->hashtab2;
    return dict_set_sf_(csound, p, handle, h);
}


// perf func for dict_set str->float
static i32
dict_set_sf_(CSOUND *csound, DICT_SET_sf *p, HANDLE *handle, khash_t(khStrFlt) *h) {
    int absent;
    khiter_t k;
    char *key;
    // test fastpath
    if(p->counter == handle->counter &&
       p->lastkey_size > 0 &&
       !strcmp(p->lastkey_data, p->outkey->data)) {
        kh_value(h, p->lastidx) = *p->outval;
        return OK;
    }
    CHECK_KEY_SIZE(p->outkey);
    p->lastidx = k = kh_put(khStrFlt, h, p->outkey->data, &absent);
    if (absent) {
        key = csound->Strdup(csound, p->outkey->data);
        LOCK(handle);
        kh_key(h, k) = key;
        UNLOCK(handle);
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
    // in
    MYFLT *handleidx;
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
    CHECK_HASHTAB_TYPE2(handle->khtype, khStrStr, khStrAny);

    // fastpath: dict was not changed and this key is unchanged, last index is valid
    if(p->counter == handle->counter &&
       // p->outkey->size == p->lastkey_size &&
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

static i32
dict_set_is_i(CSOUND *csound, DICT_SET_is *p) {
    int err = dict_set_is_0(csound, p);
    if(err==NOTOK)
        return NOTOK;
    return dict_set_is(csound, p);
}

// -----------------------------------------------------
//                      DEL
// -----------------------------------------------------

/*
 * This removes a key:value pair from a dict. It is called
 * as dict_set without a value
 *
 * called as dict_del idict, Skey removes the key at the end
 * of the note
 *
 */

typedef struct {
    OPDS h;
    MYFLT *handleidx;
    STRINGDAT *outkey;
} DICT_DEL_s;

static i32
_hashtab_del_ss(CSOUND *csound, khash_t(khStrStr) *h, STRINGDAT *key) {
    khiter_t k = kh_get(khStrStr, h, key->data);
    if(k == kh_end(h))
        return 0;
    LOCK(handle);
    csound->Free(csound, kh_key(h, k));
    kstring_t *ks = &(h->vals[k]);
    csound->Free(csound, ks->s);
    kh_del(khStrStr, h, k);
    UNLOCK(handle);
    return 1;
}

static i32
_hashtab_del_sf(CSOUND *csound, khash_t(khStrFlt) *h, STRINGDAT *key) {
    khiter_t k = kh_get(khStrFlt, h, key->data);
    if(k == kh_end(h))
        return 0;
    LOCK(handle);
    csound->Free(csound, kh_key(h, k));
    kh_del(khStrFlt, h, k);
    UNLOCK(handle);
    return 1;
}

static i32
dict_del_s(CSOUND *csound, DICT_DEL_s *p) {
    HASH_GLOBALS *g = dict_globals(csound);
    i32 idx = (i32)*p->handleidx;
    HANDLE *handle = &(g->handles[idx]);
    i32 khtype = handle->khtype;
    int found = 0;
    if(khtype == khStrFlt) {
        khash_t(khStrFlt) *h = handle->hashtab;
        CHECK_HASHTAB_EXISTS(h);
        found = _hashtab_del_sf(csound, h, p->outkey);
    } else if(khtype == khStrStr) {
        khash_t(khStrStr) *h = handle->hashtab;
        CHECK_HASHTAB_EXISTS(h);
        found = _hashtab_del_ss(csound, h, p->outkey);
    } else if(khtype == khStrAny) {
        found = _hashtab_del_sf(csound, handle->hashtab2, p->outkey);
        if(!found)
            found = _hashtab_del_ss(csound, handle->hashtab, p->outkey);
    }
    if(found)
        handle->counter++;
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
                        intdef_to_strdef(khtype));
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
    p->lastkey = UI32MAX;
    p->counter = 0;
    return OK;
}

static i32
dict_get_if(CSOUND *csound, DICT_GET_if *p) {
    HASH_GLOBALS *g = p->g;
    i32 idx = (i32)*p->handleidx;
    HANDLE *handle = &(g->handles[idx]);
    if(UNLIKELY(handle->hashtab == NULL)) {
        *p->kout = *p->defaultval;
        return OK;
    }
    CHECK_HASHTAB_TYPE(handle->khtype, khIntFlt);
    khash_t(khIntFlt) *h = handle->hashtab;
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
    MYFLT *failnodict;
    // internal
    HASH_GLOBALS *g;
    khiter_t lastidx;
    ui32 _handleidx;
    i32 lastkey_size;
    ui64 counter;
    char lastkey_data[KHASH_STRKEY_MAXSIZE+1];
    HANDLE *handle;
    int constant_key;
} DICT_GET_sf;


static i32
dict_get_sf_0(CSOUND *csound, DICT_GET_sf *p) {
    p->g = dict_globals(csound);
    p->lastkey_size = 0;
    p->lastidx = 0;
    p->counter = 0;
    p->_handleidx = (ui32)*p->handleidx;
    p->handle = NULL;
    p->constant_key = 0;
    // check if the argument NAME is a constant
    char *key = p->h.optext->t.inlist->arg[1];
    if(key != NULL && key[0] == '"') {
        p->constant_key = 1;
    }
    return OK;
}


static inline i32
dict_get_sf_(CSOUND *csound, DICT_GET_sf *p, HANDLE *handle, khash_t(khStrFlt) *h) {
    if(p->outkey->size == 0) {
        return PERFERR("dict_get: not valid key (size=0)");
    }
    if(p->counter == handle->counter &&
       (p->constant_key || !strncmp(p->outkey->data, p->lastkey_data, KHASH_STRKEY_MAXSIZE))) {
        // fast path
        khiter_t k = p->lastidx;
        *p->kout = k != kh_end(h) ? kh_val(h, k) : *p->defaultval;
        return OK;
    }
    // slow path
    CHECK_KEY_SIZE(p->outkey);
    khiter_t k = kh_get(khStrFlt, h, p->outkey->data);
    if(k != kh_end(h)) {
        // key found
        p->lastidx = k;
        p->lastkey_size = p->outkey->size;  // save size, mark key as valid
        strncpy0(p->lastkey_data, p->outkey->data, (size_t)(p->outkey->size-1));
        *p->kout = kh_val(h, k);
    } else {
        *p->kout = *p->defaultval;  // return default value
        p->lastkey_size = 0;        // mark last key as not usable
    }
    p->counter = handle->counter;
    return OK;
}


static i32 dict_get_sf_any(CSOUND *csound, DICT_GET_sf *p, HANDLE *handle);

static i32
dict_get_sf(CSOUND *csound, DICT_GET_sf *p) {
    // use cached idx, not really necessary since we declared ihandle as i-var
    HANDLE *handle = &(p->g->handles[p->_handleidx]);
    if(UNLIKELY(handle->hashtab == NULL)) {
        *p->kout = *p->defaultval;
        return OK;
    }
    if(handle->khtype == khStrAny) {
        return dict_get_sf_any(csound, p, handle);
    }
    CHECK_HASHTAB_TYPE(handle->khtype, khStrFlt);
    khash_t(khStrFlt) *h = handle->hashtab;
    return dict_get_sf_(csound, p, handle, h);
}


static i32
dict_get_sf_any(CSOUND *csound, DICT_GET_sf *p, HANDLE *handle) {
    khash_t(khStrFlt) *h = handle->hashtab2;
    return dict_get_sf_(csound, p, handle, h);
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
dict_get_ss(CSOUND *csound, DICT_GET_ss *p) {
    // if the key is not found, an empty string is returned
    HASH_GLOBALS *g = p->g;
    i32 idx = (i32)*p->handleidx;
    HANDLE *handle = &(g->handles[idx]);
    if(handle->hashtab == NULL) {
        p->outstr->data[0] = '\0';
        return OK;
    }
    CHECK_HASHTAB_TYPE2(handle->khtype, khStrStr, khStrAny);
    khash_t(khStrStr) *h = handle->hashtab;
    kstring_t *ks;
    khiter_t k;

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


static i32
dict_get_ss_i(CSOUND *csound, DICT_GET_ss *p) {
    HASH_GLOBALS *g = dict_globals(csound);
    i32 idx = (i32)*p->handleidx;
    HANDLE *handle = &(g->handles[idx]);
    if(handle->hashtab == NULL) {
        p->outstr->data[0] = '\0';
        return OK;
    }
    CHECK_HASHTAB_TYPE2(handle->khtype, khStrStr, khStrAny);
    khash_t(khStrStr) *h = handle->hashtab;
    kstring_t *ks;
    khiter_t k;

    CHECK_KEY_SIZE(p->outkey);
    k = kh_get(khStrStr, h, p->outkey->data);
    if(k == kh_end(h)) {
        // key not found, set out to empty string
        p->outstr->data[0] = '\0';
        return OK;
    }
    // key found
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
    return OK;
}

static i32
hashtab_get_is(CSOUND *csound, DICT_GET_is *p) {
    HANDLE *handle = get_handle(p);
    if(UNLIKELY(handle==NULL)) {
        p->outstr->data[0] = '\0';
        return OK;
    }
    CHECK_HASHTAB_TYPE(handle->khtype, khIntStr);
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

static i32
hashtab_get_is_i(CSOUND *csound, DICT_GET_is *p) {
    int err = hashtab_get_is_0(csound, p);
    if(err==NOTOK)
        return err;
    return hashtab_get_is(csound, p);
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
    if((i32)*p->iwhen == 0) {
        return _dict_free(csound, g, idx);
    }
#ifdef CSOUNDAPI6
    // free at the end of the note
    register_deinit(csound, p, dict_free_callback);
#endif
    return OK;
}

// -----------------------------------
//            CLEAR
// -----------------------------------

typedef struct {
    OPDS h;
    MYFLT *handleidx;
    HASH_GLOBALS *g;
} DICT_CLEAR;

static i32 _dict_clear(CSOUND *csound, HANDLE *handle, HASH_GLOBALS *g) {
    int khtype = handle->khtype;
    kstring_t *ks;
    khint_t k;
    if(khtype == khStrFlt) {
        khash_t(khStrFlt) *h = handle->hashtab;
        // we need to free all keys
        for (k = 0; k < kh_end(h); ++k) {
            if (kh_exist(h, k)) {
                csound->Free(csound, kh_key(h, k));
            }
        }
        kh_clear(khStrFlt, h);
    }
    else if (khtype == khStrStr) {
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
        kh_clear(khStrStr, h);
    }
    else if (khtype == khIntFlt) {
        khash_t(khIntFlt) *h = handle->hashtab;
        kh_clear(khIntFlt, h);
    }
    else if (khtype == khIntStr) {
        khash_t(khIntStr) *h = handle->hashtab;
        // we need to free all values
        for (k = 0; k < kh_end(h); ++k) {
            if (kh_exist(h, k)) {
                ks = &(kh_val(h, k));
                csound->Free(csound, ks->s);
            }
        }
        kh_clear(khIntStr, h);
    }
    else {
        return NOTOK;
    }
    return OK;
}

static i32 dict_clear_i(CSOUND *csound, DICT_CLEAR *p) {
    HASH_GLOBALS *g = dict_globals(csound);
    HANDLE *handle = &(g->handles[(int)*p->handleidx]);
    return _dict_clear(csound, handle, g);
}

static i32 dict_clear_init(CSOUND *csound, DICT_CLEAR *p) {
    HASH_GLOBALS *g = dict_globals(csound);
    p->g = g;
    return OK;
}

static i32 dict_clear_perf(CSOUND *csound, DICT_CLEAR *p) {
    HANDLE *handle = &(p->g->handles[(int)*p->handleidx]);
    return _dict_clear(csound, handle, p->g);
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


#define DICT_PRINT_LINELENGTH 80

void print_hashtab_ss(CSOUND *csound, khash_t(khStrStr) *h) {
    i32 chars = 0;
    const i32 linelength = DICT_PRINT_LINELENGTH;
    char line[256];
    for(khint_t k = kh_begin(h); k != kh_end(h); ++k) {
        if(!kh_exist(h, k)) continue;
        chars += sprintf(line+chars, "%s: \"%s\"", kh_key(h, k), kh_val(h, k).s);
        if(chars < linelength) {
            line[chars++] = ',';
            line[chars++] = ' ';
        } else {
            line[chars+1] = '\0';
            csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)line);
            chars = 0;
        }
    }
    if(chars > 0) {    // last line
        line[chars] = '\0';
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)line);
    }

}

static void
print_hashtab_sf(CSOUND *csound, khash_t(khStrFlt) *h) {
    i32 chars = 0;
    const i32 linelength = DICT_PRINT_LINELENGTH;
    char line[256];
    for(khint_t k = kh_begin(h); k != kh_end(h); ++k) {
        if(!kh_exist(h, k)) continue;
        chars += sprintf(line+chars, "%s: "FLOAT_FMT, kh_key(h, k), kh_val(h, k));
        if(chars < linelength) {
            line[chars++] = ',';
            line[chars++] = ' ';
        } else {
            line[chars+1] = '\0';
            csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)line);
            chars = 0;
        }
    }
    // last line
    if(chars > 0) {    // last line
        line[chars] = '\0';
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)line);
    }
}

static i32
_dict_print(CSOUND *csound, DICT_PRINT *p, HANDLE *handle) {
    int khtype = handle->khtype;
    khint_t k;
    i32 chars = 0;
    // linelength could be changed dynamically in the future
    const i32 linelength = 80;
    char line[256];
    int itemlength;

    // one item should be 000: 45.12345
    //                    1234567890123
    int item_maxlength = 15;
    csound->MessageS(csound, CSOUNDMSG_ORCH, "{\n");
    memset(line, ' ', 4);
    chars = 4;
    if(khtype == khIntFlt) {
        khash_t(khIntFlt) *h = handle->hashtab;
        for(k = kh_begin(h); k != kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;
            itemlength = sprintf(line+chars, "%d: "FLOAT_FMT, kh_key(h, k), kh_value(h, k));
            chars += itemlength;
            if(chars + (item_maxlength - itemlength) <= linelength) {
                for(int i=itemlength; i<item_maxlength; i++) {
                    line[chars++] = ' ';
                }
            } else {
                line[chars+1] = '\0';
                csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)line);
                memset(line, ' ', 4);
                chars = 4;
            }
        }
        // last line
        if(chars > 0) {
            line[chars] = '\0';
            csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)line);
        }
    } else if(khtype == khIntStr) {
        khash_t(khIntStr) *h = handle->hashtab;
        for(k = kh_begin(h); k != kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;
            chars += sprintf(line+chars, "%d: %s", kh_key(h, k), kh_val(h, k).s);
            if(chars < linelength) {
                line[chars++] = ',';
                line[chars++] = ' ';
            } else {
                line[chars+1] = '\0';
                csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)line);
                chars = 0;
            }
        }
        // last line
        if(chars > 0) {
            line[chars] = '\0';
            csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)line);
        }
    } else if(khtype == khStrFlt) {
        print_hashtab_sf(csound, handle->hashtab);
    } else if(khtype == khStrStr) {
        print_hashtab_ss(csound, handle->hashtab);
    } else if(khtype == khStrAny) {
        print_hashtab_ss(csound, handle->hashtab);
        print_hashtab_sf(csound, handle->hashtab2);
    } else {
        char *fmt = intdef_to_strdef(khtype);
        csound->ErrorMsg(csound, Str("dict format not supported: %d (%s)"), khtype, fmt);
        return NOTOK;
    }
    csound->MessageS(csound, CSOUNDMSG_ORCH, "}\n");
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
    MYFLT *outstr;

    MYFLT *handleidx;
    STRINGDAT *cmdstr;

    HASH_GLOBALS *g;
    i32 cmd;
} DICT_QUERY1;


static i32 dict_exists(CSOUND *csound, DICT_QUERY1 *p) {
    p->g = dict_globals(csound);
    int idx = (int)*p->handleidx;
    if(idx < 0 || idx >= p->g->maxhandles) {
        *p->outstr = 0;
        return OK;
    }
    HANDLE *handle = get_handle(p);
    *p->outstr = handle != NULL || handle->hashtab != NULL;
    return OK;
}

static i32 dict_size(CSOUND *csound, DICT_QUERY1 *p);

static i32 dict_query(CSOUND *csound, DICT_QUERY1 *p) {
    HANDLE *handle;
    switch(p->cmd) {
    case 0:  // size
        return dict_size(csound, p);
    case 1:  // type
        handle = get_handle_check(p);
        *p->outstr = (handle == NULL || handle->hashtab == NULL) ? 0 : handle->khtype;
        return OK;
    case 2:  // exists
        handle = get_handle_check(p);
        *p->outstr = (handle == NULL || handle->hashtab == NULL) ? 0 : 1;
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
    if(handle == NULL || handle->hashtab == NULL) {
        *p->outstr = -1;
        return OK;
    }
    with_hashtable(handle, {
        *p->outstr = kh_size(h);
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
    ARRAYDAT *outstr;

    MYFLT *handleidx;
    STRINGDAT *cmdstr;

    HASH_GLOBALS *g;
    i32 cmd;
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
    ARRAYCHECK(p->outstr, (i32) size);

    switch(p->cmd) {
    case 0:  // keys, string
        return dict_query_arr_keys_s(csound, handle, p->outstr);
    case 1:  // keys, numeric
        return dict_query_arr_keys_i(csound, handle, p->outstr);
    case 2:  // values, string
        return dict_query_arr_values_s(csound, handle, p->outstr);
    case 3:  // values, numeric
        return dict_query_arr_values_f(csound, handle, p->outstr);
    default:
        return PERFERRF("internal error: invalid cmd (%d)", p->cmd);
    }
}


// init func for dict_query when expecting an output array
static i32
dict_query_arr_0(CSOUND *csound, DICT_QUERY_ARR *p) {
    p->g = dict_globals(csound);
    char *data = p->cmdstr->data;
    HANDLE *handle = get_handle(p);
    char *vartypename = p->outstr->arrayType->varTypeName;
    ui32 size = handle_get_hashtable_size(handle);
    // tabinit(csound, p->outstr, (i32)size);
    TABINIT(csound, p->outstr, (i32)size);
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
    MYFLT *outkidx;
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
    char *dictsig = intdef_to_strdef(handle->khtype);
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
            *p->outkidx = p->numyields;
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
            *p->outkidx = p->numyields;
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
            *p->outkidx = p->numyields;
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
            *p->outkidx = p->numyields;
            p->numyields++;
            return OK;
        }
    } else
        return PERFERRF("dict: type %d not supported", khtype);
    // finished iteration! signal stop and reset if autoreset (kreset == -1)
    *p->outkidx = -1;
    if (kreset == 2) {  // reset at the end of iteration
        p->nextk = 0;
        p->numyields = 0;
    }
    return OK;
}


static inline void
_set_ss(CSOUND *csound, khash_t(khStrStr) *h, const char*key, char *val) {
    int absent;
    khiter_t k = kh_put(khStrStr, h, key, &absent);
    kstring_t *ks = &(h->vals[k]);
    if(absent) {
        kh_key(h, k) = csound->Strdup(csound, key);
        kstr_from_cstr(csound, ks, val);
    } else {
        kstr_setn(csound, ks, val, strlen(val));
    }
}


static i32
set_many_ss(CSOUND *csound, void** inargs, ui32 numargs, HANDLE *handle) {
    khash_t(khStrStr) *h = handle->hashtab;
    for(ui32 argidx=0; argidx < numargs; argidx+=2) {
        STRINGDAT *key = inargs[argidx];
        STRINGDAT *val = inargs[argidx+1];
        _set_ss(csound, h, key->data, val->data);
    }
    handle->counter++;
    return OK;
}

static inline void
_set_sf(CSOUND *csound, khash_t(khStrFlt) *h, const char *key, MYFLT val) {
    int absent;
    khiter_t k = kh_put(khStrFlt, h, key, &absent);
    if(absent) {
        kh_key(h, k) = csound->Strdup(csound, key);
    }
    kh_value(h, k) = val;
}

static i32
set_many_sf(CSOUND *csound, void** inargs, ui32 numargs, HANDLE *handle) {
    khash_t(khStrFlt) *h = handle->hashtab;
    for(ui32 argidx=0; argidx < numargs; argidx+=2) {
        STRINGDAT *key = (STRINGDAT *)inargs[argidx];
        // MYFLT val = *((MYFLT*)(inargs[argidx+1]));
        MYFLT val = *(MYFLT*)inargs[argidx+1];
        _set_sf(csound, h, key->data, val);
    }
    handle->counter++;
    return OK;
}


void _set_sa_f(CSOUND *csound, HANDLE *handle, char *key, double value) {
    // for a dict of type str:any, set a str:float pair
    khash_t(khStrFlt) *h2 = handle->hashtab2;
    _set_sf(csound, h2, key, value);
}

void _set_sa_s(CSOUND *csound, HANDLE *handle, char *key, char *value) {
    // for a dict of type str:any, set a str:float pair
    khash_t(khStrStr) *h = handle->hashtab;
    _set_ss(csound, h, key, value);
}


static i32
set_many_sa(CSOUND *csound, void**inargs, ui32 numargs, HANDLE *handle) {
    khash_t(khStrStr) *h1 = handle->hashtab;
    khash_t(khStrFlt) *h2 = handle->hashtab2;
    STRINGDAT *svalue;
    for(ui32 argidx=0; argidx < numargs; argidx+=2) {
        STRINGDAT *key = (STRINGDAT *)inargs[argidx];
        // CS_TYPE *cstype = csound->GetTypeForArg(inargs[argidx+1]);
        CS_TYPE *cstype = _GetTypeForArg(csound, inargs[argidx+1]);
        char argtype = cstype->varTypeName[0];
        switch(argtype) {
        case 'S':
            svalue = (STRINGDAT *)inargs[argidx+1];
            _set_ss(csound, h1, key->data, svalue->data);
            break;
        case 'i':
        case 'c':   // constant
        case 'k':
            _set_sf(csound, h2, key->data, *(MYFLT*)inargs[argidx+1]);
            break;
        }
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

// Sout dict_dump idict
// This is the complementary operation of dict_loadstr: it generates the definition
// string from a dict
/*
 * keyA: numvalue,\n
 * keyB: 'string value'
 * ...
 */

typedef struct {
    OPDS h;
    STRINGDAT *sout;
    MYFLT *dictidx;
} DICT_DUMP;

static i64 _dict_dump_sf(khash_t(khStrFlt) *h, char *buf, size_t buflen) {
    const char *buforig = buf;
    const char *key;
    ui64 chars = 0;
    if(h->size == 0) {
        buf[0] = '\0';
        return 0;
    }
    for(khint_t k=kh_begin(h); k != kh_end(h); ++k) {
        if(!kh_exist(h, k)) continue;
        key = kh_key(h, k);
        MYFLT val = kh_val(h, k);
        if(buflen < 80) {
            return -1;
        }
        chars = sprintf(buf, "%s: "FLOAT_FMT, key, val);
        buf += chars;
        buflen -= chars;
        *buf++ = ',';
        *buf++ = '\n';
    }
    buf[-2] = '\0';
    i64 outputlen = (buf - buforig) - 2;
    return outputlen;
}

static i64 _dict_dump_ss(khash_t(khStrStr) *h, char *buf, size_t buflen) {
    const char *buforig = buf;
    const char *key;
    kstring_t value;
    ui64 chars = 0;
    if(h->size == 0) {
        buf[0] = '\0';
        return 0;
    }
    for(khint_t k=kh_begin(h); k != kh_end(h); ++k) {
        if(!kh_exist(h, k)) continue;
        key = kh_key(h, k);
        value = kh_val(h, k);
        if(buflen < 80) {
            return -1;
        }
        chars = sprintf(buf, "%s: '%s'", key, value.s);
        buf += chars;
        buflen -= chars;
        *buf++ = ',';
        *buf++ = '\n';
    }
    buf[-2] = '\0';
    i64 outputlen = (buf - buforig) - 2;
    return outputlen;
}

static i64 _dict_dump_sa(HANDLE *handle, char *buf, size_t buflen) {
    i64 dumplen = _dict_dump_sf(handle->hashtab2, buf, buflen);
    if(dumplen < 0) {
        return -1;
    }
    if(dumplen > 0) {
        buf[dumplen] = ',';
        buf[dumplen+1] = '\n';
    }
    i64 dumplen2 = _dict_dump_ss(handle->hashtab, &buf[dumplen+2], buflen-dumplen-2);
    if(dumplen2 < 0)
        return -1;
    return dumplen + dumplen2;
}

static i32 dict_dump(CSOUND *csound, DICT_DUMP *p) {
    HASH_GLOBALS *g = dict_globals(csound);
    ui32 idx = (ui32)*p->dictidx;
    HANDLE *handle = get_handle_by_idx(g, idx);
    if(handle == NULL) {
        return INITERRF("Invalid dict handle %d", idx);
    }
    enum {buflen = 2000};
    char buf[buflen];
    i64 dumplen;
    switch(handle->khtype) {
    case khStrFlt:
        dumplen = _dict_dump_sf(handle->hashtab, buf, buflen);
        break;
    case khStrStr:
        dumplen = _dict_dump_ss(handle->hashtab, buf, buflen);
        break;
    case khStrAny:
        dumplen = _dict_dump_sa(handle, buf, buflen);
        break;
    default:
        return INITERRF("Dict type %s not supported", intdef_to_strdef(handle->khtype));
    }
    if(dumplen < 0) {
        p->sout->data[0] = '\0';
        return INITERR("Eror dumping dict");
    }
    stringdat_set(csound, p->sout, buf, dumplen);
    return OK;
}


// -----------------------------------------------------------------------------------------
//                                       Cache opcodes
// -----------------------------------------------------------------------------------------

typedef struct {
    khash_t(khStrInt) *str2int;
    khash_t(khIntStr) *int2str;
    ui32 counter;
} STRCACHE_GLOBALS;

#define STRCACHE_GLOBALS_NAME "__strcache_globals__"
#define CACHE_MINSIZE 64

static i32 cache_reset(CSOUND *csound, STRCACHE_GLOBALS *g) {
    khash_t(khIntStr) *h1 = g->int2str;
    // we need to free all values
    khiter_t k;
    kstring_t *ks;
    for (k = 0; k < kh_end(h1); ++k) {
        if (kh_exist(h1, k)) {
            ks = &(kh_val(h1, k));
            csound->Free(csound, ks->s);
        }
    }
    kh_destroy(khIntStr, h1);

    // we don't need to free the keys in str2int since they point to the values in i2s
    kh_destroy(khStrInt, g->str2int);
    return OK;
}

static STRCACHE_GLOBALS*
create_cache_globals(CSOUND *csound) {
    int err = csound->CreateGlobalVariable(csound, STRCACHE_GLOBALS_NAME, sizeof(STRCACHE_GLOBALS));
    if (err != 0) {
        INITERR(Str("cachestr: failed to allocate globals"));
        return NULL;
    };
    STRCACHE_GLOBALS *g = (STRCACHE_GLOBALS*)csound->QueryGlobalVariable(csound, STRCACHE_GLOBALS_NAME);
    g->int2str = kh_init(khIntStr);
    g->str2int = kh_init(khStrInt);
    g->counter = 0;
    kh_resize(khIntStr, g->int2str, CACHE_MINSIZE);
    kh_resize(khStrInt, g->str2int, CACHE_MINSIZE);
    csound->RegisterResetCallback(csound, (void*)g, (i32(*)(CSOUND*, void*))cache_reset);
    return g;
}

/**
 * get the cache globals struct
 */
static inline
STRCACHE_GLOBALS* cache_globals(CSOUND *csound) {
    STRCACHE_GLOBALS *g = (STRCACHE_GLOBALS*)csound->QueryGlobalVariable(csound, STRCACHE_GLOBALS_NAME);
    if(LIKELY(g != NULL)) return g;
    g = create_cache_globals(csound);
    return g;
}

static i32
cache_putstr(CSOUND *csound, STRCACHE_GLOBALS* g, STRINGDAT *s, i64 *out) {
    int absent;
    // first check if key is already there
    khiter_t ki2s, ks2i;
    khash_t(khIntStr) *i2s = g->int2str;
    khash_t(khStrInt) *s2i = g->str2int;
    ks2i = kh_get(khStrInt, g->str2int, s->data);
    if(ks2i != kh_end(g->str2int)) {
        // key found
        i64 idx = kh_val(s2i, ks2i);
        // printf("Key present, k=%d, idx=%ld, end=%d\n", ks2i, idx, kh_end(g->str2int));
        *out = idx;
        return OK;
    }
    // key is not present, get a new idx for the str
    ks2i = kh_put(khStrInt, s2i, s->data, &absent);
    ui32 idx = (g->counter++);

    // int2str[idx] = s
    ki2s = kh_put(khIntStr, i2s, idx, &absent);
    if(UNLIKELY(!absent)) {
        return INITERRF("cache: repeated int key, s=%s, idx=%d", s->data, idx);
    }
    kh_key(i2s, ki2s) = idx;
    kstring_t *ks = &(i2s->vals[ki2s]);
    kstr_init_from_stringdat(csound, ks, s);
    // str2int[s] = idx
    // we share the storage for the key in s2i and the value in i2s
    kh_key(s2i, ks2i) = ks->s;
    kh_value(s2i, ks2i) = idx;
    // printf("key not present, k=%d, idx=%d\n", ks2i, idx);
    *out = idx;
    return OK;
}

static kstring_t *
cache_getstr(STRCACHE_GLOBALS *g, ui32 idx) {
    // if the key is not present it should be an error
    khiter_t k = kh_get(khIntStr, g->int2str, idx);
    if(k == kh_end(g->int2str)) {   // key not found
        return NULL;
    }
    kstring_t *ks = &(kh_val(g->int2str, k));
    return ks;
}

// pop returns a c-string and removes the string from the cache
// the receiver OWNS the received string
// Returns NULL if idx not found. strsize is set to the str allocated size
static char *
cache_popstr(STRCACHE_GLOBALS *g, ui32 idx, ui32 *strsize) {
    khiter_t k = kh_get(khIntStr, g->int2str, idx);
    if(k == kh_end(g->int2str)) {   // key not found
        return NULL;
    }
    kstring_t *ks = &(kh_val(g->int2str, k));
    char *s = ks->s;
    *strsize = (ui32) ks->m;
    kh_del(khIntStr, g->int2str, k);
    // del g->str2int[s]
    k = kh_get(khStrInt, g->str2int, s);
    if(k != kh_end(g->str2int)) {
        kh_del(khStrInt, g->str2int, k);
    }
    return s;
}


typedef struct {
    OPDS h;
    // Sstr cacheget kidx / iidx
    STRINGDAT *outstr;
    MYFLT *idx;

    STRCACHE_GLOBALS *g;
    int done;
} CACHEGET;


static i32 cacheget_perf(CSOUND *csound, CACHEGET *p);

static i32
cacheget_i(CSOUND *csound, CACHEGET *p) {
    p->g = cache_globals(csound);
    return cacheget_perf(csound, p);
}

static i32
cacheget_0(CSOUND *csound, CACHEGET *p) {
    p->g = cache_globals(csound);
    return OK;
}

static i32
cacheget_perf(CSOUND *csound, CACHEGET *p) {
    STRCACHE_GLOBALS *g = p->g;
    ui32 idx = (ui32) (*p->idx);
    kstring_t *ks = cache_getstr(g, idx);
    if(ks == NULL)
        return PERFERRF("string not found in cache (idx: %d)", idx);
    return stringdat_set(csound, p->outstr, ks->s, ks->l);
}

static i32
sview_deinit(CSOUND *csound, CACHEGET *p) {
    IGN(csound);
    p->outstr->data = NULL;
    p->outstr->size = 0;
    return OK;
}

static i32
sview_i(CSOUND *csound, CACHEGET *p) {
    STRCACHE_GLOBALS *g = cache_globals(csound);
    ui32 idx = (ui32) (*p->idx);
    kstring_t *ks = cache_getstr(g, idx);
    if(ks == NULL)
        return INITERRF("string not found in cache (idx: %d)", idx);
    stringdat_view(csound, p->outstr, ks->s, ks->m);
#ifdef CSOUNDAPI6
    register_deinit(csound, p, sview_deinit);
#endif
    return OK;
}

static i32
sview_init(CSOUND *csound, CACHEGET *p) {
    STRCACHE_GLOBALS *g = cache_globals(csound);
    p->g = g;
    stringdat_view_init(csound, p->outstr);
#ifdef CSOUNDAPI6
    register_deinit(csound, p, sview_deinit);
#endif
    return OK;
}


static i32
sview_k(CSOUND *csound, CACHEGET *p) {
    ui32 idx = (ui32) (*p->idx);
    kstring_t *ks = cache_getstr(p->g, idx);
    if(ks == NULL)
        return PERFERRF("String not found in cache (idx: %d)", idx);
    // TODO: check that the dest. string (outstr) has not been modified (cache
    // .data nad .size and check that they are the same before modifying them
    // to make sure that we are the only client of this string)
    p->outstr->data = ks->s;
    p->outstr->size = ks->m;
    return OK;
}

typedef struct {
    OPDS h;
    STRINGDAT *s;
    MYFLT *idx;
} STRPEEK;


static i32
strpeek_deinit(CSOUND *csound, STRPEEK *p) {
    IGN(csound);
    p->s->data = NULL;
    p->s->size = 0;
    return OK;
}

#ifdef CSOUNDAPI6
static i32
strpeek_i(CSOUND *csound, STRPEEK *p) {
    int idx = (int)*p->idx;
    if(idx < 0 || idx >= csound->GetStrsmax(csound)) {
        return INITERRF("strpeek: index %d out of bounds", idx);
    }
    char *src = csound->GetStrsets(csound, idx);
    if(src == NULL) {
        return INITERRF("strpeek: index %d not set", idx);
    }
    stringdat_view(csound, p->s, src, strlen(src));
    register_deinit(csound, p, strpeek_deinit);
    return OK;
}
#endif


static i32
cachepop_i(CSOUND *csound, CACHEGET *p) {
    STRCACHE_GLOBALS *g = cache_globals(csound);
    ui32 idx = (ui32) (*p->idx);
    ui32 ssize;
    char *s = cache_popstr(g, idx, &ssize);
    if(s == NULL)
        return INITERRF(Str("cachepop: string with index %d not in cache"), idx);
    stringdat_move(csound, p->outstr, s, ssize);
    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *idx;
    STRINGDAT *s;
    STRCACHE_GLOBALS *g;
} CACHEPUT;

static i32
cacheput_0(CSOUND *csound, CACHEPUT *p) {
    p->g = cache_globals(csound);
    return OK;
}

static i32
cacheput_perf(CSOUND *csound, CACHEPUT *p) {
    STRCACHE_GLOBALS *g = p->g;
    i64 idx = 0;
    i32 ret = cache_putstr(csound, g, p->s, &idx);
    if(ret == NOTOK) return NOTOK;
    *p->idx = (MYFLT) idx;
    return OK;
}

static i32
cacheput_i(CSOUND *csound, CACHEPUT *p) {
    cacheput_0(csound, p);
    return cacheput_perf(csound, p);
}

/** pool opcodes
 *
 * a pool is a stack of integers.
 *
 * when a pool is generated, it contains a vector of integers, from 0 to size-1
 *
 * pool_pop will get one of those items
 * pool_push returns that item to the pool
 *
 * Use case is:
 *
 * gipool pool_gen 1000
 *
 * instr 1
 *   inum pool_get gipool
 *   schedule 2+inum/1000, ...
 *   ...
 * endin
 *
 * instr 2
 *   ; push the id back to the pool, at release time
 *   pool_push gipool, frac2int(p1), 1
 *   ;; or defer "pool_push", gipool, frac2int(p1)
 * endin
 *
 *
 *
 *
 */

typedef struct {
    int active;
    int size;
    int allocated;
    int cangrow;
    MYFLT *data;
    int handlenum;
} POOL_HANDLE;

typedef struct {
    CSOUND *csound;
    int numhandles;
    POOL_HANDLE *handles;
} POOL_GLOBALS;


typedef struct {
    OPDS h;
    MYFLT *handleidx;
    MYFLT *arg1;
    MYFLT *arg2;
} POOL_NEW;


static i32
pool_fill(POOL_HANDLE *handle, MYFLT start, MYFLT stop, MYFLT step) {
    int numitems = (int)((stop - start) / step);
    if (numitems <= 0 || numitems > handle->size || numitems > handle->allocated)
        return NOTOK;
    MYFLT x = start;
    MYFLT *data = handle->data;
    for(int i=numitems-1; i >= 0; i--) {
        data[i] = x;
        x += step;
    }
    return OK;
}

static i32* pool_instance_counter(CSOUND *csound) {
    i32 *poolcounter = csound->QueryGlobalVariable(csound, "_poolcounter");
    if(poolcounter == NULL) {
        csound->CreateGlobalVariable(csound, "_poolcounter", sizeof(i32));
        poolcounter = csound->QueryGlobalVariable(csound, "_poolcounter");
    }
    return poolcounter;
}

static POOL_HANDLE *pool_make(CSOUND *csound, int allocated, int cangrow) {
    i32 *instancecount = pool_instance_counter(csound);
    char name[32];
    sprintf(name, "_pool:%d", *instancecount);
    csound->CreateGlobalVariable(csound, name, sizeof(POOL_HANDLE));
    POOL_HANDLE *handle = csound->QueryGlobalVariable(csound, name);
    *instancecount++;
    handle->active = 1;
    handle->data = csound->Malloc(csound, sizeof(MYFLT) * allocated);
    if(handle->data == NULL)
        return INITERR("Allocation error when creating pool");
    handle->allocated = allocated;
    handle->size = 0;
    handle->cangrow = cangrow;
    handle->handlenum = *instancecount;
    return handle;  
}

static POOL_HANDLE *_pool_get_handle(CSOUND *csound, int instance) {
    i32 *instancecount = pool_instance_counter(csound);
    char name[32];
    sprintf(name, "_pool:%d", *instancecount);
    POOL_HANDLE *handle = csound->QueryGlobalVariable(csound, name);
    return handle;
}

static i32 _pool_free(CSOUND *csound, int instance) {
    POOL_HANDLE *handle = _pool_get_handle(csound, instance);
    if(handle == NULL) {
        return -1;
    }
    csound->Free(csound, handle->data);
    handle->data = NULL;
    i32 *instancecount = pool_instance_counter(csound);
    char name[32];
    sprintf(name, "_pool:%d", *instancecount);
    csound->DestroyGlobalVariable(csound, name);
    return 0;
}



/** pool_gen
 *
 * ipool pool_gen isize
 * ipool pool_gen istart, iend
 *
 * In its first form, it will generate a pool of the given size
 * filled with integers between 0 and size
 * In the second form, it will generate a pool filled with numbers from
 * istart to iend (inclusive)
 *
 * The pool can't be resized
 *
 * pool_gen 1, 100 -> 1 .. 100 (inclusive)
 *
 */

static i32
pool_gen(CSOUND *csound, POOL_NEW *p) {
    int a = (int)*p->arg1;
    int b = (int)*p->arg2;
    int start, end;
    int step = 1;
    if(b == 0) {
        start = 0;
        end = a;
    } else {
        start = a;
        step = a<b?1:-1;
        end = b+step;
    }
    int size = end - start;
    if(size < 0) {
        size = -size;
    }
    if(size <= 0)
        return INITERRF("Size must be positive (size=%d)", size);
    int allocated = size;
    int cangrow = 0;
    // POOL_GLOBALS *g = pool_globals(csound);
    // int slot = pool_create(csound, g, allocated, cangrow);
    POOL_HANDLE *handle = pool_make(csound, allocated, cangrow);
    // POOL_HANDLE *handle = &(g->handles[slot]);
    handle->size = size;
    if(size > 0)
        pool_fill(handle, start, end, step);
    *p->handleidx = handle->handlenum;
    return OK;
}

/**
 * pool_new: create an empty pool
 *
 * ipool pool_new [capacity=0]
 *
 * capacity: if not given, the pool is resizable
 *
 *
 */


static i32
pool_empty(CSOUND *csound, POOL_NEW *p) {
    int allocated = (int)*p->arg1;
    int cangrow = 0;
    if(allocated == 0) {
        allocated = 64;
        cangrow = 1;
    }
    POOL_HANDLE *handle = pool_make(csound, allocated, cangrow);
    *p->handleidx = handle->handlenum;
    return OK;
}


typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *handleidx, *arg1, *arg2, *arg3, *arg4;
    POOL_HANDLE *handle;
} POOL_1;

typedef struct {
    OPDS h;
    MYFLT *handleidx;
    MYFLT *arg1, *arg2;
    POOL_HANDLE *handle;
} POOL_0;

static i32
pool_free_deinit(CSOUND *csound, POOL_0 *p) {
    int when = *p->arg1;
    if(when == 1) {
        return _pool_free(csound, *p->handleidx);
    }
    return OK;
}

static i32
pool_free_init(CSOUND *csound, POOL_0 *p) {
    int when = *p->arg1;
    if(when == 0) {
        return _pool_free(csound, *p->handleidx);
    }
#ifdef CSOUNDAPI6)
    else { register_deinit(csound, p, pool_free_deinit); }
#endif
    return OK;
}


static i32
pool_1_init(CSOUND *csound, POOL_1 *p) {
    int handleidx = (int)*p->handleidx;
    p->handle = _pool_get_handle(csound, handleidx);
    if(p->handle == NULL)
        return INITERRF("Handle with idx %d does not exist", handleidx);
    return OK;
}


static i32
pool_pop_perf(CSOUND *csound, POOL_1 *p) {
    IGN(csound);
    int size = p->handle->size;
    MYFLT item;
    if(size > 0) {
        item = p->handle->data[size -1];
    } else {
        item = *p->arg1;
    }
    p->handle->size--;
    *p->out = item;
    return OK;
}

static i32
pool_pop_i(CSOUND *csound, POOL_1 *p) {
    int ans = pool_1_init(csound, p);
    if (ans == NOTOK)
        return NOTOK;
    return pool_pop_perf(csound, p);
}

static i32
pool_capacity_perf(CSOUND *csound, POOL_1 *p) {
    IGN(csound);
    *p->out = p->handle->allocated;
    return OK;
}

static i32
pool_capacity_i(CSOUND *csound, POOL_1 *p) {
    pool_1_init(csound, p);
    pool_capacity_perf(csound, p);
    return OK;
}

static i32
pool_isfull_perf(CSOUND *csound, POOL_1 *p) {
    IGN(csound);
    *p->out = p->handle->size == p->handle->allocated ? 1. : 0.;
    return OK;
}

static i32
pool_isfull_i(CSOUND *csound, POOL_1 *p) {
    pool_1_init(csound, p);
    return pool_isfull_perf(csound, p);
}


static i32
pool_size_perf(CSOUND *csound, POOL_1 *p) {
    IGN(csound);
    *p->out = p->handle->size;
    return OK;
}

static i32
pool_size_i(CSOUND *csound, POOL_1 *p) {
    pool_1_init(csound, p);
    pool_size_perf(csound, p);
    return OK;
}


// pool_put ipool, item, when=0 (0=at init, 1=at stop)
typedef struct {
    OPDS h;
    MYFLT *handleidx;
    MYFLT *item;
    MYFLT *when;
    POOL_HANDLE *handle;
} POOL_PUSH;

static i32
pool_push_init(CSOUND *csound, POOL_PUSH *p) {
    p->handle = _pool_get_handle(csound, (int)*p->handleidx);
    if(p->handle == NULL)
        return INITERRF("Invalid handle for idx: %d", (int)*p->handleidx);
    return OK;
}

void pool_resize(CSOUND *csound, POOL_HANDLE *handle, int minsize) {
    int allocated = handle->allocated;
    while(allocated < minsize) {
        allocated *= 2;
    }
    handle->data = csound->ReAlloc(csound, handle->data, sizeof(MYFLT)*allocated);
    handle->allocated = allocated;
}

static i32
pool_push_perf_(CSOUND *csound, POOL_PUSH *p, int mode) {
    if(p->handle->size >= p->handle->allocated) {
        if(p->handle->cangrow) {
            pool_resize(csound, p->handle, p->handle->allocated*2);
        } else {
            if(mode == 1)
                return INITERRF("Pool is full. Trying to push item %f to pool %d (size: %d, max. size: %d)", *p->item, p->handle->handlenum, p->handle->size, p->handle->allocated);
            else
                return PERFERRF("Pool is full. Trying to push item %f to pool %d (size: %d, max. size: %d)", *p->item, p->handle->handlenum, p->handle->size, p->handle->allocated);
        }
    }
    p->handle->data[p->handle->size] = *p->item;
    p->handle->size++;
    return OK;
}

static i32
pool_push_perf(CSOUND *csound, POOL_PUSH *p) {
    return pool_push_perf_(csound, p, 2);
}

static i32
pool_push_deinit(CSOUND *csound, POOL_PUSH *p) {
    if(*p->when == 1) {
        pool_push_perf_(csound, p, 2);
    }
    return OK;
}

static i32
pool_push_i(CSOUND *csound, POOL_PUSH *p) {
    pool_push_init(csound, p);
    if(*p->when == 0) {
        return pool_push_perf_(csound, p, 1);
    }
    return OK;

}

static i32
pool_at_perf(CSOUND *csound, POOL_1 *p) {
    int itemidx = (int)*p->arg1;
    if(itemidx >= p->handle->size)
        return PERFERRF("Index out of bounds: %d (size=%d)", itemidx, p->handle->size);
    *p->out = p->handle->data[itemidx];
    return OK;
}

static i32
pool_at_i(CSOUND *csound, POOL_1 *p) {
    pool_1_init(csound, p);
    return pool_at_perf(csound, p);
}


#define S(x) sizeof(x)

static OENTRY localops[] = {
#ifdef CSOUNDAPI6
    { "dict_new.size", S(DICT_NEW), 0, 1, "i", "Sj", (SUBR)dict_new, NULL, NULL, NULL },
    { "dict_new.many", S(DICT_NEW), 0, 1, "i", "S*", (SUBR)dict_new_many, NULL, NULL, NULL },
    { "dict_new.many_k", S(DICT_NEW), 0, 2, "k", "S*", NULL, (SUBR)dict_new_many, NULL, NULL },

    { "dict_free", S(DICT_FREE),   0, 1, "", "ip",   (SUBR)dict_free, NULL, NULL, NULL},

    { "dict_clear.i", S(DICT_CLEAR), 0, 1, "", "i", (SUBR)dict_clear_i, NULL, NULL, NULL},
    { "dict_clear.k", S(DICT_CLEAR), 0, 3, "", "k", (SUBR)dict_clear_init, (SUBR)dict_clear_perf, NULL, NULL},

    { "dict_get.sk_k", S(DICT_GET_sf), 0, 3, "k", "iSO", (SUBR)dict_get_sf_0, (SUBR)dict_get_sf, NULL, NULL },
    { "dict_get.ss_k", S(DICT_GET_ss), 0, 3, "S", "iS", (SUBR)dict_get_ss_0, (SUBR)dict_get_ss, NULL, NULL },
    { "dict_geti.ss_i", S(DICT_GET_ss), 0, 1, "S", "iS", (SUBR)dict_get_ss_i, NULL, NULL, NULL},
    { "dict_get.sf_i", S(DICT_GET_sf), 0, 1, "i", "iSo", (SUBR)dict_get_sf_i, NULL, NULL, NULL},
    { "dict_get.if_k", S(DICT_GET_if), 0, 3, "k", "ikO", (SUBR)dict_get_if_0, (SUBR)dict_get_if, NULL, NULL },
    { "dict_get.is_i", S(DICT_GET_is), 0, 1, "S", "ii", (SUBR)hashtab_get_is_i, NULL, NULL, NULL},

    { "dict_get.is_k", S(DICT_GET_is), 0, 3, "S", "ik", (SUBR)hashtab_get_is_0, (SUBR)hashtab_get_is, NULL, NULL },

    { "dict_get.if_i", S(DICT_GET_if), 0, 1, "i", "iio", (SUBR)dict_get_if_i, NULL, NULL, NULL},


    { "dict_set.ss_k", S(DICT_SET_ss), 0, 3, "",  "iSS", (SUBR)dict_set_ss_0, (SUBR)dict_set_ss, NULL, NULL },
    { "dict_set.sf_i", S(DICT_SET_sf), 0, 1, "",  "iSi", (SUBR)dict_set_sf_i, NULL, NULL, NULL},
    { "dict_set.sf_k", S(DICT_SET_sf), 0, 3, "",  "iSk", (SUBR)dict_set_sf_0, (SUBR)dict_set_sf, NULL, NULL },
    { "dict_set.if_i", S(DICT_SET_if), 0, 1, "",  "iii", (SUBR)dict_set_if_i, NULL, NULL, NULL},

    { "dict_set.if_k", S(DICT_SET_if), 0, 3, "",  "ikk", (SUBR)dict_set_if_0, (SUBR)dict_set_if, NULL, NULL },

    { "dict_set.is_i", S(DICT_SET_is), 0, 1, "",  "iiS", (SUBR)dict_set_is_i, NULL, NULL, NULL},

    { "dict_set.is_k", S(DICT_SET_is), 0, 3, "",  "ikS", (SUBR)dict_set_is_0, (SUBR)dict_set_is, NULL, NULL },

    { "dict_del.del_i", S(DICT_DEL_i),   0, 1, "", "ii",   (SUBR)dict_del_i, NULL, NULL, NULL },
    { "dict_del.del_k", S(DICT_DEL_i),   0, 2, "", "ik",   NULL, (SUBR)dict_del_i, NULL, NULL },
    { "dict_del.del_S", S(DICT_DEL_s),   0, 2, "", "iS",   NULL, (SUBR)dict_del_s, NULL, NULL },
    // { "dict_del", S(DICT_DEL_s), 0, 1, "", "iS", (SUBR)dict_del_s_atstop},
    // { "dict_del", S(DICT_DEL_s), 0, 1, "", "ii", (SUBR)dict_del_i_atstop},

    { "dict_print", S(DICT_PRINT), 0, 1, "", "i",  (SUBR)dict_print_i, NULL, NULL, NULL},
    { "dict_print", S(DICT_PRINT), 0, 3, "", "ik", (SUBR)dict_print_k_0, (SUBR)dict_print_k, NULL, NULL},

    { "dict_query.k", S(DICT_QUERY1), 0, 3, "k", "iS", (SUBR)dict_query_0, (SUBR)dict_query, NULL, NULL },
    { "dict_query.k[]", S(DICT_QUERY_ARR), 0, 3, "k[]", "iS", (SUBR)dict_query_arr_0, (SUBR)dict_query_arr, NULL, NULL},
    { "dict_query.S[]", S(DICT_QUERY_ARR), 0, 3, "S[]", "iS", (SUBR)dict_query_arr_0, (SUBR)dict_query_arr, NULL, NULL},

    { "dict_iter", S(DICT_ITER), 0, 3, "SSk", "iP", (SUBR)dict_iter_ss_0, (SUBR)dict_iter_perf, NULL, NULL},
    { "dict_iter", S(DICT_ITER), 0, 3, "Skk", "iP", (SUBR)dict_iter_sf_0, (SUBR)dict_iter_perf, NULL, NULL},
    { "dict_iter", S(DICT_ITER), 0, 3, "kSk", "iP", (SUBR)dict_iter_is_0, (SUBR)dict_iter_perf, NULL, NULL},
    { "dict_iter", S(DICT_ITER), 0, 3, "kkk", "iP", (SUBR)dict_iter_if_0, (SUBR)dict_iter_perf, NULL, NULL},

    { "dict_size.k", S(DICT_QUERY1), 0, 3, "k", "k", (SUBR)dict_size_0, (SUBR)dict_size, NULL, NULL},
    { "dict_size.i", S(DICT_QUERY1), 0, 1, "i", "i", (SUBR)dict_size_0, NULL, NULL, NULL},

    { "dict_exists.i", S(DICT_QUERY1), 0, 1, "i", "i", (SUBR)dict_exists, NULL, NULL, NULL },

    { "dict_loadstr", S(DICT_LOADSTR), 0, 1, "i", "S", (SUBR)dict_loadstr, NULL, NULL, NULL},
    { "dict_dump", S(DICT_DUMP), 0, 1, "S", "i", (SUBR)dict_dump, NULL, NULL, NULL},
    // { "cacheput.i", S(CACHEPUT), 0, 1, "i", "S", (SUBR)cacheput_i },
    // { "cacheput.k", S(CACHEPUT), 0, 3, "k", "S", (SUBR)cacheput_0, (SUBR)cacheput_perf },
    { "sref.i_set", S(CACHEPUT), 0, 1, "i", "S", (SUBR)cacheput_i, NULL, NULL, NULL },
    { "sref.k_set", S(CACHEPUT), 0, 3, "k", "S", (SUBR)cacheput_0, (SUBR)cacheput_perf, NULL, NULL },

    // { "strcache.i_get", S(CACHEGET), 0, 1, "S", "i", (SUBR)cacheget_i },
    // { "strcache.k_get", S(CACHEGET), 0, 3, "S", "k", (SUBR)cacheget_0, (SUBR)cacheget_perf },

    // { "strview.i", S(CACHEGET), 0, 1, "S", "i", (SUBR)sview_i},
    { "sderef.i", S(CACHEGET), 0, 1, "S", "i", (SUBR)sview_i, NULL, NULL, NULL},
    { "sderef.k", S(CACHEGET), 0, 3, "S", "k", (SUBR)sview_init, (SUBR)sview_k, NULL, NULL},

    { "strpeek.i", S(STRPEEK), 0, 1, "S", "i", (SUBR)strpeek_i, NULL, NULL, NULL },

    { "pool_gen", S(POOL_NEW), 0, 1, "i", "io", (SUBR)pool_gen, NULL, NULL, NULL},
    { "pool_new", S(POOL_NEW), 0, 1, "i", "o",  (SUBR)pool_empty, NULL, NULL, NULL},
    { "pool_free", S(POOL_0),  0, 1, "",  "io", (SUBR)pool_free_init, NULL, NULL, NULL },
    
    { "pool_pop.i", S(POOL_1), 0, 1, "i", "ij", (SUBR)pool_pop_i, NULL, NULL, NULL},
    { "pool_pop.k", S(POOL_1), 0, 3, "k", "iJ", (SUBR)pool_1_init, (SUBR)pool_pop_perf, NULL, NULL},

    { "pool_push.i", S(POOL_PUSH), 0, 1, "", "iio", (SUBR)pool_push_i, NULL, NULL, NULL},
    { "pool_push.k", S(POOL_PUSH), 0, 3, "", "ik", (SUBR)pool_push_init, (SUBR)pool_push_perf, NULL, NULL},

    { "pool_capacity.i", S(POOL_1), 0, 1, "i", "i", (SUBR)pool_capacity_i, NULL, NULL, NULL},
    { "pool_capacity.k", S(POOL_1), 0, 3, "k", "i", (SUBR)pool_1_init, (SUBR)pool_capacity_perf, NULL, NULL},

    { "pool_size.i", S(POOL_1), 0, 1, "i", "i", (SUBR)pool_size_i, NULL, NULL, NULL},
    { "pool_size.k", S(POOL_1), 0, 3, "k", "i", (SUBR)pool_1_init, (SUBR)pool_size_perf, NULL, NULL},

    { "pool_isfull.i", S(POOL_1), 0, 1, "i", "i", (SUBR)pool_isfull_i, NULL, NULL, NULL},
    { "pool_isfull.k", S(POOL_1), 0, 3, "k", "i", (SUBR)pool_1_init, (SUBR)pool_isfull_perf, NULL, NULL},

    { "pool_at.i", S(POOL_1), 0, 1, "i", "ii", (SUBR)pool_at_i, NULL, NULL, NULL},
    { "pool_at.k", S(POOL_1), 0, 3, "k", "ik", (SUBR)pool_1_init, (SUBR)pool_at_perf, NULL, NULL},
#else
    // Csound 7
    { "dict_new.size", S(DICT_NEW), 0, "i", "Sj", (SUBR)dict_new, NULL, NULL, NULL },
    { "dict_new.many", S(DICT_NEW), 0, "i", "S*", (SUBR)dict_new_many, NULL, NULL, NULL },
    { "dict_new.many_k", S(DICT_NEW), 0, "k", "S*", NULL, (SUBR)dict_new_many, NULL, NULL },

    { "dict_free", S(DICT_FREE),   0, "", "ip",   (SUBR)dict_free, NULL, (SUBR)dict_free_callback, NULL},

    { "dict_clear.i", S(DICT_CLEAR), 0, "", "i", (SUBR)dict_clear_i, NULL, NULL, NULL},
    { "dict_clear.k", S(DICT_CLEAR), 0, "", "k", (SUBR)dict_clear_init, (SUBR)dict_clear_perf, NULL, NULL},

    { "dict_get.sk_k",  S(DICT_GET_sf), 0, "k", "iSO", (SUBR)dict_get_sf_0, (SUBR)dict_get_sf, NULL, NULL },
    { "dict_get.ss_k",  S(DICT_GET_ss), 0, "S", "iS", (SUBR)dict_get_ss_0, (SUBR)dict_get_ss, NULL, NULL },
    { "dict_geti.ss_i", S(DICT_GET_ss), 0, "S", "iS", (SUBR)dict_get_ss_i, NULL, NULL, NULL},
    { "dict_get.sf_i",  S(DICT_GET_sf), 0, "i", "iSo", (SUBR)dict_get_sf_i, NULL, NULL, NULL},
    { "dict_get.if_k",  S(DICT_GET_if), 0, "k", "ikO", (SUBR)dict_get_if_0, (SUBR)dict_get_if, NULL, NULL },
    { "dict_get.is_i",  S(DICT_GET_is), 0, "S", "ii", (SUBR)hashtab_get_is_i, NULL, NULL, NULL},
    { "dict_get.is_k",  S(DICT_GET_is), 0, "S", "ik", (SUBR)hashtab_get_is_0, (SUBR)hashtab_get_is, NULL, NULL },
    { "dict_get.if_i",  S(DICT_GET_if), 0, "i", "iio", (SUBR)dict_get_if_i, NULL, NULL, NULL},

    { "dict_set.ss_k", S(DICT_SET_ss), 0, "",  "iSS", (SUBR)dict_set_ss_0, (SUBR)dict_set_ss, NULL, NULL },
    { "dict_set.sf_i", S(DICT_SET_sf), 0, "",  "iSi", (SUBR)dict_set_sf_i, NULL, NULL, NULL},
    { "dict_set.sf_k", S(DICT_SET_sf), 0, "",  "iSk", (SUBR)dict_set_sf_0, (SUBR)dict_set_sf, NULL, NULL },
    { "dict_set.if_i", S(DICT_SET_if), 0, "",  "iii", (SUBR)dict_set_if_i, NULL, NULL, NULL},
    { "dict_set.if_k", S(DICT_SET_if), 0, "",  "ikk", (SUBR)dict_set_if_0, (SUBR)dict_set_if, NULL, NULL },
    { "dict_set.is_i", S(DICT_SET_is), 0, "",  "iiS", (SUBR)dict_set_is_i, NULL, NULL, NULL},
    { "dict_set.is_k", S(DICT_SET_is), 0, "",  "ikS", (SUBR)dict_set_is_0, (SUBR)dict_set_is, NULL, NULL },

    { "dict_del.del_i", S(DICT_DEL_i), 0, "", "ii", (SUBR)dict_del_i, NULL, NULL, NULL },
    { "dict_del.del_k", S(DICT_DEL_i), 0, "", "ik", NULL, (SUBR)dict_del_i, NULL, NULL },
    { "dict_del.del_S", S(DICT_DEL_s), 0, "", "iS", NULL, (SUBR)dict_del_s, NULL, NULL },

    { "dict_print", S(DICT_PRINT), 0, "", "i",  (SUBR)dict_print_i, NULL, NULL, NULL},
    { "dict_print", S(DICT_PRINT), 0, "", "ik", (SUBR)dict_print_k_0, (SUBR)dict_print_k, NULL, NULL},

    { "dict_iter", S(DICT_ITER), 0, "SSk", "iP", (SUBR)dict_iter_ss_0, (SUBR)dict_iter_perf, NULL, NULL},
    { "dict_iter", S(DICT_ITER), 0, "Skk", "iP", (SUBR)dict_iter_sf_0, (SUBR)dict_iter_perf, NULL, NULL},
    { "dict_iter", S(DICT_ITER), 0, "kSk", "iP", (SUBR)dict_iter_is_0, (SUBR)dict_iter_perf, NULL, NULL},
    { "dict_iter", S(DICT_ITER), 0, "kkk", "iP", (SUBR)dict_iter_if_0, (SUBR)dict_iter_perf, NULL, NULL},

    { "dict_query.S[]", S(DICT_QUERY_ARR), 0, "S[]", "iS", (SUBR)dict_query_arr_0, (SUBR)dict_query_arr, NULL, NULL},
    { "dict_query.k",   S(DICT_QUERY1),    0, "k",   "iS", (SUBR)dict_query_0, (SUBR)dict_query, NULL, NULL},
    { "dict_query.k[]", S(DICT_QUERY_ARR), 0, "k[]", "iS", (SUBR)dict_query_arr_0, (SUBR)dict_query_arr, NULL, NULL},

    { "dict_size.k", S(DICT_QUERY1), 0, "k", "k", (SUBR)dict_size_0, (SUBR)dict_size, NULL, NULL},
    { "dict_size.i", S(DICT_QUERY1), 0, "i", "i", (SUBR)dict_size_0, NULL, NULL, NULL},

    { "dict_exists.i", S(DICT_QUERY1), 0, "i", "i", (SUBR)dict_exists, NULL, NULL, NULL },

    { "dict_loadstr", S(DICT_LOADSTR), 0, "i", "S", (SUBR)dict_loadstr, NULL, NULL, NULL},
    { "dict_dump", S(DICT_DUMP), 0, "S", "i", (SUBR)dict_dump, NULL, NULL, NULL},

    { "sref.i_set", S(CACHEPUT), 0, "i", "S", (SUBR)cacheput_i, NULL, NULL, NULL },
    { "sref.k_set", S(CACHEPUT), 0, "k", "S", (SUBR)cacheput_0, (SUBR)cacheput_perf, NULL, NULL },

    { "sderef.i", S(CACHEGET), 0, "S", "i", (SUBR)sview_i, NULL, (SUBR)sview_deinit, NULL},
    { "sderef.k", S(CACHEGET), 0, "S", "k", (SUBR)sview_init, (SUBR)sview_k, (SUBR)sview_deinit, NULL},

    // { "strpeek.i", S(STRPEEK), 0, "S", "i", (SUBR)strpeek_i, NULL, NULL, NULL },

    { "pool_gen", S(POOL_NEW), 0, "i", "io", (SUBR)pool_gen, NULL, NULL, NULL},
    { "pool_new", S(POOL_NEW), 0, "i", "o", (SUBR)pool_empty, NULL, NULL, NULL},
    { "pool_free", S(POOL_0), 0, "", "io", (SUBR)pool_free_init, NULL, (SUBR)pool_free_deinit },

    { "pool_pop.i", S(POOL_1), 0, "i", "ij", (SUBR)pool_pop_i, NULL, NULL, NULL},
    { "pool_pop.k", S(POOL_1), 0, "k", "iJ", (SUBR)pool_1_init, (SUBR)pool_pop_perf, NULL, NULL},

    { "pool_push.i", S(POOL_PUSH), 0, "", "iio", (SUBR)pool_push_i, NULL, (SUBR)pool_push_deinit, NULL},
    { "pool_push.k", S(POOL_PUSH), 0, "", "ik", (SUBR)pool_push_init, (SUBR)pool_push_perf, NULL, NULL},

    { "pool_capacity.i", S(POOL_1), 0, "i", "i", (SUBR)pool_capacity_i, NULL, NULL, NULL},
    { "pool_capacity.k", S(POOL_1), 0, "k", "i", (SUBR)pool_1_init, (SUBR)pool_capacity_perf, NULL, NULL},

    { "pool_size.i", S(POOL_1), 0, "i", "i", (SUBR)pool_size_i, NULL, NULL, NULL},
    { "pool_size.k", S(POOL_1), 0, "k", "i", (SUBR)pool_1_init, (SUBR)pool_size_perf, NULL, NULL},

    { "pool_isfull.i", S(POOL_1), 0, "i", "i", (SUBR)pool_isfull_i, NULL, NULL, NULL},
    { "pool_isfull.k", S(POOL_1), 0, "k", "i", (SUBR)pool_1_init, (SUBR)pool_isfull_perf, NULL, NULL},

    { "pool_at.i", S(POOL_1), 0, "i", "ii", (SUBR)pool_at_i, NULL, NULL, NULL},
    { "pool_at.k", S(POOL_1), 0, "k", "ik", (SUBR)pool_1_init, (SUBR)pool_at_perf, NULL, NULL},
#endif
};

LINKAGE
