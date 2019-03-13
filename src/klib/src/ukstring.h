/* The MIT License
   Copyright (c) by Attractive Chaos <attractor@live.co.uk>
   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:
   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#ifndef KSTRING_H
#define KSTRING_H

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#ifndef kroundup32
#define kroundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))
#endif

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define KS_ATTR_PRINTF(fmt, arg) __attribute__((__format__ (__printf__, fmt, arg)))
#else
#define KS_ATTR_PRINTF(fmt, arg)
#endif


/* kstring_t is a simple non-opaque type whose fields are likely to be
 * used directly by user code (but see also ks_str() and ks_len() below).
 * A kstring_t object is initialised by either of
 *       kstring_t str = { 0, 0, NULL };
 *       kstring_t str; ...; str.l = str.m = 0; str.s = NULL;
 * and either ownership of the underlying buffer should be given away before
 * the object disappears (see ks_release() below) or the kstring_t should be
 * destroyed with  free(str.s);  */
#ifndef KSTRING_T
#define KSTRING_T kstring_t
typedef struct __kstring_t {
    size_t l, m;
    char *s;
} kstring_t;
#endif

static inline int ks_resize(kstring_t *s, size_t size)
{
    if (s->m < size) {
        char *tmp;
        s->m = size;
        kroundup32(s->m);
        if ((tmp = (char*)realloc(s->s, s->m)))
            s->s = tmp;
        else
            return -1;
    }
    return 0;
}

// Give ownership of the underlying buffer away to something else (making
// that something else responsible for freeing it), leaving the kstring_t
// empty and ready to be used again, or ready to go out of scope without
// needing  free(str.s)  to prevent a memory leak.
static inline char *ks_release(kstring_t *s)
{
    char *ss = s->s;
    s->l = s->m = 0;
    s->s = NULL;
    return ss;
}

// append string p of len l to string s, possibly reallocating s
static inline int kputsn(kstring_t *s, const char *p, int l)
{
    if (s->l + l + 1 >= s->m) {
        char *tmp;
        s->m = s->l + l + 2;
        kroundup32(s->m);
        if ((tmp = (char*)realloc(s->s, s->m)))
            s->s = tmp;
        else
            return EOF;
    }
    memcpy(s->s + s->l, p, l);
    s->l += l;
    s->s[s->l] = 0;
    return l;
}

// append str p to kstring s, possibly reallocating s
static inline int ks_puts(kstring_t *s, const char *p)
{
    return kputsn(s, p, strlen(p));
}


// append one char, make sure s ends with 0
static inline int ks_putc(kstring_t *s, int c)
{
    if (s->l + 1 >= s->m) {
        char *tmp;
        s->m = s->l + 2;
        kroundup32(s->m);
        if ((tmp = (char*)realloc(s->s, s->m)))
            s->s = tmp;
        else
            return EOF;
    }
    s->s[s->l++] = c;
    s->s[s->l] = 0;
    return c;
}

static inline int ks_putc_(kstring_t *s, int c)
{
    if (s->l + 1 > s->m) {
        char *tmp;
        s->m = s->l + 1;
        kroundup32(s->m);
        if ((tmp = (char*)realloc(s->s, s->m)))
            s->s = tmp;
        else
            return EOF;
    }
    s->s[s->l++] = c;
    return 1;
}

static inline int ks_putsn_(kstring_t *s, const void *p, int l)
{
    if (s->l + l > s->m) {
        char *tmp;
        s->m = s->l + l;
        kroundup32(s->m);
        if ((tmp = (char*)realloc(s->s, s->m)))
            s->s = tmp;
        else
            return EOF;
    }
    memcpy(s->s + s->l, p, l);
    s->l += l;
    return l;
}    

static inline int ks_setn(kstring_t *s, const char *p, int l) {
    if(l > s->m) {
        char *tmp;
        s->m = l + 1;
        kroundup32(s->m);
        if ((tmp = (char*)realloc(s->s, s->m)))
            s->s = tmp;
        else
            return EOF;
    }
    memcpy(s->s, p, l);
    s->s[l] = 0;
    s->l = l;
    return l;
}

static inline int ks_set(kstring_t *s, const char *p) {
    return ks_setn(s, p, strlen(p));
}

#endif
