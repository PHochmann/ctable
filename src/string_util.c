#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "string_util.h"

#define ESC_START  27
#define ESC_END   109

bool is_digit(char c)
{
    return (c >= '0' && c <= '9') || c == '.';
}

bool begins_with(char *prefix, char *str)
{
    size_t prefix_length = strlen(prefix);
    size_t string_length = strlen(str);
    if (prefix_length > string_length) return false;
    return strncmp(prefix, str, prefix_length) == 0;
}

StringIterator get_iterator(char *string)
{
    return (StringIterator){ .string = string, .index = 0 };
}

char get_next_char(StringIterator *iterator)
{
    if (iterator->string[iterator->index] == ESC_START)
    {
        while (iterator->string[iterator->index] != ESC_END)
        {
            iterator->index++;
        }
        iterator->index++;
    }
    return iterator->string[iterator->index++];
}

/*
Summary: Calculates length of string displayed in console,
    i.e. reads until \0 or \n and omits ANSI-escaped color sequences
    Todo: Consider \t and other special chars
*/
size_t console_strlen(char *str)
{
    if (str == NULL) return 0;
    StringIterator iterator = get_iterator(str);
    size_t res = 0;
    while (true)
    {
        char curr = get_next_char(&iterator);
        if (curr == '\0' || curr == '\n') break;
        res++;
    }
    return res;
}

// Stringbuilder follows ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

Vector strbuilder_create(size_t start_size)
{
    Vector builder = vec_create(sizeof(char), start_size);
    *(char*)vec_push_empty(&builder) = '\0';
    return builder;
}

void strbuilder_reset(Vector *builder)
{
    vec_reset(builder),
    *(char*)vec_push_empty(builder) = '\0';
}

void strbuilder_append(Vector *builder, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vstrbuilder_append(builder, fmt, args);
    va_end(args);
}

void strbuilder_append_char(Vector *builder, char c)
{
    VEC_SET_ELEM(builder, char, builder->elem_count - 1, c);
    VEC_PUSH_ELEM(builder, char, '\0');
}

void strbuilder_reverse(Vector *builder)
{
    for (size_t i = 0; i < (vec_count(builder) - 1) / 2; i++)
    {
        char temp = VEC_GET_ELEM(builder, char, i);
        size_t partner = vec_count(builder) - 2 - i;
        VEC_SET_ELEM(builder, char, i, VEC_GET_ELEM(builder, char, partner));
        VEC_SET_ELEM(builder, char, partner, temp);
    }
}

void vstrbuilder_append(Vector *builder, char *fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    size_t appended_length = vsnprintf(vec_get(builder, vec_count(builder) - 1),
        builder->buffer_size - builder->elem_count + 1,
        fmt, args);

    if (vec_ensure_size(builder, vec_count(builder) + appended_length))
    {
        vsnprintf(vec_get(builder, vec_count(builder) - 1),
            builder->buffer_size - builder->elem_count + 1,
            fmt, args_copy);
    }
    builder->elem_count += appended_length;
    va_end(args_copy);
}
