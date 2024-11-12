/*
   sndmeta.c

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

#include "sndfile.h"
// This define is needed in csound7 to avoid soundfile.h getting included
// and creating conflicts with the symbols imported in sndfile.h
#define _SOUNDFILE_H_

#include "csdl.h"
#include <math.h>
#include <ctype.h>
#include "arrays.h"
#include "cs_strlib.h"
#include "../../common/_common.h"


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



#define FORCE_FORWARD_SLASH


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
    TABINIT(csound, p->skeys, numpairs);
    TABINIT(csound, p->svalues, numpairs);
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#define S(x) sizeof(x)

static OENTRY localops[] = {
#ifdef CSOUNDAPI6
    { "filereadmeta.i", S(SFREADMETA), 0, 1, "S", "SS", (SUBR)sfreadmeta_i, NULL, NULL, NULL},
    { "filereadmeta.i", S(SFREADMETA_SS), 0, 1, "S[]S[]", "S", (SUBR)sfreadmeta_ss, NULL, NULL, NULL},
#else
    { "filereadmeta.i", S(SFREADMETA), 0, "S", "SS", (SUBR)sfreadmeta_i, NULL, NULL, NULL},
    { "filereadmeta.i", S(SFREADMETA_SS), 0, "S[]S[]", "S", (SUBR)sfreadmeta_ss, NULL, NULL, NULL},
#endif
};

LINKAGE
