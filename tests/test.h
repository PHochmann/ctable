#pragma once
#include <stdbool.h>
#include "../src/string_util.h"

#define F_RED     "\x1B[1;31m"
#define F_GREEN   "\x1B[1;32m"
#define COL_RESET "\x1B[0m"

typedef struct {
    bool (*suite)(Vector *error_builder);
    int num_cases;
    char *name;
} Test;
