/*
   pathtools.c

  Copyright (C) 2020 Eduardo Moguillansky

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


*/

#include "csdl.h"
#include <math.h>
#include <ctype.h>
#include "arrays.h"
#include "cs_strlib.h"

#if defined WIN32 || defined __MINGW32__ || defined _WIN32 || defined _WIN64
    #define OS_WIN32
#endif


#ifdef OS_WIN32
#include <direct.h>
#define getcurrdir _getcwd
#else
#include <unistd.h>
#define getcurrdir getcwd
#endif


#ifndef OS_WIN32
  // we disable any opcode using sndfile in windows until I can figure out how to
  // build it using github actions
  #include "sndfile.h"
#endif


#define FORCE_FORWARD_SLASH


#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

#define MSGF(fmt, ...) (csound->Message(csound, fmt, __VA_ARGS__))
#define MSG(s) (csound->Message(csound, s))
#define INITERR(m) (csound->InitError(csound, "%s", m))
#define INITERRF(fmt, ...) (csound->InitError(csound, fmt, __VA_ARGS__))
#define PERFERR(m) (csound->PerfError(csound, &(p->h), "%s", m))
#define PERFERRF(fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))


// #define DEBUG

#ifdef DEBUG
    #define DBG(fmt, ...) printf(">>>> "fmt"\n", __VA_ARGS__); fflush(stdout);
    #define DBG_(m) DBG("%s", m)
#else
    #define DBG(fmt, ...)
    #define DBG_(m)
#endif


#define SAMPLE_ACCURATE(out) \
    uint32_t n, nsmps = CS_KSMPS;                                    \
    uint32_t offset = p->h.insdshead->ksmps_offset;                  \
    uint32_t early = p->h.insdshead->ksmps_no_end;                   \
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));   \
    if (UNLIKELY(early)) {                                           \
        nsmps -= early;                                              \
        memset(&out[nsmps], '\0', early*sizeof(MYFLT));              \
    }                                                                \


#define register_deinit(csound, p, func) \
    csound->RegisterDeinitCallback(csound, p, (int32_t(*)(CSOUND*, void*))(func))


static inline char _get_path_separator() {
#ifdef FORCE_FORWARD_SLASH
    return '/';
#endif
#ifdef OS_WIN32
    return '\\';
#else
    return '/';
#endif
}

static inline char _get_path_delim() {
#ifdef OS_WIN32
    return ';';
#else
    return ':';
#endif
}


static int32_t _str_rfind(const char *s, char ch) {
    // find first occurrence of ch in s from right to left
    // returns -1 if not found
    int len = (int)strlen(s);
    if(len == 0)
        return -1;
    for(int i=len-1; i>=0; i--) {
        if(s[i] == ch)
            return i;
    }
    return -1;
}


static void _str_replace_inplace(char *s, char orig, char replacement) {
    while (*s) {
        if(*s == orig)
            *s = replacement;
        s++;
    }
}


/*
static void _path_make_native_inplace(char *s, size_t len) {
#ifdef OS_WIN32
    char wrongsep = '/';
    char sep = '\\';
#else
    char wrongsep = '\\';
    char sep = '/';
#endif
    _str_replace_inplace(s, sep, wrongsep);
}
*/


typedef struct {
    OPDS h;
    ARRAYDAT *parts;
    STRINGDAT *s;
    STRINGDAT *sep;
} STRSPLIT;

static inline char *strsubdup(CSOUND *csound, char *src, int64_t start, int64_t len) {
    char *dest = csound->Malloc(csound, len+1);
    memcpy(dest, src+start, len);
    dest[len] = '\0';
    return dest;
}

static int32_t string_split(CSOUND *csound, STRSPLIT *p) {
    int numseps = 0;
    enum { maxseps = 1000 };
    int32_t separator_starts[maxseps];
    char *sep = p->sep->data;
    size_t seplen = strlen(sep);
    char *s = p->s->data;
    char *ptr = s;
    while (1) {
        ptr = strstr(ptr, sep);
        if(ptr == NULL)
            break;
        int32_t offset = ptr - s;
        separator_starts[numseps] = offset;
        numseps += 1;
        if(numseps >= maxseps) {
            return INITERRF("Too many separators in string %s", s);
        }
        ptr += seplen;
    }
    tabinit(csound, p->parts, numseps+1);
    STRINGDAT *parts = (STRINGDAT *)p->parts->data;
    int64_t start = 0;
    int64_t partlen;
    for(int i=0; i<numseps; i++) {
        partlen = separator_starts[i] - start;
        parts[i].size = partlen + 1;
        parts[i].data = strsubdup(csound, s, start, partlen);
        start = separator_starts[i] + seplen;
    }
    // last part
    partlen = strlen(s+start);
    parts[numseps].size = partlen+1;
    parts[numseps].data = strsubdup(csound, s, start, partlen);
    return OK;
}

typedef struct {
    OPDS h;
    STRINGDAT *out;
    STRINGDAT *sep;
    ARRAYDAT *strs;
} STRJOIN_ARR;

static int32_t strjoin_arr_i(CSOUND *csound, STRJOIN_ARR *p) {
    int32_t strsize = 0;
    int32_t sizes[2000];
    char *sep = p->sep->data;
    int32_t sepsize = strlen(sep);
    if(p->strs->dimensions != 1)
        return INITERRF("Input array must be 1D, got %d", p->strs->dimensions);
    if(p->strs->sizes[0] > 2000) {
        return INITERRF("Array too big, max len is 2000, got %d", p->strs->sizes[0]);
    }
    STRINGDAT *strs = (STRINGDAT *)p->strs->data;
    for(int i=0; i<p->strs->sizes[0]; i++) {
        sizes[i] = strlen(strs[i].data);
        strsize += sizes[i];
        strsize += sepsize;
    }
    string_ensure(csound, p->out, strsize+1);
    char *dest = p->out->data;
    for(int i=0; i<p->strs->sizes[0]; i++) {
        memcpy(dest, strs[i].data, sizes[i]);
        dest += sizes[i];
        if(sepsize > 0) {
            memcpy(dest, sep, sepsize);
            dest += sepsize;
        }
    }
    p->out->data[strsize - sepsize] = '\0';
    return OK;
}

typedef struct {
    OPDS h;
    STRINGDAT* out;
    STRINGDAT* sep;
    STRINGDAT* strs[200];
} STRJOIN_VARARGS;

static int32_t strjoin_varargs_i(CSOUND *csound, STRJOIN_VARARGS *p) {
    int32_t strsize = 0;
    int32_t sizes[200];
    char *sep = p->sep->data;
    int32_t sepsize = strlen(sep);
    int32_t numstrs = csound->GetInputArgCnt(p) - 1;
    if(numstrs > 200) {
        return INITERRF("Too many arguments, max. is 200, got %d", numstrs);
    }
    for(int i=0; i<numstrs; i++) {
        sizes[i] = strlen(p->strs[i]->data);
        strsize += sizes[i];
        strsize += sepsize;
    }
    string_ensure(csound, p->out, strsize+1);
    char *dest = p->out->data;
    for(int i=0; i<numstrs; i++) {
        memcpy(dest, p->strs[i]->data, sizes[i]);
        dest += sizes[i];
        if(sepsize > 0) {
            memcpy(dest, sep, sepsize);
            dest += sepsize;
        }
    }
    p->out->data[strsize - sepsize] = '\0';
    return OK;

}


// Sdir, Sfolder pathSplit Sfile
typedef struct {
    OPDS h;
    STRINGDAT *S1, *S2;
    STRINGDAT *Spath;
} SS_S;


static int32_t pathSplit_opcode(CSOUND *csound, SS_S *p) {
    size_t pathlen = strlen(p->Spath->data);
    if(pathlen == 0) {
        return PERFERR("pathSplit: source path is empty");
    }

    char sep = _get_path_separator();
    int i = _str_rfind(p->Spath->data, sep) + 1;
    if(i == 0) {
        // no path separator, so no dir, all base
        stringdat_clear(csound, p->S1);
        stringdat_copy_cstr(csound, p->S2, p->Spath->data, pathlen);
        return OK;
    }
    if(i == 1) {
        // the path is of the type /filename.ext
        // so Sdir is /
        stringdat_copy_cstr(csound, p->S1, "/", 1);
        stringdat_copy_cstr(csound, p->S2, p->Spath->data+1, pathlen-1);
        return OK;
    }

    stringdat_copy_cstr(csound, p->S1, p->Spath->data, i-1);
    stringdat_copy_cstr(csound, p->S2, p->Spath->data+i, pathlen-i);
    return OK;
}


static int32_t pathSplitExt_opcode(CSOUND *csound, SS_S *p) {
    size_t pathlen = strlen(p->Spath->data);
    if(pathlen == 0) {
        return PERFERR("pathSplit: source path is empty");
    }

    int i = _str_rfind(p->Spath->data, '.');
    if(i == -1) {
        // no . so no extension
        stringdat_copy_cstr(csound, p->S1, p->Spath->data, pathlen);
        stringdat_clear(csound, p->S2);
        return OK;
    }
    if(i == 0) {
        // Only an extension is not an extension but a hidden file
        stringdat_clear(csound, p->S1);
        stringdat_copy_cstr(csound, p->S2, p->Spath->data, pathlen);
        return OK;
    }
    stringdat_copy_cstr(csound, p->S1, p->Spath->data, i);
    stringdat_copy_cstr(csound, p->S2, p->Spath->data+i, pathlen-i);
    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *out;
    STRINGDAT *s;
} K_S;


static int32_t _path_is_absolute(char *path, size_t len) {
    char sep = _get_path_separator();
#ifdef OS_WIN32
    if(len < 2)
        return 0;
    if(path[1] == ':' && (path[2] == '/' || path[2] == '\\') ) {
        // starts with a drive, it is absolute
        return 1;
    }
    return 0;
#else
    if(len == 0)
        return 0;
    return path[0] == sep ? 1 : 0;
#endif
}


static int32_t pathIsAbsolute(CSOUND *csound, K_S *p) {
    char *data = p->s->data;
    size_t len = strlen(data);
    if(len == 0) {
        MSG("Path is empty\n");
        return NOTOK;
    }
    *p->out = _path_is_absolute(data, len);
    return OK;
}


typedef struct {
    OPDS h;
    STRINGDAT *Sout;
} S_;


typedef struct {
    OPDS h;
    STRINGDAT *Sout;
    STRINGDAT *s;
} S_S;

typedef struct {
    OPDS h;
    STRINGDAT *Sout;
    STRINGDAT *S1, *S2;
} S_SS;


// this should only be called in windows
static void _win32_normalize_path_slashes(char *s) {
#ifdef FORCE_FORWARD_SLASH
    _str_replace_inplace(s, '\\', '/');
#else
    _str_replace_inplace(s, '/', '\\');
#endif
}

static int32_t pathJoin(CSOUND *csound, S_SS *p) {
    size_t len1 = strlen(p->S1->data);
    size_t len2 = strlen(p->S2->data);
    char sep = _get_path_separator();
    if(len1 == 0) {
        stringdat_copy_cstr(csound, p->Sout, p->S2->data, len2);
        return OK;
    }
    else if(len2 == 0) {
        stringdat_copy_cstr(csound, p->Sout, p->S1->data, len1);
        return OK;
    }
    size_t soutlen = len1+len2+2;
    string_ensure(csound, p->Sout, soutlen);
    strncpy0(p->Sout->data, p->S1->data, len1);
    if(p->Sout->data[len1-1] != sep) {
        p->Sout->data[len1] = sep;
        strncpy0(p->Sout->data + len1 + 1, p->S2->data, len2);
    } else {
        // it already has a separator, just join them
        strncpy0(p->Sout->data + len1, p->S2->data, len2);
    }
#ifdef OS_WIN32
    _win32_normalize_path_slashes(p->Sout->data);
#endif
    return OK;
}

// returns the length of the string in outbuf, or -1
static int64_t _path_make_absolute(CSOUND *csound,
                                   char *outbuf, size_t outbuflen,
                                   char *path) {
    size_t slen = strlen(path);
    if(slen == 0) {
        MSG("pathAbsolute: Path is empty\n");
        return -1;
    }
    if(slen > outbuflen) {
        MSG("pathAbsolute: Path two long!\n");
        return -1;
    }

#ifdef OS_WIN32
    if(path[0] == '/') {
        MSG("Path is ambiguous. This looks like a unix absolute path (it starts with a\n"
            " forward slash), but is not an absolute path in windows. Prepending the \n"
            " current working directory will probably result in an invalid path\n");
        return NOTOK;
    }
#endif

    char sep = _get_path_separator();

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
    if(path[0] == '~' && path[1] == sep) {
        char *home = getenv("HOME");
        size_t homelen = strlen(home);
        if(outbuflen < slen+homelen+10) {
            return NOTOK;
        }
        strncpy0(outbuf, home, homelen);
        strncpy0(outbuf+homelen, &path[1], slen-1);
        return slen-1+homelen;
    }
#endif

    int isabsolute = _path_is_absolute(path, slen);
    if (isabsolute) {
        strncpy0(outbuf, path, slen);
        return slen;
    }

    if(outbuflen < 1024)
        return -1;

    if (getcurrdir(outbuf, outbuflen - slen - 2) == NULL) {
        outbuf[0] = '\0';
        MSG("Could not get the current working directory\n");
        return -1;
    }

    // now concatenate the abs path
    size_t lenout = strlen(outbuf);
    if(outbuf[lenout-1] != sep) {
        outbuf[lenout] = sep;
        strncpy0(outbuf + (lenout + 1), path, slen);
        lenout += slen + 1;
    } else {
        strncpy0(outbuf + lenout, path, slen);
        lenout += slen;
    }
    return lenout;
#ifdef OS_WIN32
    _win32_normalize_path_slashes(p->Sout->data);
#endif

    return OK;
}

static int32_t pathAbsolute(CSOUND *csound, S_S *p) {
    size_t slen = strlen(p->s->data);
    if(slen == 0) {
        MSG("pathAbsolute: Path is empty\n");
        return NOTOK;
    }
    if(slen > 1000) {
        MSG("pathAbsolute: Path two long!\n");
        return NOTOK;
    }

#ifdef OS_WIN32
    if(p->s->data[0] == '/') {
        MSG("Path is ambiguous. This looks like a unix absolute path (it starts with a\n"
            " forward slash), but is not an absolute path in windows. Prepending the \n"
            " current working directory will probably result in an invalid path\n");
        return NOTOK;
    }
#endif

    char sep = _get_path_separator();

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
    if(p->s->data[0] == '~' && p->s->data[1] == sep) {
        char *home = getenv("HOME");
        size_t homelen = strlen(home);
        string_ensure(csound, p->Sout, slen + homelen + 10);
        strncpy0(p->Sout->data, home, homelen);
        strncpy0(p->Sout->data + homelen, p->s->data + 1, slen-1);
        return OK;
    }
#endif

    int isabsolute = _path_is_absolute(p->s->data, slen);
    if (isabsolute) {
        stringdat_copy_cstr(csound, p->Sout, p->s->data, slen);
        return OK;
    }

    string_ensure(csound, p->Sout, 1024);
    if (getcurrdir(p->Sout->data, p->Sout->size - slen - 2) == NULL) {
        stringdat_clear(csound, p->Sout);
        MSG("Could not get the current working directory\n");
        return NOTOK;
    }

    // now concatenate the abs path
    size_t lenout = strlen(p->Sout->data);
    string_ensure(csound, p->Sout, lenout + 2 + slen);
    char *outdata = p->Sout->data;
    if(outdata[lenout-1] != sep) {
        outdata[lenout] = sep;
        strncpy0(outdata + lenout+1, p->s->data, slen);
        lenout += slen + 1;
    } else {
        strncpy0(outdata + lenout, p->s->data, slen);
        lenout += slen;
    }
#ifdef OS_WIN32
    _win32_normalize_path_slashes(p->Sout->data);
#endif

    return OK;
}

// find file in path, resolve to an absolute path
static int32_t findFile(CSOUND *csound, S_S *p) {
    enum { buflen = 1024 };
    char buf[buflen];
    char *outpath = csound->FindInputFile(csound, p->s->data, "SSDIR");
    if(outpath == NULL) {
        stringdat_clear(csound, p->Sout);
        return OK;
    }
    size_t outpathlen = strlen(outpath);
    if(_path_is_absolute(outpath, outpathlen)) {
        stringdat_copy_cstr(csound, p->Sout, outpath, strlen(outpath));
    } else {
        int64_t pathlen = _path_make_absolute(csound, buf, buflen, outpath);
        if(pathlen < 0) {
            return INITERRF("Could not convert path %s to absolute", outpath);
        }
        stringdat_copy_cstr(csound, p->Sout, buf, pathlen);
    }
    csound->Free(csound, outpath);
    return OK;
}


static int32_t getEnvVar(CSOUND *csound, S_S *p) {
    const char *val = csound->GetEnv(csound, p->s->data);
    if(val != NULL) {
        stringdat_copy_cstr(csound, p->Sout, val, strlen(val));
        return OK;
    }
    val = getenv(p->s->data);
    if(val != NULL) {
        stringdat_copy_cstr(csound, p->Sout, val, strlen(val));
        return OK;
    }
    stringdat_clear(csound, p->Sout);
    return OK;
}


static int32_t pathOfScript(CSOUND *csound, S_ *p) {
    // return the path of the current script
    const char *val = csound->GetEnv(csound, "SSDIR");
    if(val == NULL) {
        stringdat_clear(csound, p->Sout);
        return OK;
    }
    char delim = _get_path_delim();
    int delimidx = _str_rfind(val, delim);
    if(delimidx < 0) {
        stringdat_copy_cstr(csound, p->Sout, val, strlen(val));
        return OK;
    }
    stringdat_copy_cstr(csound, p->Sout, val+delimidx+1, strlen(val)-delimidx+1);
    return OK;
}

// convert the path to its native version (using native separators)
static int32_t pathNative(CSOUND *csound, S_S *p) {
    size_t len = strlen(p->s->data);
    stringdat_copy_cstr(csound, p->Sout, p->s->data, len);
    char *outdata = p->Sout->data;
#ifdef OS_WIN32
    char wrongsep = '/';
    char sep = '\\';
#else
    char wrongsep = '\\';
    char sep = '/';
#endif
    _str_replace_inplace(outdata, wrongsep, sep);
    return OK;
}

static int32_t getPlatform(CSOUND *csound, S_ *p) {
#ifdef OS_WIN32
    stringdat_copy_literal(csound, p->Sout, "windows");
    return OK;
#endif
#ifdef __APPLE__
    stringdat_copy_literal(csound, p->Sout, "macos");
    return OK;
#endif
#ifdef __linux__
    stringdat_copy_literal(csound, p->Sout, "linux");
    return OK;
#endif
#ifdef __unix__
    stringdat_copy_literal(csound, p->Sout, "unix");
    return OK;
#else
    stringdat_clear(csound, p->Sout);
    return OK;
#endif
}


#ifndef OS_WIN32

// Sstr filereadmeta "sndfile.wav", Skey
typedef struct {
    OPDS h;
    STRINGDAT *sout;
    STRINGDAT *sndfile;
    STRINGDAT *key;
} SFREADMETA;


static int sf_string_to_type(const char *key) {
    int str_type;
    if(!strcmp(key, "comment"))
        str_type = SF_STR_COMMENT;
    else if(!strcmp(key, "title"))
        str_type = SF_STR_TITLE;
    else if(!strcmp(key, "artist"))
        str_type = SF_STR_ARTIST;
    else if(!strcmp(key, "album"))
        str_type = SF_STR_ALBUM;
    else if(!strcmp(key, "tracknumber"))
        str_type = SF_STR_TRACKNUMBER;
    else if(!strcmp(key, "software"))
        str_type = SF_STR_SOFTWARE;
    else {
        str_type = 0;
    }
    return str_type;
}


static const char * sf_strtype_to_string(int str_type) {
    switch(str_type) {
    case SF_STR_COMMENT:
        return "comment";
    case SF_STR_TITLE:
        return "title";
    case SF_STR_ARTIST:
        return "artist";
    case SF_STR_ALBUM:
        return "album";
    case SF_STR_TRACKNUMBER:
        return "tracknumber";
    default:
        return NULL;
    }
}


static int32_t sfreadmeta_i(CSOUND *csound, SFREADMETA *p) {
    SNDFILE *file;
    SF_INFO sfinfo;

    char *key = p->key->data;
    int str_type = sf_string_to_type(key);
    if(str_type == 0)
        return INITERRF("Key not supported: %s", key);

    if ((file = sf_open (p->sndfile->data, SFM_READ, &sfinfo)) == NULL) {
        return INITERRF("Error: Not able to open input file %s.\n", p->sndfile->data);
    }
    const char *svalue = sf_get_string(file, str_type);
    if(svalue == NULL) {
        stringdat_clear(csound, p->sout);
        return OK;
    }
    size_t slen = strlen(svalue);
    stringdat_copy_cstr(csound, p->sout, svalue, slen);
    return OK;
}


typedef struct {
    OPDS h;
    ARRAYDAT *skeys;
    ARRAYDAT *svalues;
    STRINGDAT *sndfile;
} SFREADMETA_SS;

static int32_t sfreadmeta_ss(CSOUND *csound, SFREADMETA_SS *p) {
    SNDFILE *file;
    SF_INFO sfinfo;
    const char *skey, *svalue;
    if ((file = sf_open (p->sndfile->data, SFM_READ, &sfinfo)) == NULL) {
        return INITERRF("Error: Not able to open input file %s.\n", p->sndfile->data);
    }
    // first, count number of key:value pairs
    int numpairs = 0;

    for(int str_type=SF_STR_FIRST; str_type<SF_STR_LAST; str_type++) {
        if(sf_get_string(file, str_type) != NULL)
            numpairs++;
    }
    tabinit(csound, p->skeys, numpairs);
    tabinit(csound, p->svalues, numpairs);
    STRINGDAT *keys = (STRINGDAT *)p->skeys->data;
    STRINGDAT *values = (STRINGDAT *)p->svalues->data;
    size_t i=0;
    for(int str_type=SF_STR_FIRST; str_type<SF_STR_LAST; str_type++) {
        svalue = sf_get_string(file, str_type);
        if(svalue == NULL)
            continue;
        skey = sf_strtype_to_string(str_type);
        if(skey == NULL)
            continue;
        stringdat_copy_cstr(csound, &keys[i], skey, strlen(skey));
        stringdat_copy_cstr(csound, &values[i], svalue, strlen(svalue));
        i++;
    }
    return OK;
}

#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#define S(x) sizeof(x)

static OENTRY localops[] = {
     { "pathSplit", S(SS_S), 0, 1, "SS", "S", (SUBR)pathSplit_opcode }
    ,{ "pathSplitk", S(SS_S), 0, 2, "SS", "S", NULL, (SUBR)pathSplit_opcode }
    ,{ "pathSplitExt", S(SS_S), 0, 1, "SS", "S", (SUBR)pathSplitExt_opcode }
    ,{ "pathSplitExtk", S(SS_S), 0, 2, "SS", "S", NULL, (SUBR)pathSplitExt_opcode }
    ,{ "pathIsAbsolute.i", S(K_S), 0, 1, "i", "S", (SUBR)pathIsAbsolute}
    ,{ "pathIsAbsolute.k", S(K_S), 0, 2, "k", "S", NULL, (SUBR)pathIsAbsolute}
    ,{ "pathAbsolute", S(S_S), 0, 1, "S", "S", (SUBR)pathAbsolute}
    ,{ "pathJoin", S(S_SS), 0, 1, "S", "SS", (SUBR)pathJoin}
    ,{ "findFileInPath", S(S_S), 0, 1, "S", "S", (SUBR)findFile}
    ,{ "getEnvVar", S(S_S), 0, 1, "S", "S", (SUBR)getEnvVar }
    ,{ "scriptDir", S(S_), 0, 1, "S", "", (SUBR)pathOfScript }
    ,{ "pathNative", S(S_S), 0, 1, "S", "S", (SUBR)pathNative }
    ,{ "sysPlatform", S(S_), 0, 1, "S", "", (SUBR)getPlatform}
    ,{ "strsplit", S(STRSPLIT), 0, 1, "S[]", "SS", (SUBR)string_split}
    ,{ "strjoin.arr_i", S(STRJOIN_ARR), 0, 1, "S", "SS[]", (SUBR)strjoin_arr_i}
    ,{ "strjoin.i", S(STRJOIN_VARARGS), 0, 1, "S", "S*", (SUBR)strjoin_varargs_i}

#ifndef OS_WIN32
    ,{ "filereadmeta.i", S(SFREADMETA), 0, 1, "S", "SS", (SUBR)sfreadmeta_i}
    ,{ "filereadmeta.i", S(SFREADMETA_SS), 0, 1, "S[]S[]", "S", (SUBR)sfreadmeta_ss}
#endif
};

LINKAGE
