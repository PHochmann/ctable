#pragma once
#include <stdbool.h>
#include "vector.h"

// A StringBuilder is just a Vector of chars
typedef Vector StringBuilder;

typedef struct
{
    char *string;
    size_t index;
} StringIterator;

bool is_digit(char c);
bool begins_with(char *prefix, char *str);
StringIterator get_iterator(char *string);
char get_next_char(StringIterator *iterator);
size_t console_strlen(char *str);

// Stringbuilder follows ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

Vector strbuilder_create(size_t start_size);
void strbuilder_reset(Vector *builder);
void strbuilder_append(Vector *builder, char *fmt, ...);
void strbuilder_append_char(Vector *builder, char c);
void strbuilder_reverse(Vector *builder);
void vstrbuilder_append(Vector *builder, char *fmt, va_list args);
