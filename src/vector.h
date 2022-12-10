#pragma once
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>

// May change the buffer location
#define VEC_PUSH_ELEM(vec, type, expr) ((*(type*)vec_push_empty(vec)) = (expr))
#define VEC_SET_ELEM(vec, type, index, expr) ((*(type*)vec_get(vec, index)) = (expr))

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
Vector vec_create(size_t elem_size, size_t start_size);
void vec_clear(Vector *vec);
void vec_destroy(Vector *vec);
void vec_trim(Vector *vec);
void vec_ensure_size(Vector *vec, size_t needed_size);

// Insertion
void *vec_push(Vector *vec, void *elem);
void *vec_push_many(Vector *vec, size_t num, void *elems);
void *vec_push_empty(Vector *vec);

// Retrieval
void *vec_get(const Vector *vec, size_t index);
void *vec_pop(Vector *vec);
void *vec_peek(const Vector *vec);

// Attributes
size_t vec_count(const Vector *vec);
