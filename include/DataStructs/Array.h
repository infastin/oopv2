#ifndef ARRAY_H_7HHJRIFF
#define ARRAY_H_7HHJRIFF

#include <stdbool.h>
#include <stddef.h>

#include "Base.h"
#include "Interfaces/StringerInterface.h"

#define ARRAY_TYPE (array_get_type())
DECLARE_DERIVABLE_TYPE(Array, array, ARRAY, Object);

struct _ArrayClass
{
	ObjectClass parent;

	Array* (*append)(Array *self, const void *data);
	Array* (*prepend)(Array *self, const void *data);
	Array* (*insert)(Array *self, size_t index, const void *data);
	void   (*sort)(Array *self, CmpFunc cmp_func);
	Array* (*remove_index)(Array *self, size_t index, bool to_free);
	Array* (*remove_val)(Array *self, const void *target, CmpFunc cmp_func, bool to_free, bool remove_all);
	Array* (*unique)(Array *self, CmpFunc cmp_func);
	void   (*set_free_func)(Array *self, FreeFunc free_func);
	Array* (*append_many)(Array *self, const void *data, size_t len);
	Array* (*prepend_many)(Array *self, const void *data, size_t len);
	Array* (*insert_many)(Array *self, size_t index, const void *data, size_t len);
	Array* (*remove_range)(Array *self, size_t index, size_t len, bool to_free);
	bool   (*binary_search)(Array *self, const void *target, CmpFunc cmp_func, size_t *index);
	void*  (*steal)(Array *self, size_t *len);
};

struct _Array
{
	Object parent;
	size_t len;
	void *data;
};

Array* array_new(bool clear, bool zero_terminated, size_t elemsize);
Array* array_copy(const Array *self);
Array* array_set(Array *self, size_t index, const void *data);
void array_get(const Array *self, size_t index, void *ret);
Array* array_append(Array *self, const void *data);
Array* array_prepend(Array *self, const void *data);
Array* array_insert(Array *self, size_t index, const void *data);
Array* array_append_many(Array *self, const void *data, size_t len);
Array* array_prepend_many(Array *self, const void *data, size_t len);
Array* array_insert_many(Array *self, size_t index, const void *data, size_t len);
Array* array_remove_index(Array *self, size_t index, bool to_free);
Array* array_remove_range(Array *self, size_t index, size_t len, bool to_free);
Array* array_remove_val(Array *self, const void *target, CmpFunc cmp_func, bool to_free, bool remove_all);
void array_sort(Array *self, CmpFunc cmp_func);
bool array_binary_search(Array *self, const void *target, CmpFunc cmp_func, size_t *index);
Array* array_unique(Array *self, CmpFunc cmp_func);
void array_delete(Array *self, bool free_segment);
void array_set_free_func(Array *self, FreeFunc free_func);
void *array_steal(Array *self, size_t *len);

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
