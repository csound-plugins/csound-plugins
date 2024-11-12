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
// #include <ctype.h>
// #include "arrays.h"
#include "cs_strlib.h"

#include "../../common/_common.h"

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


#define MSGF(fmt, ...) (csound->Message(csound, fmt, __VA_ARGS__))
#define MSG(s) (csound->Message(csound, s))
#define INITERR(m) (csound->InitError(csound, "%s", m))
#define INITERRF(fmt, ...) (csound->InitError(csound, fmt, __VA_ARGS__))
#define PERFERR(m) (csound->PerfError(csound, &(p->h), "%s", m))
#define PERFERRF(fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))

// -------------------------------------------------------------------------------------

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

static char *risset_root() {
    if(strlen(rissetroot) == 0) {
        _getroot(rissetroot);
    }
    return rissetroot;
}

static int32_t risset1(CSOUND *csound, RISSET1 *p) {
    char *data = p->cmd->data;
    if (!strcmp("root", data)) {
        char *root = risset_root();
        stringdat_copy_cstr(csound, p->out, root, strlen(root));
    } else if(!strcmp("assets", data)) {
        char *root = risset_root();
        int rootlen = strlen(root);
        int totallen = rootlen + 8;
        string_ensure(csound, p->out, totallen);
        strncpy0(p->out->data, root, rootlen);
        strncpy0(p->out->data + rootlen, "/assets", 8);
    } else {
        p->out->data[0] = 0;
        return INITERRF("risset: command %s not understood. Possible commands: root, assets", data);
    }
    return OK;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#define S(x) sizeof(x)

static OENTRY localops[] = {
#ifdef CSOUNDAPI6
     { "risset.1", S(RISSET1), 0, 1, "S", "S", (SUBR)risset1, NULL, NULL, NULL }
#else
    { "risset.1", S(RISSET1), 0, "S", "S", (SUBR)risset1, NULL, NULL, NULL }
#endif
};

LINKAGE
