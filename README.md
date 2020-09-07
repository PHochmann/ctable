![build](https://github.com/PhilippHochmann/ctable/workflows/build/badge.svg)
[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](http://www.gnu.org/licenses/gpl-3.0)

# ctable
Library to print nicely formatted tables to stdout.
Supports...
* ...cells spanning over other multiple columns or rows
* ...ANSI color sequences
* ...alignment of numbers under decimal dot

## How to use it
Include ```src/table.h``` to use it. Invoke ```make``` to run tests.

## Example
``┌──────────┬───────────────┬──────────┬───────┬──────┐``<br/>
``│  alpha   │     beta      │  gamma   │ delta │ test │``<br/>
``├══════════┼═══════════════┼══════════┼═══════┼══════┤``<br/>
``│ 1        │ -1110.1000000 │ a....... │  777  │      │``<br/>
``│ 2        │    10.0000000 │        b │  222  │      │``<br/>
``│ 3....... │    23.1132310 │        c │  333  │      │``<br/>
``├──────────┴───────────────┼──────────┼───────┼──────┤``<br/>
``│ span x                   │ span y   │   !   │      │``<br/>
``├──────────────────────────┤ span y   │       └──────┤``<br/>
``│ span x                   │ span y   │  ^ no border │``<br/>
``├══════════════════════════┘ span y   │   and span x │``<br/>
``│ span x                     < span y │   and also y │``<br/>
``└─────────────────────────────────────┴──────────────┘``<br/>

## Functions
### Data and printing
Table get_empty_table();
void print_table(Table *table);
void free_table(Table *table);

### Control
void set_position(Table *table, size_t x, size_t y);
void next_row(Table *table);

### Cell insertion
void add_empty_cell(Table *table);
void add_cell(Table *table, char *text);
void add_cell_gc(Table *table, char *text);
void add_cell_fmt(Table *table, char *fmt, ...);
void add_cell_vfmt(Table *table, char *fmt, va_list args);
void add_cells_from_array(Table *table, size_t width, size_t height, char **array);

### Settings
void set_default_alignments(Table *table, size_t num_alignments, TextAlignment *alignments);
void override_alignment(Table *table, TextAlignment alignment);
void override_alignment_of_row(Table *table, TextAlignment alignment);
void set_hline(Table *table, BorderStyle style);
void set_vline(Table *table, size_t index, BorderStyle style);
void make_boxed(Table *table, BorderStyle style);
void set_all_vlines(Table *table, BorderStyle style);
void override_left_border(Table *table, BorderStyle style);
void override_above_border(Table *table, BorderStyle style);
void set_span(Table *table, size_t span_x, size_t span_y);
