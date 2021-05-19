#ifndef CS_STRLIB
#define CS_STRLIB

#include "csdl.h"


unsigned long next_power_of_two(unsigned long v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}


// like strncpy but really makes sure that the dest str is 0 terminated
// dest should have allocated at least n+1
static inline void strncpy0(char *dest, const char *src, size_t n) {
#ifdef _WIN32
    strncpy_s(dest, n+1, src, n);
#else
    strncpy(dest, src, n);
#endif
    dest[n] = '\0';
}


// make sure that the out string has enough allocated space to hold
// a string of `size` chars (the \0 should be taken into account)
// This can be run only at init time
void string_ensure(CSOUND *csound, STRINGDAT *s, size_t size) {
    if (s->size >= (int)size)
        return;
    size = next_power_of_two(size);
    s->data = csound->ReAlloc(csound, s->data, size);
    s->size = (int)size;
    return;
}


void stringdat_copy_cstr(CSOUND *csound, STRINGDAT *dest, const char *src, size_t len) {
    string_ensure(csound, dest, len+1);
    strncpy0(dest->data, src, len);
}

void stringdat_copy_literal(CSOUND *csound, STRINGDAT *dest, const char *src) {
    size_t len = strlen(src);
    string_ensure(csound, dest, len+1);
    strncpy0(dest->data, src, len);
}

void stringdat_clear(CSOUND *csound, STRINGDAT *s) {
    string_ensure(csound, s, 1);
    s->data[0] = '\0';
}

#endif
