#ifndef ITERABLEINTERFACE_H_YR9SAFND
#define ITERABLEINTERFACE_H_YR9SAFND

#include "Interface.h"
#include "Object.h"

#define ITERABLE_INTERFACE_TYPE (iterable_interface_get_type())
DECLARE_INTERFACE(Iterable, iterable, ITERABLE);

typedef int (*cmp_f)(const void *a, const void *b);
typedef void (*free_f)(void *ptr);

struct _IterableInterface
{
	Interface parent;

	void (*append_val)(Iterable *self, const void *value);
	void (*prepend_val)(Iterable *self, const void *value);
	void (*insert_val)(Iterable *self, size_t index, const void *value);
	void (*append_vals)(Iterable *self, const void *data, size_t len);
	void (*prepend_vals)(Iterable *self, const void *data, size_t len);
	void (*insert_vals)(Iterable *self, size_t index, const void *data, size_t len);
	void (*remove_index)(Iterable *self, size_t index, bool to_free);
	void (*remove_range)(Iterable *self, size_t index, size_t len, bool to_free);
	void (*sort)(Iterable *self, cmp_f cmp_func);
	bool (*binary_search)(Iterable *self, const void *target, cmp_f cmp_func, size_t *index);
	Iterable* (*unique)(Iterable *self, cmp_f cmp_func);
	void (*set_free_func)(Iterable *self, free_f free_func);
};

void iterable_append_val(Iterable *self, const void *value);
void iterable_prepend_val(Iterable *self, const void *value);
void iterable_insert_val(Iterable *self, size_t index, const void *value);
void iterable_append_vals(Iterable *self, const void *data, size_t len);
void iterable_prepend_vals(Iterable *self, const void *data, size_t len);
void iterable_insert_vals(Iterable *self, size_t index, const void *data, size_t len);
void iterable_remove_index(Iterable *self, size_t index, bool to_free);
void iterable_remove_range(Iterable *self, size_t index, size_t len, bool to_free);
void iterable_sort(Iterable *self, cmp_f cmp_func);
bool iterable_binary_search(Iterable *self, const void *target, cmp_f cmp_func, size_t *index);
Iterable* iterable_unique(Iterable *self, cmp_f cmp_func);
void iterable_set_free_func(Iterable *self, free_f free_func);

#endif /* end of include guard: ITERABLEINTERFACE_H_YR9SAFND */
