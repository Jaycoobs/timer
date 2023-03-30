#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdbool.h>

#include "vecs.h"

char* path_join(char* base, char* path);

bool read_line(vec_char_t* line, FILE* f);

#endif // UTIL_H
