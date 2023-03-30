#ifndef DURATION_H
#define DURATION_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    bool minus;
    bool present;
    struct timespec value;
} duration_t;

void duration_sub(duration_t* d, duration_t* a, duration_t* b);

void duration_parse(duration_t* d, char* s);

void duration_print(char* buffer, size_t buffer_length, duration_t* d, bool sign, bool full);

#endif // DURATION_H
