#ifndef VECS_H
#define VECS_H

#include "duration.h"

#define VEC_TYPE duration_t
#include "vec.h"
#undef VEC_TYPE

#define VEC_TYPE size_t
#include "vec.h"
#undef VEC_TYPE

#define VEC_TYPE char
#include "vec.h"
#undef VEC_TYPE

#define VEC_TYPE vec_char_t
#include "vec.h"
#undef VEC_TYPE

#endif // VECS_H
