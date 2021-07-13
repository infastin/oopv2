#ifndef SEARCH_H_SZMXDFHI
#define SEARCH_H_SZMXDFHI

#include <stdbool.h>

#include "Base/Definitions.h"

bool linear_search(void *mass, const void* target, size_t len, size_t elemsize, CmpFunc cmp_func, size_t *index);
bool binary_search(void *mass, const void* target, size_t left, size_t right, size_t elemsize, CmpFunc cmp_func, size_t *index);

#endif /* end of include guard: SEARCH_H_SZMXDFHI */
