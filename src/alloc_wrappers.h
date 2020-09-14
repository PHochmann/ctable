#include <stdlib.h>

void *malloc_wrapper(size_t size);
void *realloc_wrapper(void *ptr, size_t size);
void *calloc_wrapper(size_t n, size_t elem_size);