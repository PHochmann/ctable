#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "test.h"
#include "test_table.h"
#include "../src/table.h"

static const size_t NUM_TESTS = 1;
static Test (*test_getters[])() = {
    get_table_test,
};

int main()
{
    Table table = get_empty_table();
    set_default_alignments(&table, 4,
        (TextAlignment[]){ ALIGN_RIGHT, ALIGN_LEFT, ALIGN_RIGHT, ALIGN_LEFT });
    add_empty_cell(&table);
    override_left_border(&table, BORDER_NONE);
    add_cell(&table, " Test suite ");
    add_cell(&table, " #Cases ");
    add_cell(&table, " Result ");
    override_alignment_of_row(&table, ALIGN_LEFT);
    next_row(&table);
    set_hline(&table, BORDER_SINGLE);

    bool error = false;
    Vector error_builder = strbuilder_create(100);
    for (size_t i = 0; i < NUM_TESTS; i++)
    {
        Test test = test_getters[i]();
        if (test.num_cases != 0)
        {
            add_cell_fmt(&table, " %zu ", i + 1);
            add_cell_fmt(&table, " %s ", test.name);
            add_cell_fmt(&table, " %d ", test.num_cases);

            if (test.suite(&error_builder))
            {
                add_cell(&table, F_GREEN " passed " COL_RESET);
            }
            else
            {
                printf("[" F_RED "%s" COL_RESET "] %s",
                    test_getters[i]().name,
                    (char*)error_builder.buffer);
                add_cell(&table, F_RED " failed " COL_RESET);
                error = true;
            }
            strbuilder_reset(&error_builder);
            next_row(&table);
        }
    }
    vec_destroy(&error_builder);

    set_span(&table, 3, 1);
    override_alignment(&table, ALIGN_CENTER);
    set_hline(&table, BORDER_SINGLE);
    add_cell(&table, " End result ");
    add_cell(&table, error ? F_RED " failed " COL_RESET : F_GREEN " passed " COL_RESET);
    next_row(&table);
    make_boxed(&table, BORDER_SINGLE);
    print_table(&table);
    free_table(&table);

    return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
