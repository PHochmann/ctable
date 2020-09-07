#pragma once
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>

// May change the buffer location
#define VEC_PUSH_ELEM(vec, type, expr) (*(type*)vec_push_empty(vec)) = expr
#define VEC_SET_ELEM(vec, type, index, expr) (*(type*)vec_get(vec, index)) = expr
// The following macros will dereference a NULL pointer on invalid index or empty vector
#define VEC_GET_ELEM(vec, type, index) (*(type*)vec_get(vec, index))
#define VEC_POP_ELEM(vec, type) *(type*)vec_pop(vec)
#define VEC_PEEK_ELEM(vec, type) *(type*)vec_peek(vec)

/*
 * Never save pointers when there are elements
 * inserted into the buffer! It can be realloc'ed!
 */

typedef struct
{
    size_t elem_size;
    size_t elem_count;
    size_t buffer_size;
    void *buffer;
} Vector;

// Buffer handling
void vec_trim(Vector *vec);
bool vec_ensure_size(Vector *vec, size_t needed_size);
Vector vec_create(size_t elem_size, size_t start_size);
void vec_reset(Vector *vec);
void vec_destroy(Vector *vec);

// Insertion
void *vec_push(Vector *vec, void *elem);
void *vec_push_many(Vector *vec, size_t num, void *elem);
void *vec_push_empty(Vector *vec);

// Retrieval
void *vec_get(Vector *vec, size_t index);
void *vec_pop(Vector *vec);
void *vec_peek(Vector *vec);

// Attributes
size_t vec_count(Vector *vec);
