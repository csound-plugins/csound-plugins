#ifndef _STR_COMMON_H
#define _STR_COMMON_H


char* join_strings(const char* strings[], const char* seperator, int count) {
    char* str = NULL;             /* Pointer to the joined strings  */
    size_t total_length = 0;      /* Total length of joined strings */
    int i = 0;                    /* Loop counter                   */

    /* Find total length of joined strings */
    for (i = 0; i < count; i++) total_length += strlen(strings[i]);
    total_length++;     /* For joined string terminator */
    total_length += strlen(seperator) * (count - 1); // for seperators

    str = (char*) malloc(total_length);  /* Allocate memory for joined strings */
    str[0] = '\0';                      /* Empty string we can append to      */

    /* Append all the strings */
    for (i = 0; i < count; i++) {
        strcat(str, strings[i]);
        if (i < (count - 1)) strcat(str, seperator);
    }

    return str;
}


// returns the size of a null-terminated array of strings. Mostly used for an array
// of char*, terminated by a NULL pointer
int nullterminated_chararray_len(const char *arr[], int maxitems) {
    for(int i=0; i < maxitems; i++) {
        if(arr[i] == NULL)
            return i;
    }
    return -1;
}



#endif
