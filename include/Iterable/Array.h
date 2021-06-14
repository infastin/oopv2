#ifndef ARRAY_H_7HHJRIFF
#define ARRAY_H_7HHJRIFF

#include <stdbool.h>
#include <stddef.h>

#include "Object.h"
#include "IterableInterface.h"
#include "StringerInterface.h"

#define ARRAY_TYPE (array_get_type())
DECLARE_TYPE(Array, array, ARRAY, Object);

struct _Array
{
	Object parent;
	size_t len;
	void *data;
};

Array* array_new(bool clear, bool zero_terminated, size_t elemsize);
void array_set(Array *self, size_t index, const void *value);
void array_get(Array *self, size_t index, void *ret);
void array_append_val(Array *self, const void *value);
void array_prepend_val(Array *self, const void *value);
void array_insert_val(Array *self, size_t index, const void *value);
void array_append_vals(Array *self, const void *data, size_t len);
void array_prepend_vals(Array *self, const void *data, size_t len);
void array_insert_vals(Array *self, size_t index, const void *data, size_t len);
void array_remove_index(Array *self, size_t index, bool to_free);
void array_sort(Array *self, cmp_f cmp_func);
bool array_binary_search(Array *self, const void *target, cmp_f cmp_func, size_t *index);
Array* array_unique(Array *self, cmp_f cmp_func);
void array_delete(Array *self, bool free_segment);
void array_set_free_func(Array *self, free_f free_func);
void array_remove_range(Array *self, size_t index, size_t len, bool to_free);

#define array_output(self, str_func...) (stringer_output((Stringer*) self, str_func))

#endif /* end of include guard: ARRAY_H_7HHJRIFF */
