#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "string_util.h"
#include "table.h"
#include "printing.h"
#include "constraint.h"
#include "alloc_wrappers.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define STRBUILDER_STARTSIZE 5

void add_cell_internal(Table *table, char *text, bool needs_free)
{
    assert(table != NULL);
    assert(table->curr_col < MAX_COLS);
    struct Cell *cell = &table->curr_row->cells[table->curr_col];
    assert(!cell->is_set);

    cell->is_set = true;
    cell->text_needs_free = needs_free;
    cell->text = text;

    if (table->curr_col >= table->num_cols)
    {
        table->num_cols = table->curr_col + 1;
    }

    while (table->curr_col != MAX_COLS && table->curr_row->cells[table->curr_col].is_set)
    {
        table->curr_col++;
    }
}

void override_alignment_internal(struct Cell *cell, TextAlignment alignment)
{
    cell->align = alignment;
    cell->override_align = true;
}

struct Row *malloc_row(size_t y)
{
    struct Row *res = calloc_wrapper(1, sizeof(struct Row));
    if (res == NULL) return NULL;
    for (size_t i = 0; i < MAX_COLS; i++)
    {
        res->cells[i] = (struct Cell){
            .is_set                = false,
            .x                     = i,
            .y                     = y,
            .parent                = NULL,
            .text                  = NULL,
            .override_align        = false,
            .override_border_left  = false,
            .override_border_above = false,
            .span_x                = 1,
            .span_y                = 1,
            .text_needs_free       = false,
            .zeros_needed          = 0,
            .zero_position         = 0,
            .dot_needed            = false
        };
    }
    return res;
}

struct Row *append_row(Table *table)
{
    struct Row *row = table->curr_row;
    while (row->next_row != NULL)
    {
        row = row->next_row;
    }
    row->next_row = malloc_row(table->num_rows);
    table->num_rows++;
    return row->next_row;
}

struct Row *get_row(Table *table, size_t index)
{
    struct Row *row = table->first_row;
    while (index-- > 0 && row != NULL) row = row->next_row;
    return row;
}

struct Cell *get_curr_cell(Table *table)
{
    return &table->curr_row->cells[table->curr_col];
}

void free_row(Table *table, struct Row *row)
{
    for (size_t i = 0; i < table->num_cols; i++)
    {
        if (row->cells[i].text_needs_free)
        {
            free(row->cells[i].text);
        }
    }
    free(row);
}

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ User-functions ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

/*
Returns: A new table with a single, empty row.
*/
Table get_empty_table()
{
    struct Row *first_row = malloc_row(0);
    return (Table){
        .num_cols             = 0,
        .curr_col             = 0,
        .first_row            = first_row,
        .curr_row             = first_row,
        .num_rows             = 1,
        .alignments           = { ALIGN_LEFT },
        .borders_left         = { BORDER_NONE },
        .border_left_counters = { 0 }
    };
}

/*
Summary: Prints table to stdout
*/
void print_table(Table *table)
{
    fprint_table_internal(table, stdout);
}

void fprint_table(Table *table, FILE *stream)
{
    fprint_table_internal(table, stream);
}

/*
Summary: Frees all rows and content strings in cells created by add_cell_fmt.
    Don't use the table any more, get a new one!
*/
void free_table(Table *table)
{
    struct Row *row = table->first_row;
    while (row != NULL)
    {
        struct Row *next_row = row->next_row;
        free_row(table, row);
        row = next_row;
    }
}

void set_position(Table *table, size_t x, size_t y)
{
    assert(table != NULL);
    assert(x < MAX_COLS);

    table->curr_col = x;
    if (y < table->num_rows)
    {
        table->curr_row = get_row(table, y);
    }
    else
    {
        while (y >= table->num_rows)
        {
            table->curr_row = append_row(table);
        }
    }
}

/*
Summary: Next inserted cell will be inserted at first unset column of next row.
*/
void next_row(Table *table)
{
    assert(table != NULL);

    // Extend linked list if necessary
    table->curr_col = 0;
    if (table->curr_row->next_row == NULL)
    {
        table->curr_row = append_row(table);
    }
    else
    {
        table->curr_row = table->curr_row->next_row;
        while (table->curr_col < MAX_COLS && table->curr_row->cells[table->curr_col].is_set)
        {
            table->curr_col++;
        }
    }
}

/*
Summary: Adds next cell. Buffer is not copied. print_table will access it.
    Ensure that lifetime of buffer outlasts last call of print_table!
*/
void add_cell(Table *table, char *text)
{
    add_cell_internal(table, text, false);
}

/*
Summary: Same as add_cell, but frees buffer on reset
*/
void add_cell_gc(Table *table, char *text)
{
    add_cell_internal(table, text, true);
}

void add_empty_cell(Table *table)
{
    add_cell_internal(table, NULL, false);
}

/*
Summary: Adds next cell and maintains buffer of text.
    Use add_cell to save memory if you maintain a content string yourself.
*/
void add_cell_fmt(Table *table, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    add_cell_vfmt(table, fmt, args);
    va_end(args);
}

void add_cell_vfmt(Table *table, char *fmt, va_list args)
{
    StringBuilder builder = strbuilder_create(STRBUILDER_STARTSIZE);
    vstrbuilder_append(&builder, fmt, args);
    add_cell_internal(table, builder.buffer, true);
}

/*
Summary: Puts contents of memory-contiguous 2D array into table cell by cell.
    Strings are not copied. Ensure that lifetime of array outlasts last call of print_table.
    Position of next insertion is first cell in next row.
*/
void add_cells_from_array(Table *table, size_t width, size_t height, char **array)
{
    if (table->curr_col + width > MAX_COLS) return;
    for (size_t i = 0; i < height; i++)
    {
        for (size_t j = 0; j < width; j++)
        {
            add_cell(table, *(array + i * width + j));
        }
        next_row(table);
    }
}

/*
Summary: Sets default alignment of columns
*/
void set_default_alignments(Table *table, size_t num_alignments, TextAlignment *alignments)
{
    assert(table != NULL);
    assert(num_alignments < MAX_COLS);

    for (size_t i = 0; i < num_alignments; i++)
    {
        table->alignments[i] = alignments[i];
    }
}

/*
Summary: Overrides alignment of current cell
*/
void override_alignment(Table *table, TextAlignment alignment)
{
    assert(table != NULL);
    override_alignment_internal(get_curr_cell(table), alignment);
}

/*
Summary: Overrides alignment of all cells in current row
*/
void override_alignment_of_row(Table *table, TextAlignment alignment)
{
    assert(table != NULL);
    for (size_t i = 0; i < MAX_COLS; i++)
    {
        override_alignment_internal(&table->curr_row->cells[i], alignment);
    }
}

void set_hline(Table *table, BorderStyle style)
{
    assert(table != NULL);

    if (table->curr_row->border_above != BORDER_NONE)
    {
        table->curr_row->border_above_counter--;
    }
    if (style != BORDER_NONE)
    {
        table->curr_row->border_above_counter++;
    }
    table->curr_row->border_above = style;
}

void set_vline(Table *table, size_t index, BorderStyle style)
{
    assert(table != NULL);
    assert (index < MAX_COLS);

    if (table->num_cols <= index)
    {
        table->num_cols = index + 1;
    }
    if (table->borders_left[index] != BORDER_NONE)
    {
        table->border_left_counters[index]--;
    }
    if (style != BORDER_NONE)
    {
        table->border_left_counters[index]++;
    }

    table->borders_left[index] = style;
}

void make_boxed(Table *table, BorderStyle style)
{
    assert(table != NULL);

    set_position(table, 0, 0);
    set_vline(table, 0, style);
    set_hline(table, style);
    set_position(table, table->num_cols, table->num_rows - 1);
    set_vline(table, table->num_cols, style);
    set_hline(table, style);
}

void override_left_border(Table *table, BorderStyle style)
{
    assert(table != NULL);

    if (get_curr_cell(table)->override_border_left && get_curr_cell(table)->border_left != BORDER_NONE)
    {
        table->border_left_counters[table->curr_col]--;
    }
    if (style != BORDER_NONE)
    {
        table->border_left_counters[table->curr_col]++;
    }

    get_curr_cell(table)->border_left = style;
    get_curr_cell(table)->override_border_left = true;
}

void override_above_border(Table *table, BorderStyle style)
{
    assert(table != NULL);

    if (get_curr_cell(table)->override_border_above && get_curr_cell(table)->border_above != BORDER_NONE)
    {
        table->curr_row->border_above_counter--;
    }
    if (style != BORDER_NONE)
    {
        table->curr_row->border_above_counter++;
    }

    get_curr_cell(table)->border_above = style;
    get_curr_cell(table)->override_border_above = true;
}

/*
Summary: Changes span of current cell
    When spanning cell clashes with already set cells, the span will be truncated
*/
void set_span(Table *table, size_t span_x, size_t span_y)
{
    assert(table != NULL);
    assert(span_x != 0);
    assert(span_y != 0);
    assert(table->curr_col + span_x <= MAX_COLS);

    struct Cell *cell = &table->curr_row->cells[table->curr_col];
    cell->span_x = span_x;
    cell->span_y = span_y;
    table->num_cols = MAX(table->curr_col + span_x, table->num_cols);

    // Inserts rows and sets child cells
    struct Row *row = table->curr_row;
    for (size_t i = 0; i < span_y; i++)
    {
        for (size_t j = 0; j < span_x; j++)
        {
            if (i == 0 && j == 0) continue;
            struct Cell *child = &row->cells[cell->x + j];

            if (!child->is_set)
            {
                child->is_set = true;
                child->parent = cell;
                
                if (j != 0)
                {
                    child->border_left = BORDER_NONE;
                    child->override_border_left = true;
                }
                if (i != 0)
                {
                    child->border_above = BORDER_NONE;
                    child->override_border_above = true;
                }
            }
            else
            {
                // Span clashes with already set cell, truncate it and finalize method
                cell->span_y = i;
                cell->span_x = j;
                return;
            }
        }

        if (i + 1 < span_y && row->next_row == NULL)
        {
            row = append_row(table);
        }
        else
        {
            row = row->next_row;
        }
    }
}

void set_all_vlines(Table *table, BorderStyle style)
{
    assert(table != NULL);

    size_t cc = table->curr_col;
    for (size_t i = 1; i < table->num_cols; i++)
    {
        set_vline(table, i, style);
    }
    table->curr_col = cc;
}
