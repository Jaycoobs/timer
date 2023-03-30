#include <stdlib.h>
#include <string.h>

#include "util.h"

char* path_join(char* base, char* path) {
    size_t base_len = strlen(base);
    size_t path_len = strlen(path);
    char* buffer = malloc(base_len + path_len + 1);
    strcpy(buffer, base);
    if (buffer[base_len-1] != '/') {
        buffer[base_len] = '/';
        buffer[base_len+1] = '\0';
        base_len += 1;
    }
    strcpy(buffer+base_len, path);
    return buffer;
}

bool read_line(vec_char_t* line, FILE* f) {
    line->length = 0;
    while (!feof(f) && !ferror(f)) {
        int c = fgetc(f);

        if (c == EOF || c == '\n')
            vec_char_push(line, '\0');

        if (c == EOF)
            return false;

        if (c == '\n')
            return true;

        vec_char_push(line, (char)c);
    }
}
