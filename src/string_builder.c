#include <stdio.h>
#include <string.h>
#include "string_builder.h"
#include "vector.h"

StringBuilder strb_create()
{
    StringBuilder builder = vec_create(sizeof(char), 2);
    *(char*)vec_push_empty(&builder) = '\0';
    return builder;
}

// It is very important that heap_string was dynamically allocated
StringBuilder strb_from_heapstring(char *heap_string)
{
    size_t len = strlen(heap_string) + 1;
    StringBuilder builder = (Vector){
        .elem_size   = sizeof(char),
        .elem_count  = len,
        .buffer_size = len,
        .buffer      = heap_string
    };
    return builder;
}

void strb_clear(StringBuilder *builder)
{
    vec_clear(builder);
    *(char*)vec_push_empty(builder) = '\0';
}

void strb_append(StringBuilder *builder, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vstrb_append(builder, fmt, args);
    va_end(args);
}

void strb_append_char(StringBuilder *builder, char c)
{
    VEC_SET_ELEM(builder, char, builder->elem_count - 1, c);
    VEC_PUSH_ELEM(builder, char, '\0');
}

void vstrb_append(StringBuilder *builder, const char *fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    size_t appended_length = vsnprintf(vec_get(builder, vec_count(builder) - 1),
        builder->buffer_size - builder->elem_count + 1,
        fmt, args);

    // Check whether previous vsnprintf truncated the string...
    if (builder->buffer_size < vec_count(builder) + appended_length)
    {
        // ...if this is the case, grow the vector and call vsnprintf again
        vec_ensure_size(builder, vec_count(builder) + appended_length);
        vsnprintf(vec_get(builder, vec_count(builder) - 1), // -1 to overwrite old \0
            builder->buffer_size - builder->elem_count + 1,
            fmt, args_copy);
    }

    builder->elem_count += appended_length;
    va_end(args_copy);
}

// Lifetime of string is tied to lifetime of StringBuilder!
char *strb_to_str(const StringBuilder *builder)
{
    return builder->buffer;
}

void strb_destroy(StringBuilder *builder)
{
    vec_destroy(builder);
}
