#ifndef TIME_FUNCS_H
#define TIME_FUNCS_H

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

void time_now(struct timespec* t);

void time_add(struct timespec* d, struct timespec* a, struct timespec* b);

bool time_sub(struct timespec* d, struct timespec* a, struct timespec* b);

void time_parse(struct timespec* d, char* s);

void time_print(char* buffer, size_t buffer_length, struct timespec* t, bool full);

#endif // TIME_FUNCS_H
