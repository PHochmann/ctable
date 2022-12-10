#pragma once
#include <stdlib.h>
#include <stdarg.h>
#include "vector.h"

// A StringBuilder is just a Vector of chars
typedef Vector StringBuilder;

StringBuilder strb_create();
StringBuilder strb_from_heapstring(char *heap_string);
void strb_clear(StringBuilder *builder);
void strb_append(StringBuilder *builder, const char *fmt, ...);
void vstrb_append(StringBuilder *builder, const char *fmt, va_list args);
void strb_append_char(StringBuilder *builder, char c);
char *strb_to_str(const StringBuilder *builder);
void strb_destroy(StringBuilder *builder);
