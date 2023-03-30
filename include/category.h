#ifndef CATEGORY_H
#define CATEGORY_H

#include <stdio.h>

#include "vecs.h"
#include "run.h"

typedef struct {
    char* name;
    vec_size_t_t attempts;
    vec_vec_char_t_t names;
    run_t pb;
    run_t golds;
} category_t;

bool names_load(vec_vec_char_t_t* names, char* path);

void names_delete(vec_vec_char_t_t* names);

bool attempts_load(vec_size_t_t* attempts, char* path);

bool attempts_write(vec_size_t_t* attempts, vec_vec_char_t_t* names, char* path);

void category_delete(category_t* category);

bool category_load(category_t* category, char* path);

void category_write(category_t* category, char* path);

#endif // CATEGORY_H
