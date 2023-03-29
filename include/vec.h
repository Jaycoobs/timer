/*
 * Generic vector header I wrote early 2022.
 *
 * Jacob Acosta
 */

#include <stdlib.h>

// ============================================================================
//                          DEFINE CHECK / DEFAULTS
// ============================================================================

#ifndef VEC_TYPE
#error "You must decleare VEC_TYPE!"
#endif

#ifndef VEC_PREFIX
#define VEC_PREFIX vec
#endif

// ============================================================================
//                                 MACROS
// ============================================================================

#define CONCAT(a, b, c) a ## _ ## b ## _ ## c
#define EXPAND_CONCAT(a, b, c) CONCAT(a, b, c)

#define VEC_MAKE_STR(x)  EXPAND_CONCAT(VEC_PREFIX, VEC_TYPE, x)

#define VEC             VEC_MAKE_STR(t)

#define VEC_INIT        VEC_MAKE_STR(init)
#define VEC_DELETE      VEC_MAKE_STR(delete)
#define VEC_ALLOC       VEC_MAKE_STR(alloc)
#define VEC_PUSH        VEC_MAKE_STR(push)

// ============================================================================
//                                DECLARATIONS
// ============================================================================

typedef struct {
    VEC_TYPE* data;
    size_t length;
    size_t capacity;
} VEC;

void VEC_INIT(VEC* vec);
void VEC_DELETE(VEC* vec);
VEC_TYPE* VEC_ALLOC(VEC* vec);
void VEC_PUSH(VEC* vec, VEC_TYPE a);

// ============================================================================
//                                DEFINITIONS
// ============================================================================

#ifdef VEC_IMPL

void VEC_INIT(VEC* vec) {
    vec->data = (VEC_TYPE*) malloc(sizeof(VEC_TYPE));
    vec->length = 0;
    vec->capacity = 1;
}

void VEC_DELETE(VEC* vec) {
    if (vec->data)
        free(vec->data);
    vec->data = NULL;
    vec->length = 0;
    vec->capacity = 0;
}

VEC_TYPE* VEC_ALLOC(VEC* vec) {
    // Increase the length
    vec->length += 1;

    // If the data doesn't fit, then resize.
    if (vec->length > vec->capacity) {
        vec->capacity *= 2;
        vec->data = (VEC_TYPE*) realloc(vec->data, vec->capacity * sizeof(VEC_TYPE));
    }

    // Return a pointer to the last element in the vector
    return &((VEC_TYPE*)vec->data)[vec->length - 1];
}

void VEC_PUSH(VEC* vec, VEC_TYPE a) {
    VEC_TYPE* p = VEC_ALLOC(vec);
    *p = a;
}

#endif

#undef CONCAT
#undef EXPAND_CONCAT

#undef VEC_MAKE_STR

#undef VEC
#undef VEC_INIT
#undef VEC_DELETE
#undef VEC_PUSH
