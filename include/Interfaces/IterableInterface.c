#include "IterableInterface.h"
#include "Interface.h"
#include "Messages.h"

DEFINE_INTERFACE(Iterable, iterable);

void iterable_append_val(Iterable *self, const void *value)
{
	return_if_fail(IS_ITERABLE(self));

	IterableInterface *iface = ITERABLE_INTERFACE(self);
	
	return_if_fail(iface->append_val != NULL);
	iface->append_val(self, value);
}

void iterable_prepend_val(Iterable *self, const void *value)
{
	return_if_fail(IS_ITERABLE(self));

	IterableInterface *iface = ITERABLE_INTERFACE(self);

	return_if_fail(iface->prepend_val != NULL);
	iface->prepend_val(self, value);
}

void iterable_insert_val(Iterable *self, size_t index, const void *value)
{
	return_if_fail(IS_ITERABLE(self));

	IterableInterface *iface = ITERABLE_INTERFACE(self);

	return_if_fail(iface->insert_val != NULL);
	iface->insert_val(self, index, value);
}

void iterable_append_vals(Iterable *self, const void *data, size_t len)
{
	return_if_fail(IS_ITERABLE(self));

	IterableInterface *iface = ITERABLE_INTERFACE(self);

	return_if_fail(iface->append_vals != NULL);
	iface->append_vals(self, data, len);
}

void iterable_prepend_vals(Iterable *self, const void *data, size_t len)
{
	return_if_fail(IS_ITERABLE(self));

	IterableInterface *iface = ITERABLE_INTERFACE(self);

	return_if_fail(iface->prepend_vals != NULL);
	iface->prepend_vals(self, data, len);
}

void iterable_insert_vals(Iterable *self, size_t index, const void *data, size_t len)
{
	return_if_fail(IS_ITERABLE(self));

	IterableInterface *iface = ITERABLE_INTERFACE(self);

	return_if_fail(iface->insert_vals != NULL);
	iface->insert_vals(self, index, data, len);
}

void iterable_remove_index(Iterable *self, size_t index, bool to_free)
{
	return_if_fail(IS_ITERABLE(self));

	IterableInterface *iface = ITERABLE_INTERFACE(self);

	return_if_fail(iface->remove_index != NULL);
	iface->remove_index(self, index, to_free);
}

void iterable_remove_range(Iterable *self, size_t index, size_t len, bool to_free)
{
	return_if_fail(IS_ITERABLE(self));

	IterableInterface *iface = ITERABLE_INTERFACE(self);

	return_if_fail(iface->remove_range != NULL);
	iface->remove_range(self, index, len, to_free);
}

void iterable_sort(Iterable *self, cmp_f cmp_func)
{
	return_if_fail(IS_ITERABLE(self));

	IterableInterface *iface = ITERABLE_INTERFACE(self);

	return_if_fail(iface->sort != NULL);
	iface->sort(self, cmp_func);
}

bool iterable_binary_search(Iterable *self, const void *target, cmp_f cmp_func, size_t *index)
{
	return_val_if_fail(IS_ITERABLE(self), false);

	IterableInterface *iface = ITERABLE_INTERFACE(self);

	return_val_if_fail(iface->binary_search != NULL, false);
	return iface->binary_search(self, target, cmp_func, index);
}

Iterable* iterable_unique(Iterable *self, cmp_f cmp_func)
{
	return_val_if_fail(IS_ITERABLE(self), NULL);

	IterableInterface *iface = ITERABLE_INTERFACE(self);

	return_val_if_fail(iface->unique != NULL, NULL);
	return iface->unique(self, cmp_func);
}

void iterable_set_free_func(Iterable *self, free_f free_func)
{
	return_if_fail(IS_ITERABLE(self));

	IterableInterface *iface = ITERABLE_INTERFACE(self);

	return_if_fail(iface->set_free_func != NULL);
	iface->set_free_func(self, free_func);
}
