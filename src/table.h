#pragma once
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

// Can be changed without further modifications
#define TABLE_MAX_COLS 11

typedef enum
{
    BORDER_NONE,
    BORDER_SINGLE,
    BORDER_DOUBLE
} TableBorderStyle;

typedef enum
{
    H_ALIGN_LEFT,
    H_ALIGN_RIGHT,
    H_ALIGN_CENTER // Rounded to the left
} TableHAlign;

typedef enum
{
    V_ALIGN_TOP,
    V_ALIGN_BOTTOM,
    V_ALIGN_CENTER // Rounded to the top
} TableVAlign;

typedef struct Table Table;

// Data and printing
Table *get_empty_table();
void print_table(Table *table);
void fprint_table(Table *table, FILE *stream);
void free_table(Table *table);

// Control
void set_position(Table *table, size_t x, size_t y);
void next_row(Table *table);

// Cell insertion
void add_empty_cell(Table *table);
void add_cell(Table *table, const char *text);
void add_cells(Table *table, size_t num_cells, ...);
void add_cell_gc(Table *table, char *text);
void add_cell_fmt(Table *table, const char *fmt, ...);
void add_cell_vfmt(Table *table, const char *fmt, va_list args);
void add_cells_from_array(Table *table, size_t width, size_t height, const char **array);

// Settings
void set_default_alignments(Table *table, size_t num_alignments, const TableHAlign *hor_aligns, const TableVAlign *vert_aligns);
void override_vertical_alignment(Table *table, TableVAlign align);
void override_horizontal_alignment(Table *table, TableHAlign align);
void override_vertical_alignment_of_row(Table *table, TableVAlign align);
void override_horizontal_alignment_of_row(Table *table, TableHAlign align);
void set_hline(Table *table, TableBorderStyle style);
void set_vline(Table *table, size_t index, TableBorderStyle style);
void make_boxed(Table *table, TableBorderStyle style);
void set_all_vlines(Table *table, TableBorderStyle style);
void override_left_border(Table *table, TableBorderStyle style);
void override_above_border(Table *table, TableBorderStyle style);
void set_span(Table *table, size_t span_x, size_t span_y);
