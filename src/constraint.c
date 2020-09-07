#include <string.h>
#include <stdio.h>
#include "constraint.h"
#include "printing.h"
#include "string_util.h" // For is_digit

struct Constraint
{
    size_t from_index;
    size_t to_index;
    size_t min;
};

void set_dot_paddings(size_t num_cells, TextAlignment default_align, struct Cell **col)
{
    size_t after_dot[num_cells];
    size_t max_after_dot = 0;

    for (size_t i = 0; i < num_cells; i++)
    {
        if (col[i]->text == NULL) continue;
        after_dot[i] = 0;

        if (get_align(default_align, col[i]) == ALIGN_NUMBERS)
        {
            bool before_dot = true;
            bool before_first_digit = true;

            StringIterator iterator = get_iterator(col[i]->text);
            char curr_char;
            while ((curr_char = get_next_char(&iterator)) != '\0')
            {
                if (before_dot)
                {
                    if (curr_char == DECIMAL_SEPARATOR)
                    {
                        before_dot = false;
                    }
                    else
                    {
                        if (is_digit(curr_char))
                        {
                            before_first_digit = false;
                        }
                        else
                        {
                            if (!before_first_digit)
                            {
                                break;
                            }
                        }
                    }
                }
                else
                {
                    if (is_digit(curr_char))
                    {
                        before_first_digit = false;
                        after_dot[i]++;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            if (after_dot[i] > max_after_dot) max_after_dot = after_dot[i];
            if (!before_first_digit)
            {
                col[i]->zero_position = iterator.index - 1;
            }
            else
            {
                col[i]->align = ALIGN_RIGHT;
                col[i]->override_align = true;
            }
        }
    }

    for (size_t i = 0; i < num_cells; i++)
    {
        if (col[i]->text == NULL) continue;
        if (get_align(default_align, col[i]) == ALIGN_NUMBERS)
        {
            col[i]->zeros_needed = max_after_dot - after_dot[i];
            if (max_after_dot > 0 && after_dot[i] == 0) col[i]->dot_needed = true;
        }
    }
}

size_t needed_to_satisfy(struct Constraint *constr, size_t *vars)
{
    size_t sum = 0;
    for (size_t i = constr->from_index; i < constr->to_index; i++)
    {
        sum += vars[i];
    }
    return sum < constr->min ? constr->min - sum : 0;
}

// Make sure to zero out result before
void satisfy_constraints(size_t num_constrs, struct Constraint *constrs, size_t *result)
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

size_t get_num_lines(char *string)
{
    if (string == NULL) return 0;
    size_t res = 1;
    size_t pos = 0;
    while (string[pos] != '\0')
    {
        if (string[pos] == '\n')
        {
            res++;
        }
        pos++;
    }
    return res;
}

size_t get_text_width(char *str)
{
    size_t res = 0;
    for (size_t i = 0; i < get_num_lines(str); i++)
    {
        char *line;
        get_line_of_string(str, i, &line);
        size_t line_width = console_strlen(line);
        if (line_width > res) res = line_width;
    }
    return res;
}

// out_cells must hold table->num_rows pointers to cells
void get_col(Table *table, size_t col_index, struct Cell **out_cells)
{
    struct Row *curr_row = table->first_row;
    size_t index = 0;
    while (curr_row != NULL)
    {
        out_cells[index] = &curr_row->cells[col_index];
        curr_row = curr_row->next_row;
        index++;
    }
}

void get_dimensions(Table *table, size_t *out_col_widths, size_t *out_row_heights)
{
    size_t num_cells_upper = table->num_cols * table->num_rows;
    struct Constraint constrs[num_cells_upper];

    // Calculate padding for ALIGN_NUMBERS
    for (size_t i = 0; i < table->num_cols; i++)
    {
        struct Cell *cells[table->num_rows];
        get_col(table, i, (struct Cell**)&cells);
        set_dot_paddings(table->num_rows, table->alignments[i], cells);
    }

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
                size_t min = get_text_width(curr_row->cells[i].text) + curr_row->cells[i].zeros_needed;
                if (curr_row->cells[i].dot_needed) min++;
                // Constraint can be weakened when vlines are in between
                for (size_t j = i + 1; j < i + curr_row->cells[i].span_x; j++)
                {
                    if (min == 0) break;
                    if (table->border_left_counters[j] > 0) min--;
                }

                constrs[index] = (struct Constraint){
                    .min = min,
                    .from_index = i,
                    .to_index = i + curr_row->cells[i].span_x
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
                size_t min = get_num_lines(curr_row->cells[i].text);
                struct Row *row_checked_for_hline = curr_row->next_row;

                // Constraint can be weakened when hlines are in between
                for (size_t j = i + 1; j < i + curr_row->cells[i].span_y; j++)
                {
                    if (row_checked_for_hline == NULL || min == 0) break;
                    if (row_checked_for_hline->border_above_counter > 0) min--;
                    row_checked_for_hline = row_checked_for_hline->next_row;
                }

                constrs[index] = (struct Constraint){
                    .min = min,
                    .from_index = row_index,
                    .to_index = row_index + curr_row->cells[i].span_y
                };
                index++;
            }
        }
        curr_row = curr_row->next_row;
        row_index++;
    }
    for (size_t i = 0; i < table->num_rows; i++) out_row_heights[i] = 0;
    satisfy_constraints(index, constrs, out_row_heights);
}
