#ifndef RUN_H
#define RUN_H

#include <stdlib.h>
#include <stdbool.h>

#include "vecs.h"
#include "duration.h"

typedef struct {
    bool present;
    bool segments;
    vec_duration_t_t splits;
} run_t;

void run_empty(run_t* r, size_t split_count);

bool run_load(run_t* r, char* path, bool segments_only);

void run_get_split_duration(duration_t* d, run_t* r, size_t i);

bool run_write(run_t* r, vec_vec_char_t_t* names, char* path);

void run_delete(run_t* r);

#endif // RUN_H
