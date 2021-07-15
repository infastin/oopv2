#include <stdbool.h>

#include "Base/Definitions.h"
#include "Base/Macros.h"
#include "Base/Messages.h"

bool linear_search(void *mass, const void *target, size_t len, size_t elemsize, CmpFunc cmp_func, size_t *index)
{
	return_val_if_fail(mass != NULL, false);
	return_val_if_fail(cmp_func != NULL, false);
	return_val_if_fail(elemsize != 0, false);

	for (size_t i = 0; i < len; ++i) 
	{
		if (cmp_func(mass_cell(mass, elemsize, i), target) == 0)
		{
			if (index != NULL)
				*index = i;

			return true;
		}
	}

	return false;
}

bool binary_search(void *mass, const void *target, size_t left, size_t right, size_t elemsize, CmpFunc cmp_func, size_t *index)
{
	return_val_if_fail(mass != NULL, false);
	return_val_if_fail(cmp_func != NULL, false);
	return_val_if_fail(elemsize != 0, false);

	while (left <= right) 
	{
		size_t mid = left + ((right - left) >> 1);

		if (cmp_func(mass_cell(mass, elemsize, mid), target) == 0)
		{
			if (index != NULL)
				*index = mid;

			return true;
		}

		if (cmp_func(mass_cell(mass, elemsize, mid), target) < 0)
			left = mid + 1;
		else
			right = mid - 1;
	}

	return false;
}
