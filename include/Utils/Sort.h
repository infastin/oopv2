#ifndef SORT_H_KRWPHNRW
#define SORT_H_KRWPHNRW

#include <stdlib.h>
#include "Base/Definitions.h"

void inssort(void *mass, size_t len, size_t elemsize, CmpFunc cmp_func);
void heapsort(void *mass, size_t len, size_t elemsize, CmpFunc cmp_func);
void quicksort(void *mass, size_t len, size_t elemsize, CmpFunc cmp_func);

#endif /* end of include guard: SORT_H_KRWPHNRW */
