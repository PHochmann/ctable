#include <stdio.h>
#include "alloc_wrappers.h"

#define ERROR_MESSAGE "Out of heap memory - terminating\n"

void print_error_and_exit()
{
    fprintf(stderr, ERROR_MESSAGE);
    exit(EXIT_FAILURE);
}

void *malloc_wrapper(size_t size)
{
    void *res = malloc(size);
    if (size != 0 && res == NULL)
    {
        print_error_and_exit();
    }
    return res;
}

void *realloc_wrapper(void *ptr, size_t size)
{
    void *res = realloc(ptr, size);
    if (size != 0 && res == NULL)
    {
        print_error_and_exit();
    }
    return res;
}

void *calloc_wrapper(size_t n, size_t elem_size)
{
    void *res = calloc(n, elem_size);
    if (n != 0 && elem_size != 0 && res == NULL)
    {
        print_error_and_exit();
    }
    return res;
}
