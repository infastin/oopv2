#ifndef ARRAY_H_7HHJRIFF
#define ARRAY_H_7HHJRIFF

#include <stdbool.h>
#include <stddef.h>

#include "Base.h"
#include "Interfaces/StringerInterface.h"

#define ARRAY_TYPE (array_get_type())
DECLARE_TYPE(Array, array, ARRAY, Object);

Array* array_new(bool clear, bool zero_terminated, size_t elemsize, FreeFunc free_func);
Array* array_copy(const Array *self);
Array* array_set(Array *self, size_t index, const void *data);
void array_get(const Array *self, size_t index, void *ret);
Array* array_append(Array *self, const void *data);
Array* array_prepend(Array *self, const void *data);
Array* array_insert(Array *self, size_t index, const void *data);
Array* array_append_many(Array *self, const void *data, size_t len);
Array* array_prepend_many(Array *self, const void *data, size_t len);
Array* array_insert_many(Array *self, size_t index, const void *data, size_t len);
Array* array_remove_index(Array *self, size_t index);
Array* array_remove_range(Array *self, size_t index, size_t len);
Array* array_remove_val(Array *self, const void *target, CmpFunc cmp_func, bool remove_all);
void array_sort(Array *self, CmpFunc cmp_func);
bool array_binary_search(Array *self, const void *target, CmpFunc cmp_func, size_t *index);
Array* array_unique(Array *self, CmpFunc cmp_func);
void array_delete(Array *self);
void* array_steal(Array *self, size_t *len);
ssize_t array_get_length(const Array *self);
void* array_pop(Array *self);
bool array_is_empty(const Array *self);

#define array_output(self, str_func...)                        \
	(                                                          \
	  	(IS_ARRAY(self)) ?                                     \
	  	(stringer_output((const Stringer*) self, str_func)) :  \
	  	(return_if_fail_warning(STRFUNC, "IS_ARRAY("#self")")) \
  	)

#define array_outputln(self, str_func...)                       \
	(                                                           \
		(IS_ARRAY(self)) ?                                      \
		(stringer_outputln((const Stringer*) self, str_func)) : \
		(return_if_fail_warning(STRFUNC, "IS_ARRAY("#self")"))  \
	)

#endif /* end of include guard: ARRAY_H_7HHJRIFF */
