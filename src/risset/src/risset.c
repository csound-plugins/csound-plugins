/*
   risset.c

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
    #define OS_WINDOWS
#endif


#ifdef OS_WINDOWS
#include <direct.h>
#define getcurrdir _getcwd
#else
#include <unistd.h>
#define getcurrdir getcwd
#endif


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


typedef struct {
    OPDS h;
    STRINGDAT *out;
    STRINGDAT *cmd;
} RISSET1;


/* risset root
 *
 * linux: ~/.local/share/risset
 * macos: ~/Library/Application Support/risset
 * windows: %LocalAppData%/risset
 */


static char rissetroot[256] = "";

static void _getroot(char *dest) {
    #ifdef __linux__
    char *home = getenv("HOME");
    strncpy(dest, home, 100);
    strcat(dest, "/.local/share/risset");
#endif
#ifdef __APPLE__
    char *home = getenv("HOME");
    strncpy(dest, home, 100);
    strcat(dest, "/Library/Application Support/risset");

#endif
#ifdef OS_WINDOWS
    char *localappdata = getenv("LocalAppData");
    strncpy(dest, localappdata, 200);
    strncat(dest, "/risset");
#endif
}

static int32_t risset1(CSOUND *csound, RISSET1 *p) {
    char *data = p->cmd->data;

    if (!strcmp("root", data)) {
        int l = strlen(rissetroot);
        if(l == 0) {
            _getroot(rissetroot);
            l = strlen(rissetroot);
        }
        stringdat_copy_cstr(csound, p->out, rissetroot, l);
    } else if(!strcmp("assets", data)) {
        string_ensure(csound, p->out, 256);
        char *outdata = p->out->data;
        strcpy(outdata, rissetroot);
        strcat(outdata, "/assets");
    } else {
        p->out->data[0] = 0;
        return INITERRF("risset: command %s not understood. Possible commands: root, assets", data);
    }
    return OK;
}

static inline char *strsubdup(CSOUND *csound, char *src, int64_t start, int64_t len) {
    char *dest = csound->Malloc(csound, len+1);
    memcpy(dest, src+start, len);
    dest[len] = '\0';
    return dest;
}


static int32_t _path_is_absolute(char *path, size_t len) {
    char sep = '/';
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


// this should only be called in windows
static void _win32_normalize_path_slashes(char *s) {
#ifdef FORCE_FORWARD_SLASH
    _str_replace_inplace(s, '\\', '/');
#else
    _str_replace_inplace(s, '/', '\\');
#endif
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

    char sep = '/';

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
    _win32_normalize_path_slashes(outbuf);
#endif

    return OK;
}


/*
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
*/

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#define S(x) sizeof(x)

static OENTRY localops[] = {
     { "risset.1", S(RISSET1), 0, 1, "S", "S", (SUBR)risset1 }
};

LINKAGE
