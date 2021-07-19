#include <stdlib.h>

#include "Utils/Sort.h"
#include "Base/Macros.h"
#include "Base/Messages.h"

#define SORT_LEN_THRESHOLD 16

#define SWAP(a, b, elemsize)          \
{                                     \
	size_t __size = (elemsize);       \
	char *__a = (a); char *__b = (b); \
	do                                \
	{                                 \
		char __tmp = *__a;            \
		*__a++ = *__b;                \
		*__b++ = __tmp;               \
	} while (--__size > 0);           \
}

/* Insertion sort */
void inssort(void *mass, size_t len, size_t elemsize, CmpFunc cmp_func)
{
	return_if_fail(mass != NULL);
	return_if_fail(cmp_func != NULL);
	return_if_fail(elemsize != 0);

	for (int i = 1; i < len; ++i) 
	{
		size_t cur = i;

		for (size_t j = i - 1; j >= 0; --j) 
		{
			if (cmp_func(mass_cell(mass, elemsize, j), mass_cell(mass, elemsize, cur)) <= 0)
				break;

			SWAP(mass_cell(mass, elemsize, j), mass_cell(mass, elemsize, cur), elemsize);

			cur = j;

			if (j == 0)
				break;
		}
	}
}

/* Heapsort */
static inline void heap(void *mass, size_t start, size_t end, size_t elemsize, CmpFunc cmp_func)
{
	size_t root = start;

	while ((root << 1) < end)
	{
		size_t child = (root << 1) + 1;

		if ((child < end) && cmp_func(mass_cell(mass, elemsize, child), mass_cell(mass, elemsize, child + 1)) < 0)
			child++;

		if (cmp_func(mass_cell(mass, elemsize, root), mass_cell(mass, elemsize, child)) < 0)
		{
			SWAP(mass_cell(mass, elemsize, root), mass_cell(mass, elemsize, child), elemsize);
			root = child;
		}
		else
			return;
	}
}

static inline void heapify(void *mass, size_t len, size_t elemsize, CmpFunc cmp_func)
{
	size_t start = (len - 1) >> 1;

	while (start >= 0) 
	{
		heap(mass, start, len - 1, elemsize, cmp_func);

		if (start == 0)
			break;

		start--;
	}
}

void heapsort(void *mass, size_t len, size_t elemsize, CmpFunc cmp_func)
{
	return_if_fail(mass != NULL);
	return_if_fail(cmp_func != NULL);
	return_if_fail(elemsize != 0);

	size_t end = len - 1;

	if (len <= 1)
		return;

	heapify(mass, len, elemsize, cmp_func);

	while (end > 0)
	{
		SWAP(mass_cell(mass, elemsize, 0), mass_cell(mass, elemsize, end), elemsize);
		end--;
		heap(mass, 0, end, elemsize, cmp_func);
	}
}

/* Based on Knuth vol. 3 */
static inline size_t quicksort_partition(void *mass, size_t left, size_t right, size_t pivot, size_t elemsize, CmpFunc cmp_func)
{
	size_t i = left + 1;
	size_t j = right;

	if (pivot != left)
		SWAP(mass_cell(mass, elemsize, left), mass_cell(mass, elemsize, pivot), elemsize);

	while (1) 
	{
		while (cmp_func(mass_cell(mass, elemsize, i), mass_cell(mass, elemsize, left)) < 0)
			i++;

		while (cmp_func(mass_cell(mass, elemsize, left), mass_cell(mass, elemsize, j)) < 0)
			j--;

		if (j <= i)
		{
			SWAP(mass_cell(mass, elemsize, j), mass_cell(mass, elemsize, left), elemsize);
			return j;
		}

		SWAP(mass_cell(mass, elemsize, i), mass_cell(mass, elemsize, j), elemsize);

		i++;
		j--;
	}

	return 0;
}

static inline size_t find_median(void *mass, size_t a, size_t b, size_t c, size_t elemsize, CmpFunc cmp_func)
{
	if (cmp_func(mass_cell(mass, elemsize, a), mass_cell(mass, elemsize, b)) > 0)
	{
		if (cmp_func(mass_cell(mass, elemsize, b), mass_cell(mass, elemsize, c)) > 0)
			return b;
		else if (cmp_func(mass_cell(mass, elemsize, a), mass_cell(mass, elemsize, c)) > 0)
			return c;
		else 
			return a;
	}
	else
	{
		if (cmp_func(mass_cell(mass, elemsize, a), mass_cell(mass, elemsize, c)) > 0)
			return a;
		else if (cmp_func(mass_cell(mass, elemsize, b), mass_cell(mass, elemsize, c)) > 0)
			return c;
		else 
			return b;
	}
}

static inline void quicksort_recursive(void *mass, size_t left, size_t right, size_t elemsize, CmpFunc cmp_func)
{
	size_t mid;
	size_t pivot;
	size_t new_pivot;

	int loop_count = 0;
	int max_loops = ULONG_BIT - __builtin_clzl(right - left);

	while (1) 
	{
		if (right <= left)
			return;

		if ((right - left + 1) <= SORT_LEN_THRESHOLD)
		{
			inssort(mass_cell(mass, elemsize, left), right - left + 1, elemsize, cmp_func);
			return;
		}

		if (++loop_count >= max_loops)
		{
			heapsort(mass_cell(mass, elemsize, left), right - left + 1, elemsize, cmp_func);
			return;
		}

		mid = left + ((right - left) >> 1);
		pivot = find_median(mass, left, mid, right, elemsize, cmp_func);
		new_pivot = quicksort_partition(mass, left, right, pivot, elemsize, cmp_func);

		if (new_pivot == 0)
			return;

		if ((new_pivot - left - 1) > (right - new_pivot - 1))
		{
			quicksort_recursive(mass, new_pivot + 1, right, elemsize, cmp_func);
			right = new_pivot - 1;
		}
		else 
		{
			quicksort_recursive(mass, left, new_pivot - 1, elemsize, cmp_func);
			left = new_pivot + 1;
		}
	}
}

void quicksort(void *mass, size_t len, size_t elemsize, CmpFunc cmp_func)
{
	return_if_fail(mass != NULL);
	return_if_fail(cmp_func != NULL);
	return_if_fail(elemsize != 0);

	quicksort_recursive(mass, 0, len - 1, elemsize, cmp_func);
}
