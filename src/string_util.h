#pragma once
#include <stdbool.h>
#include <stdarg.h>

bool is_space(char c);
bool is_digit(char c);
bool is_letter(char c);
bool is_opening_parenthesis(const char *c);
bool is_closing_parenthesis(const char *c);
bool is_delimiter(const char *c);
bool begins_with(const char *prefix, const char *str);
size_t str_split(char *str, char **out_strs, size_t num_delimiters, ...);
size_t get_line_of_string(const char *string, size_t line_index, char **out_start);
char *skip_ansi(const char *str);
char *strip(char *str);
const char *first_char(const char* string);
char to_lower(char c);
char to_upper(char c);
