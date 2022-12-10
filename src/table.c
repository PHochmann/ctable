#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "string_builder.h"
#include "string_util.h"
#include "table.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

struct Cell
{
    char *text;         // Actual content to be displayed
    size_t text_height; // Number of lines
    size_t text_width;  // Maximum width of lines

    // Settings
    TableHAlign h_align;           // Non default, how to place text in col width
    TableVAlign v_align;           // Non default, how to place text in col width
    TableBorderStyle border_left;  // Non-default border left
    TableBorderStyle border_above; // Non-default border above
    size_t span_x;                 // How many cols to span over
    size_t span_y;                 // How many rows to span over

    // Generated
    bool override_v_align;      // Default set for each col in table
    bool override_h_align;      // Default set for each col in table
    bool override_border_left;  // Default set for each col in table
    bool override_border_above; // Default set in row

    bool is_set;          // Indicates whether data is valid
    bool text_needs_free; // When set to true, text will be freed on free_table
    size_t x;             // Column position
    size_t y;             // Row position
    struct Cell *parent;  // Cell that spans into this cell
};

struct Row
{
    struct Cell cells[TABLE_MAX_COLS]; // All cells of this row from left to right
    struct Row *next_row;              // Pointer to next row or NULL if last row
    TableBorderStyle border_above;     // Default border above (can be overwritten in cell)
    int border_above_counter;       // Counts cells that override their border_above
};

struct Table
{
    size_t num_cols;                         // Number of columns (max. of num_cells over all rows)
    size_t num_rows;                         // Number of rows (length of linked list)
    struct Row *first_row;                   // Start of linked list to rows
    struct Row *curr_row;                    // Marker of row of next inserted cell
    size_t curr_col;                         // Marker of col of next inserted cell
    TableBorderStyle borders_left[TABLE_MAX_COLS]; // Default left border of cols
    TableHAlign h_aligns[TABLE_MAX_COLS];          // Default horizontal alignment of cols
    TableVAlign v_aligns[TABLE_MAX_COLS];          // Default vertical alignment of cols
    int border_left_counters[TABLE_MAX_COLS];   // Counts cells that override their border_left
};

// Represents a size contraint in one dimension imposed by a single cell
struct Constraint
{
    size_t from_index; // Inclusive
    size_t to_index;   // Exclusive
    size_t min;        // Needed size (i.e. minimum size needed)
};

static char *BORDER_MATRIX_SINGLE[] = {
    "┌", "┬", "┐",
    "├", "┼", "┤",
    "└", "┴", "┘",
    "─", "│", " ",
};

static char *BORDER_MATRIX_DOUBLE[] = {
    "╔", "╦", "╗",
    "╠", "╬", "╣",
    "╚", "╩", "╝",
    "═", "║", " ",
};

static size_t HLINE_INDEX = 9;
static size_t VLINE_INDEX = 10;

// Index encodes whether a border intersects (0: no intersection, 1: intersection), clockwise
static size_t BORDER_LOOKUP[16] = { 11, 11, 11, 6, 11, 10, 0, 3, 11, 8, 9, 7, 2, 5, 1, 4 };

static struct Row *get_row(const Table *table, size_t index)
{
    struct Row *row = table->first_row;
    while (index-- > 0 && row != NULL) row = row->next_row;
    return row;
}

static TableHAlign get_h_align(TableHAlign default_h, const struct Cell *cell)
{
    if (cell->parent != NULL) return get_h_align(default_h, cell->parent);
    return (cell->override_h_align ? cell->h_align : default_h);
}

static TableVAlign get_v_align(TableVAlign default_v, const struct Cell *cell)
{
    if (cell->parent != NULL) return get_v_align(default_v, cell->parent);
    return (cell->override_v_align ? cell->v_align : default_v);
}

/*
Summary: Calculates length of string displayed in console,
    i.e. reads until \0 or \n and omits ANSI-escaped color sequences
    Todo: Consider \t and other special chars
*/
size_t console_strlen(const char *str)
{
    size_t res = 0;
    str = skip_ansi(str);
    while (*str != '\0' && *str != '\n')
    {
        str = skip_ansi(str + 1);
        res++;
    }
    return res;
}

// Counts number of \n in string
static size_t get_text_height(const char *str)
{
    if (str == NULL) return 0;
    size_t res = 1;
    size_t pos = 0;
    while (str[pos] != '\0')
    {
        if (str[pos] == '\n')
        {
            res++;
        }
        pos++;
    }
    return res;
}

static size_t get_text_width(const char *str)
{
    size_t res = 0;
    size_t height = get_text_height(str);
    for (size_t i = 0; i < height; i++)
    {
        char *line;
        get_line_of_string(str, i, &line);
        size_t line_width = console_strlen(line);
        if (line_width > res) res = line_width;
    }
    return res;
}

static void print_repeated(const char *string, size_t times, FILE *stream)
{
    for (size_t i = 0; i < times; i++)
    {
        fprintf(stream, "%s", string);
    }
}

static void print_text(const struct Cell *cell,
    TableHAlign default_h,
    TableVAlign default_v,
    size_t line_index,
    int total_width,
    size_t total_height,
    FILE *stream)
{
    if (cell->parent != NULL)
    {
        cell = cell->parent;
    }

    // First, select actual line that needs to be printed based on vertical alignment
    int actual_line = 0;
    switch (get_v_align(default_v, cell))
    {
        case V_ALIGN_TOP:
            actual_line = line_index;
            break;
        case V_ALIGN_CENTER:
            actual_line = line_index - (total_height - cell->text_height) / 2;
            break;
        case V_ALIGN_BOTTOM:
            actual_line = line_index - (total_height - cell->text_height);
    }

    char *string = NULL;
    int bytes = 0;
    
    if (actual_line >= 0)
    {
        bytes = get_line_of_string(cell->text, actual_line, &string);
    }

    if (string == NULL)
    {
        fprintf(stream, "%*s", total_width, "");
        return;
    }

    // Lengths passed to printf are including color codes
    // We need to adjust the total length to include them
    int string_length = console_strlen(string);
    int adjusted_total_len = total_width + bytes - string_length;

    switch (get_h_align(default_h, cell))
    {
        case H_ALIGN_LEFT:
        {
            fprintf(stream, "%-*.*s", adjusted_total_len, bytes, string);
            break;
        }
        case H_ALIGN_RIGHT:
        {
            fprintf(stream, "%*.*s", adjusted_total_len, bytes, string);
            break;
        }
        case H_ALIGN_CENTER:
        {
            int padding = (total_width - string_length) / 2;
            fprintf(stream, "%*s%.*s%*s", padding, "", bytes, string,
                (total_width - string_length) % 2 == 0 ? padding : padding + 1, "");
            break;
        }
    }
}

static size_t get_total_width(const Table *table, const size_t *col_widths, struct Cell *cell)
{
    if (cell->parent != NULL) cell = cell->parent;

    size_t sum = 0;
    for (size_t i = 0; i < cell->span_x; i++)
    {
        if (i != 0
            && table->border_left_counters[cell->x + i] > 0)
        {
            sum++;
        }
        sum += col_widths[cell->x + i];
    }
    return sum;
}

static size_t get_total_height(const Table *table, const size_t *row_heights, struct Cell *cell)
{
    if (cell->parent != NULL) cell = cell->parent;

    size_t sum = 0;
    struct Row *curr_row = get_row(table, cell->y);
    for (size_t i = 0; i < cell->span_y; i++)
    {
        if (i != 0 && curr_row->border_above_counter > 0) sum++;
        sum += row_heights[cell->y + i];
        curr_row = curr_row->next_row;
    }
    return sum;
}

static size_t get_span_x(struct Cell *cell)
{
    if (cell->parent != NULL)
    {
        return cell->parent->span_x;
    }
    else
    {
        return cell->span_x;
    }
}

static TableBorderStyle get_border_above(TableBorderStyle default_style, const struct Cell *cell)
{
    if (cell->override_border_above)
    {
        return cell->border_above;
    }
    else
    {
        return default_style;
    }
}

static TableBorderStyle get_border_left(TableBorderStyle default_style, const struct Cell *cell)
{
    if (cell->override_border_left)
    {
        return cell->border_left;
    }
    else
    {
        return default_style;
    }
}

static void count_styles(TableBorderStyle style, size_t *out_num_single, size_t *out_num_double)
{
    if (style == BORDER_SINGLE) (*out_num_single)++;
    if (style == BORDER_DOUBLE) (*out_num_double)++;
}

static void print_intersection_char(
    TableBorderStyle default_right_border_left,
    TableBorderStyle default_below_border_above,
    struct Cell *right_above,
    struct Cell *left_below,
    struct Cell *right_below,
    FILE *stream)
{
    size_t num_single = 0;
    size_t num_double = 0;

    TableBorderStyle above = BORDER_NONE;
    if (right_above != NULL)
    {
        above = get_border_left(default_right_border_left, right_above);
        count_styles(above, &num_single, &num_double);
    }

    TableBorderStyle right = BORDER_NONE;
    if (right_below != NULL)
    {
        right = get_border_above(default_below_border_above, right_below);
        count_styles(right, &num_single, &num_double);
    }

    TableBorderStyle below = BORDER_NONE;
    if (right_below != NULL)
    {
        below = get_border_left(default_right_border_left, right_below);
        count_styles(below, &num_single, &num_double);
    }

    TableBorderStyle left  = BORDER_NONE;
    if (left_below != NULL)
    {
        left = get_border_above(default_below_border_above, left_below);
        count_styles(left, &num_single, &num_double);
    }

    size_t index = 0;
    if (above != BORDER_NONE)
    {
        index |= (1 << 0);
    }
    if (right != BORDER_NONE)
    {
        index |= (1 << 1);
    }
    if (below != BORDER_NONE)
    {
        index |= (1 << 2);
    }
    if (left != BORDER_NONE)
    {
        index |= (1 << 3);
    }

    if (num_double > num_single)
    {
        fprintf(stream, "%s", BORDER_MATRIX_DOUBLE[BORDER_LOOKUP[index]]);
    }
    else
    {
        fprintf(stream, "%s", BORDER_MATRIX_SINGLE[BORDER_LOOKUP[index]]);
    }
}

// Above row can be NULL, below row must not be NULL!
static void print_row_border(Table *table,
    struct Row *above_row,
    struct Row *below_row,
    size_t *line_indices,
    size_t *col_widths,
    size_t *row_heights,
    FILE *stream)
{
    for (size_t i = 0; i < table->num_cols; i++)
    {
        // Print vline-hline intersection
        if (table->border_left_counters[i] > 0)
        {
            print_intersection_char(table->borders_left[i],
                below_row->border_above,
                above_row != NULL ? &above_row->cells[i] : NULL,
                i > 0 ? &below_row->cells[i - 1] : NULL,
                &below_row->cells[i], stream);
        }

        // Print hline in between intersections (or content when cell has span_y > 1)
        if (below_row->cells[i].parent == NULL || below_row->cells[i].parent->y == below_row->cells[i].y)
        {
            switch (get_border_above(below_row->border_above, &below_row->cells[i]))
            {
                case BORDER_SINGLE:
                    print_repeated(BORDER_MATRIX_SINGLE[HLINE_INDEX], col_widths[i], stream);
                    break;
                case BORDER_DOUBLE:
                    print_repeated(BORDER_MATRIX_DOUBLE[HLINE_INDEX], col_widths[i], stream);
                    break;
                case BORDER_NONE:
                    print_repeated(" ", col_widths[i], stream);
            }
        }
        else
        {
            struct Cell *parent = below_row->cells[i].parent;
            print_text(parent,
                table->h_aligns[i],
                table->v_aligns[i],
                line_indices[i],
                get_total_width(table, col_widths, parent),
                get_total_height(table, row_heights, parent),
                stream);
            line_indices[i]++;
            i += parent->span_x - 1;
        }
    }
    fprintf(stream, "\n");
}

static void override_superfluous_lines(Table *table, size_t last_col_width, size_t last_row_height)
{
    // Special cases: If last row/col is empty, delete all vlines/hlines in it
    if (last_col_width == 0)
    {
        struct Row *row = table->first_row;
        table->curr_col = table->num_cols - 1;
        while (row != NULL)
        {
            table->curr_row = row;
            override_above_border(table, BORDER_NONE);
            row = row->next_row;
        }
    }
    else
    {
        struct Row *row = table->first_row;
        while (row != NULL)
        {
            table->curr_row = row;
            row = row->next_row;
        }
    }
    // table->curr_row is last row now
    if (last_row_height == 0)
    {
        for (size_t i = 0; i < table->num_cols; i++)
        {
            table->curr_col = i;
            override_left_border(table, BORDER_NONE);
        }
    }
}

static void add_text_cell(Table *table, char *text, bool needs_free)
{
    struct Cell *cell = &table->curr_row->cells[table->curr_col];
    cell->is_set = true;
    cell->text_needs_free = needs_free;
    cell->text = text;
    cell->text_height = get_text_height(text);
    cell->text_width = get_text_width(text);

    if (table->curr_col >= table->num_cols)
    {
        table->num_cols = table->curr_col + 1;
    }

    while (table->curr_col != TABLE_MAX_COLS && table->curr_row->cells[table->curr_col].is_set)
    {
        table->curr_col++;
    }
}

static void override_h_align_internal(struct Cell *cell, TableHAlign h_align)
{
    cell->h_align = h_align;
    cell->override_h_align = true;
}

static void override_v_align_internal(struct Cell *cell, TableVAlign v_align)
{
    cell->v_align = v_align;
    cell->override_v_align = true;
}

static struct Row *malloc_row(size_t y)
{
    struct Row *res = calloc(1, sizeof(struct Row));
    if (res == NULL) return NULL;
    for (size_t i = 0; i < TABLE_MAX_COLS; i++)
    {
        res->cells[i] = (struct Cell){
            .is_set                = false,
            .x                     = i,
            .y                     = y,
            .parent                = NULL,
            .text                  = NULL,
            .override_h_align      = false,
            .override_v_align      = false,
            .override_border_left  = false,
            .override_border_above = false,
            .span_x                = 1,
            .span_y                = 1,
            .text_needs_free       = false
        };
    }
    return res;
}

static struct Row *append_row(Table *table)
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

static struct Cell *get_curr_cell(Table *table)
{
    return &table->curr_row->cells[table->curr_col];
}

static void free_row(Table *table, struct Row *row)
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

static size_t needed_to_satisfy(struct Constraint *constr, size_t *vars)
{
    size_t sum = 0;
    for (size_t i = constr->from_index; i < constr->to_index; i++)
    {
        sum += vars[i];
    }
    return sum < constr->min ? constr->min - sum : 0;
}

// Make sure to zero out result before
static void satisfy_constraints(size_t num_constrs, struct Constraint *constrs, size_t *result)
{
    // Current heuristic: Do simple cells before...
    for (size_t i = 0; i < num_constrs; i++)
    {
        if (constrs[i].to_index - constrs[i].from_index == 1)
        {
            if (result[constrs[i].from_index] < constrs[i].min)
            {
                result[constrs[i].from_index] = constrs[i].min;
            }
        }
    }
    // ...then split remaining amount evenly
    for (size_t i = 0; i < num_constrs; i++)
    {
        size_t needed = needed_to_satisfy(&constrs[i], result);
        if (needed > 0)
        {
            size_t length = constrs[i].to_index - constrs[i].from_index;
            if (length > 0)
            {
                size_t adjustment = needed / length;
                for (size_t j = 0; j < length; j++)
                {
                    result[constrs[i].from_index + j] += adjustment;
                    if (needed % length > j) result[constrs[i].from_index + j]++;
                }
            }
        }
    }
}

void get_dimensions(Table *table, size_t *out_col_widths, size_t *out_row_heights)
{
    struct Constraint *constrs = malloc(table->num_cols * table->num_rows * sizeof(struct Constraint));
    // Satisfy constraints of width
    struct Row *curr_row = table->first_row;
    size_t index = 0;
    while (curr_row != NULL)
    {
        for (size_t i = 0; i < table->num_cols; i++)
        {
            // Build constraints for set parent cells
            if (curr_row->cells[i].is_set && curr_row->cells[i].parent == NULL)
            {
                size_t min = curr_row->cells[i].text_width;

                // Constraint can be weakened when vlines are in between
                for (size_t j = i + 1; j < i + curr_row->cells[i].span_x; j++)
                {
                    if (min == 0) break;
                    if (table->border_left_counters[j] > 0) min--;
                }

                constrs[index] = (struct Constraint){
                    .min        = min,
                    .from_index = i,
                    .to_index   = i + curr_row->cells[i].span_x
                };
                index++;
            }
        }
        curr_row = curr_row->next_row;
    }
    for (size_t i = 0; i < table->num_cols; i++) out_col_widths[i] = 0;
    satisfy_constraints(index, constrs, out_col_widths);

    // Satisfy constraints of height
    curr_row = table->first_row;
    index = 0;
    size_t row_index = 0;
    while (curr_row != NULL)
    {
        for (size_t i = 0; i < table->num_cols; i++)
        {
            if (curr_row->cells[i].is_set && curr_row->cells[i].parent == NULL)
            {
                size_t min = curr_row->cells[i].text_height;
                struct Row *row_checked_for_hline = curr_row->next_row;

                // Constraint can be weakened when hlines are in between
                for (size_t j = i + 1; j < i + curr_row->cells[i].span_y; j++)
                {
                    if (row_checked_for_hline == NULL || min == 0) break;
                    if (row_checked_for_hline->border_above_counter > 0) min--;
                    row_checked_for_hline = row_checked_for_hline->next_row;
                }

                constrs[index] = (struct Constraint){
                    .min        = min,
                    .from_index = row_index,
                    .to_index   = row_index + curr_row->cells[i].span_y
                };
                index++;
            }
        }
        curr_row = curr_row->next_row;
        row_index++;
    }
    for (size_t i = 0; i < table->num_rows; i++) out_row_heights[i] = 0;
    satisfy_constraints(index, constrs, out_row_heights);
    free(constrs);
}

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ User-functions ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

/*
Returns: A new table with a single, empty row.
*/
Table *get_empty_table()
{
    Table *res = malloc(sizeof(Table));
    struct Row *first_row = malloc_row(0);
    *res = (Table){
        .num_cols             = 0,
        .curr_col             = 0,
        .first_row            = first_row,
        .curr_row             = first_row,
        .num_rows             = 1,
        .h_aligns             = { H_ALIGN_LEFT },
        .v_aligns             = { V_ALIGN_TOP },
        .borders_left         = { BORDER_NONE },
        .border_left_counters = { 0 }
    };
    return res;
}

/*
Summary: Frees all rows and content strings in cells created by add_cell_fmt.
    Don't use the table any more, get a new one!
*/
void free_table(Table *table)
{
    assert(table != NULL);

    struct Row *row = table->first_row;
    while (row != NULL)
    {
        struct Row *next_row = row->next_row;
        free_row(table, row);
        row = next_row;
    }
    free(table);
}

void set_position(Table *table, size_t x, size_t y)
{
    assert(table != NULL);
    assert(x < TABLE_MAX_COLS);

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
        while (table->curr_col < TABLE_MAX_COLS && table->curr_row->cells[table->curr_col].is_set)
        {
            table->curr_col++;
        }
    }
}

/*
Summary: Adds next cell. Buffer is not copied. print_table will access it.
    Ensure that lifetime of buffer outlasts last call of print_table!
*/
void add_cell(Table *table, const char *text)
{
    add_text_cell(table, (char*)text, false);
}

void add_cells(Table *table, size_t num_cells, ...)
{
    va_list args;
    va_start(args, num_cells);
    for (size_t i = 0; i < num_cells; i++)
    {
        add_cell(table, va_arg(args, const char*));
    }
    va_end(args);
}

/*
Summary: Same as add_cell, but frees buffer on reset
*/
void add_cell_gc(Table *table, char *text)
{
    add_text_cell(table, text, true);
}

void add_empty_cell(Table *table)
{
    add_text_cell(table, NULL, false);
}

/*
Summary: Adds next cell and maintains buffer of text.
    Use add_cell to save memory if you maintain a content string yourself.
*/
void add_cell_fmt(Table *table, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    add_cell_vfmt(table, fmt, args);
    va_end(args);
}

void add_cell_vfmt(Table *table, const char *fmt, va_list args)
{
    StringBuilder builder = strb_create(0);
    vstrb_append(&builder, fmt, args);
    add_text_cell(table, builder.buffer, true);
}

/*
Summary: Puts contents of memory-contiguous 2D array into table cell by cell.
    Strings are not copied. Ensure that lifetime of array outlasts last call of print_table.
    Position of next insertion is first cell in next row.
*/
void add_cells_from_array(Table *table, size_t width, size_t height, const char **array)
{
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
void set_default_alignments(Table *table, size_t num_alignments, const TableHAlign *h_aligns, const TableVAlign *v_aligns)
{
    assert(table != NULL);
    assert(num_alignments <= TABLE_MAX_COLS);

    for (size_t i = 0; i < num_alignments; i++)
    {
        if (h_aligns != NULL) table->h_aligns[i] = h_aligns[i];
        if (v_aligns != NULL) table->v_aligns[i] = v_aligns[i];
    }
}

/*
Summary: Overrides alignment of current cell
*/
void override_horizontal_alignment(Table *table, TableHAlign h_align)
{
    assert(table != NULL);
    override_h_align_internal(get_curr_cell(table), h_align);
}

void override_vertical_alignment(Table *table, TableVAlign v_align)
{
    assert(table != NULL);
    override_v_align_internal(get_curr_cell(table), v_align);
}

/*
Summary: Overrides alignment of all cells in current row
*/
void override_horizontal_alignment_of_row(Table *table, TableHAlign h_align)
{
    assert(table != NULL);
    for (size_t i = 0; i < TABLE_MAX_COLS; i++)
    {
        override_h_align_internal(&table->curr_row->cells[i], h_align);
    }
}

void override_vertical_alignment_of_row(Table *table, TableVAlign v_align)
{
    assert(table != NULL);
    for (size_t i = 0; i < TABLE_MAX_COLS; i++)
    {
        override_v_align_internal(&table->curr_row->cells[i], v_align);
    }
}

void set_hline(Table *table, TableBorderStyle style)
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

void set_vline(Table *table, size_t index, TableBorderStyle style)
{
    assert(table != NULL);
    assert (index < TABLE_MAX_COLS);

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

void make_boxed(Table *table, TableBorderStyle style)
{
    assert(table != NULL);
    set_position(table, 0, 0);
    set_vline(table, 0, style);
    set_hline(table, style);
    set_position(table, table->num_cols, table->num_rows - 1);
    set_vline(table, table->num_cols, style);
    set_hline(table, style);
}

void override_left_border(Table *table, TableBorderStyle style)
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

void override_above_border(Table *table, TableBorderStyle style)
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
    assert(table->curr_col + span_x <= TABLE_MAX_COLS);
    struct Cell *cell = &table->curr_row->cells[table->curr_col];
    assert(cell->span_x == 1);
    assert(cell->span_y == 1);

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

void set_all_vlines(Table *table, TableBorderStyle style)
{
    assert(table != NULL);

    for (size_t i = 1; i < table->num_cols; i++)
    {
        set_vline(table, i, style);
    }
}

// Printing

#ifdef DEBUG
__attribute__((unused))
static void print_debug(Table *table)
{
    size_t col_widths[TABLE_MAX_COLS];
    size_t *row_heights = malloc(table->num_rows * sizeof(size_t));
    get_dimensions(table, col_widths, row_heights);

    printf("Table dimensions: #rows: %zu, #cols: %zu\n", table->num_rows, table->num_cols);
    for (size_t i = 0; i < table->num_rows; i++) printf("%zu ", row_heights[i]);
    printf("; ");
    for (size_t i = 0; i < table->num_cols; i++) printf("%zu ", col_widths[i]);
    printf("\n");
    free(row_heights);
}
#endif

/*
Summary: Prints table to stdout
*/
void print_table(Table *table)
{
    assert(table != NULL);
    fprint_table(table, stdout);
}

void fprint_table(Table *table, FILE *stream)
{
    if (table->num_cols == 0)
    {
        return;
    }

    size_t col_widths[TABLE_MAX_COLS];
    size_t *row_heights = malloc(table->num_rows * sizeof(size_t));
    get_dimensions(table, col_widths, row_heights);
    override_superfluous_lines(table, col_widths[table->num_cols - 1], row_heights[table->num_rows - 1]);
    
    //#ifdef DEBUG
    //print_debug(table);
    //#endif

    size_t line_indices[TABLE_MAX_COLS];
    for (size_t i = 0; i < table->num_cols; i++) line_indices[i] = 0;

    // Print rows
    struct Row *prev_row = NULL;
    struct Row *curr_row = table->first_row;
    size_t row_index = 0;
    while (curr_row != NULL)
    {
        if (curr_row->border_above_counter > 0)
        {
            print_row_border(table, prev_row, curr_row, line_indices, col_widths, row_heights, stream);
        }

        // Reset line indices for newly beginning cells, don't reset them for cells that are children spanning from above
        for (size_t j = 0; j < table->num_cols; j++)
        {
            if (curr_row->cells[j].parent == NULL || curr_row->cells[j].y == curr_row->cells[j].parent->y)
            {
                line_indices[j] = 0;
            }
        }

        for (size_t j = 0; j < row_heights[row_index]; j++)
        {
            // Print cell
            for (size_t k = 0; k < table->num_cols; k += get_span_x(&curr_row->cells[k]))
            {
                if (table->border_left_counters[k] > 0)
                {
                    switch (get_border_left(table->borders_left[k], &curr_row->cells[k]))
                    {
                        case BORDER_SINGLE:
                            fprintf(stream, "%s", BORDER_MATRIX_SINGLE[VLINE_INDEX]);
                            break;
                        case BORDER_DOUBLE:
                            fprintf(stream, "%s", BORDER_MATRIX_DOUBLE[VLINE_INDEX]);
                            break;
                        case BORDER_NONE:
                            fprintf(stream, " ");
                    }
                }

                print_text(&curr_row->cells[k],
                    table->h_aligns[k],
                    table->v_aligns[k],
                    line_indices[k],
                    get_total_width(table, col_widths, &curr_row->cells[k]),
                    get_total_height(table, row_heights, &curr_row->cells[k]),
                    stream);
                
                line_indices[k]++;
            }

            fprintf(stream, "\n");
        }

        row_index++;
        prev_row = curr_row;
        curr_row = curr_row->next_row;
    }

    free(row_heights);
}
