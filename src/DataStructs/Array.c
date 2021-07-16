#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "DataStructs/Array.h"
#include "Utils/Stuff.h"
#include "Utils/Sort.h"
#include "Utils/Search.h"

/* Predefinitions {{{ */

static void stringer_interface_init(StringerInterface *iface);

struct _Array
{
	Object parent;
	FreeFunc ff;
	void *mass;
	size_t capacity;
	size_t elemsize;
	size_t len;
	bool clear;
	bool zero_terminated;
};

DEFINE_TYPE_WITH_IFACES(Array, array, object, 1,
		USE_INTERFACE(STRINGER_INTERFACE_TYPE, stringer_interface_init));

#define BINARY_SEARCH_LEN_THRESHOLD 32

#define arr_cell(s, i) (&((char*) ((s)->mass))[(i) * (s)->elemsize])

/* }}} */

/* Private methods {{{ */

static Array* _Array_growcap(Array *self, size_t add)
{
	if (add == 0)
		return self;

	size_t mincap = self->capacity + add;
	size_t new_allocated = (mincap >> 3) + (mincap < 9 ? 3 : 6);

	if (mincap > SIZE_MAX - new_allocated)
	{
		msg_error("array capacity overflow!");
		return NULL;
	}

	mincap += new_allocated;

	void *mass = (void*)realloc(self->mass, mincap * self->elemsize);

	if (mass == NULL)
	{
		msg_error("couldn't reallocate memory for array!");
		return NULL;
	}

	self->mass = mass;

	if (self->clear)
		memset(arr_cell(self, self->capacity), 0, (mincap - self->capacity) * self->elemsize);

	self->capacity = mincap;

	return self;
}

static Array* _Array_insert(Array *self, size_t index, const void *data)
{
	int zt = self->zero_terminated;

	if (index + zt >= self->capacity)
	{
		self = _Array_growcap(self, (index + zt + 1) - self->capacity);
		return_val_if_fail(self != NULL, NULL);
	}
	else if (self->len >= self->capacity)
	{
		self = _Array_growcap(self, 1);
		return_val_if_fail(self != NULL, NULL);
	}

	if (index + 1 >= self->len && zt)
	{
		memset(arr_cell(self, index + 1), 0, self->elemsize);
		self->len = index + zt + 1;
	}
	else if (index >= self->len)
	{
		self->len = index + 1;
	}
	else
	{
		memmove(arr_cell(self, index + 1), arr_cell(self, index), (self->len - index) * self->elemsize);
		self->len++;
	}

	if (data == NULL)
		memset(arr_cell(self, index), 0, self->elemsize);
	else
		memcpy(arr_cell(self, index), data, self->elemsize);

	return self;
}

static Array* _Array_insert_many(Array *self, size_t index, const void *data, size_t len)
{
	int zt = self->zero_terminated;

	if (index + zt + len >= self->capacity)
	{
		self = _Array_growcap(self, (index + zt + len) - self->capacity);
		return_val_if_fail(self != NULL, NULL);
	}
	else if (self->len + len >= self->capacity)
	{
		self = _Array_growcap(self, len);
		return_val_if_fail(self != NULL, NULL);
	}

	if (index + 1 >= self->len && zt)
	{
		self->len = index + len + zt;
		memset(arr_cell(self, index + len), 0, self->elemsize);
	}
	else if (index >= self->len)
	{
		self->len = index + len;
	}
	else
	{
		memmove(arr_cell(self, index + len), arr_cell(self, index), (self->len - index) * self->elemsize);
		self->len += len;
	}

	if (data == NULL)
	{
		for (size_t i = 0; i < len; ++i)
			memset(arr_cell(self, index + i), 0, self->elemsize);
	}
	else
	{
		for (size_t i = 0; i < len; ++i)
			memcpy(arr_cell(self, index + i), mass_cell(data, self->elemsize, i), self->elemsize);
	}

	return self;
}

/* }}} */

/* Public methods {{{ */

static Object* Array_ctor(Object *_self, va_list *ap)
{
	Array *self = ARRAY(OBJECT_CLASS(OBJECT_TYPE)->ctor(_self, ap));

	bool clear = (bool) va_arg(*ap, int);
	bool zero_terminated = (bool) va_arg(*ap, int);
	size_t elemsize = va_arg(*ap, size_t);

	if (clear)
		self->mass = calloc(1, elemsize);
	else
		self->mass = malloc(elemsize);

	if (self->mass == NULL)
	{
		object_delete((Object*) self);
		msg_error("couldn't allocate memory for array!");
		return NULL;
	}

	self->clear = clear;
	self->zero_terminated = zero_terminated;
	self->ff = NULL;
	self->elemsize = elemsize;
	self->capacity = 1;
	
	self->len = 0;

	return _self;
}

static Object* Array_dtor(Object *_self, va_list *ap)
{
	Array *self = ARRAY(_self);

	if (self->mass != NULL)
	{
		if (self->ff != NULL)
			for (size_t i = 0; i < self->len; ++i) 
				self->ff(arr_cell(self, i));

		free(self->mass);
	}

	return _self;
}

static Object* Array_cpy(const Object *_self, Object *_object, va_list *ap)
{
	const Array *self = ARRAY(_self);
	Array *object = ARRAY(OBJECT_CLASS(OBJECT_TYPE)->cpy(_self, _object, ap));

	object->mass = calloc(1, self->capacity * self->elemsize);

	if (object->mass == NULL)
	{
		object_delete((Object*) object);
		msg_error("couldn't allocate memory for the copy of array!");
		return NULL;
	}

	object->clear = self->clear;
	object->zero_terminated = self->zero_terminated;
	object->capacity = self->capacity;
	object->elemsize = self->elemsize;

	object->len = self->len;

	memcpy(object->mass, self->mass, object->len * object->elemsize);

	return _object;
}

static Object* Array_set(Object *_self, va_list *ap)
{
	Array *self = ARRAY(_self);

	size_t index = va_arg(*ap, size_t);
	const void *data = va_arg(*ap, const void*);

	int zt = self->zero_terminated;

	if (index + zt >= self->capacity)
	{
		self = _Array_growcap(self, (index + zt + 1) - self->capacity);
		return_val_if_fail(self != NULL, NULL);
	}

	if (data == NULL)
		memset(arr_cell(self, index), 0, self->elemsize);
	else
		memcpy(arr_cell(self, index), data, self->elemsize);

	if (index + 1 >= self->len && zt) 
		memset(arr_cell(self, index + 1), 0, self->elemsize);

	if (index + zt >= self->len)
		self->len = index + zt + 1;

	return _self;
}

static void Array_get(const Object *_self, va_list *ap)
{
	const Array *self = ARRAY(_self);

	size_t index = va_arg(*ap, size_t);
	void *ret = va_arg(*ap, void*);
	
	return_if_fail(ret != NULL);
	
	if (index >= self->len)
	{
		msg_warn("element at [%lu] is out of bounds!", index);
		return;
	}

	memcpy(ret, arr_cell(self, index), self->elemsize);
}

static Array* Array_insert(Array *self, size_t index, const void *data)
{
	return _Array_insert(self, index, data);
}

static Array* Array_insert_many(Array *self, size_t index, const void *data, size_t len)
{
	return _Array_insert_many(self, index, data, len);
}

static Array* Array_append(Array *self, const void *data)
{
	if (self->zero_terminated)
	{
		if (self->len == 0)
			return _Array_insert(self, self->len, data);
		else
			return _Array_insert(self, self->len - 1, data);
	}
	else
		return _Array_insert(self, self->len, data);
}

static Array* Array_append_many(Array *self, const void *data, size_t len)
{
	if (self->zero_terminated)
	{
		if (self->len == 0)
			return _Array_insert_many(self, self->len, data, len);
		else
			return _Array_insert_many(self, self->len - 1, data, len);
	}
	else
		return _Array_insert_many(self, self->len, data, len);
}

static Array* Array_prepend(Array *self, const void *data)
{
	return _Array_insert(self, 0, data);
}

static Array* Array_prepend_many(Array *self, const void *data, size_t len)
{
	return _Array_insert_many(self, 0, data, len);
}

static Array* Array_remove_index(Array *self, size_t index)
{
	if (index >= self->len)
	{
		msg_warn("element at [%lu] is out of bounds!", index);
		return NULL;
	}

	if (self->zero_terminated && (index == self->len - 1))
	{
		msg_warn("can't remove terminating zero at [%lu]!", index);
		return NULL;
	}

	if (self->ff != NULL)
		self->ff(*((void**) arr_cell(self, index)));

	memmove(arr_cell(self, index), arr_cell(self, index + 1), (self->len - index - 1) * self->elemsize);
	
	self->len--;

	return self;
}

static Array* Array_remove_range(Array *self, size_t index, size_t len)
{
	if (index + len >= self->len)
	{
		msg_warn("range [%lu:%lu] is out of bounds!", index, index + len - 1);
		return NULL;
	}

	if (self->zero_terminated && (index + len == self->len - 1))
	{
		msg_warn("can't remove range [%lu:%lu] since it's overlapping terminating zero!", index, index + len - 1);
		return NULL;
	}

	if (self->ff != NULL)
		for (size_t i = index; i < index + len; ++i) 
			self->ff(arr_cell(self, i));

	memmove(arr_cell(self, index), arr_cell(self, index + len), (self->len - len - index) * self->elemsize);

	self->len -= len;

	return self;
}

static void Array_sort(Array *self, CmpFunc cmp_func)
{
	if (self->len <= 1)
		return;

	size_t len = (self->zero_terminated) ? (self->len - 1) : (self->len);

	quicksort(self->mass, len, self->elemsize, cmp_func);
}

static bool Array_binary_search(Array *self, const void *target, CmpFunc cmp_func, size_t *index)
{
	if (self->len == 0)
		return false;

	size_t len = (self->zero_terminated) ? (self->len - 1) : (self->len);

	if (len < BINARY_SEARCH_LEN_THRESHOLD)
		return linear_search(self->mass, target, len, self->elemsize, cmp_func, index);

	quicksort(self->mass, len, self->elemsize, cmp_func);
	return binary_search(self->mass, target, 0, len - 1, self->elemsize, cmp_func, index);
}

static Array* Array_remove_val(Array *self, const void *target, CmpFunc cmp_func, bool remove_all)
{
	bool was_deleted = false;
	size_t index;

	while (Array_binary_search(self, target, cmp_func, &index)) 
	{
		Array_remove_index(self, index);
		was_deleted = true;

		if (!remove_all)
			break;
	}

	if (!was_deleted)
		return NULL;

	return self;
}

static Array* Array_unique(Array *self, CmpFunc cmp_func)
{
	Array *result = array_new(self->clear, self->zero_terminated, self->elemsize);

	return_val_if_fail(result != NULL, NULL);

	if (self->len == 0)
		return result;

	size_t len = (self->zero_terminated) ? (self->len - 1) : (self->len);

	if (len == 1)
	{
		Array_append(result, arr_cell(self, 0));
		return result;
	}

	quicksort(self->mass, len, self->elemsize, cmp_func);

	size_t result_last = 0;
	Array_append(result, arr_cell(self, 0));

	for (size_t i = 1; i < len; ++i) 
	{
		if (cmp_func(arr_cell(self, i), arr_cell(result, result_last)) != 0)
		{
			Array_append(result, arr_cell(self, i));
			result_last++;
		}
	}

	return result;
}

static void Array_string(const Stringer *_self, va_list *ap)
{
	const Array *self = ARRAY((Object*) _self);

	StringFunc str_func = va_arg(*ap, StringFunc);

	if (str_func == NULL)
		return;

	printf("[");
	
	for (size_t i = 0; i < self->len; ++i) 
	{
		va_list ap_copy;
		va_copy(ap_copy, *ap);
		
		str_func(arr_cell(self, i), &ap_copy);
		if (i + 1 != self->len)
			printf(" ");
		
		va_end(ap_copy);
	}

	printf("]");
}

static void* Array_steal(Array *self, size_t *len)
{
	void *ret = calloc(self->len, self->elemsize);
	return_val_if_fail(ret != NULL, NULL);

	memcpy(ret, self->mass, self->len * self->elemsize);
	
	if (len != NULL)
		*len = self->len;

	self->len = 0;
	memset(self->mass, 0, self->len * self->elemsize);

	return ret;
}

/* }}} */

/* Selectors {{{ */

Array* array_new(bool clear, bool zero_terminated, size_t elemsize)
{
	return_val_if_fail(elemsize != 0, NULL);
	return (Array*)object_new(ARRAY_TYPE, clear, zero_terminated, elemsize);
}

Array* array_set(Array *self, size_t index, const void *data)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return (Array*)object_set((Object*) self, index, data);
}

void array_get(const Array *self, size_t index, void *ret)
{
	return_if_fail(IS_ARRAY(self));
	return_if_fail(ret != NULL);
	object_get((const Object*) self, index, ret);
}

Array* array_copy(const Array *self)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return (Array*)object_copy((const Object*) self);
}

void array_delete(Array *self)
{
	return_if_fail(IS_ARRAY(self));
	object_delete((Object*) self);
}

Array* array_append(Array *self, const void *data)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return Array_append(self, data);
}

Array* array_prepend(Array *self, const void *data)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return Array_prepend(self, data);
}

Array* array_insert(Array *self, size_t index, const void *data)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return Array_insert(self, index, data);
}

Array* array_append_many(Array *self, const void *data, size_t len)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return Array_append_many(self, data, len);
}

Array* array_prepend_many(Array *self, const void *data, size_t len)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return Array_prepend_many(self, data, len);
}

Array* array_insert_many(Array *self, size_t index, const void *data, size_t len)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return Array_insert_many(self, index, data, len);
}

Array* array_remove_index(Array *self, size_t index)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return Array_remove_index(self, index);
}

Array* array_remove_val(Array *self, const void *target, CmpFunc cmp_func, bool remove_all)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return_val_if_fail(cmp_func != NULL, NULL);
	return Array_remove_val(self, target, cmp_func, remove_all);
}

Array* array_remove_range(Array *self, size_t index, size_t len)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return Array_remove_range(self, index, len);
}

void array_sort(Array *self, CmpFunc cmp_func)
{
	return_if_fail(IS_ARRAY(self));
	return_if_fail(cmp_func != NULL);
	Array_sort(self, cmp_func);
}

bool array_binary_search(Array *self, const void *target, CmpFunc cmp_func, size_t *index)
{
	return_val_if_fail(IS_ARRAY(self), false);
	return_val_if_fail(cmp_func != NULL, false);
	return Array_binary_search(self, target, cmp_func, index);
}

Array* array_unique(Array *self, CmpFunc cmp_func)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return_val_if_fail(cmp_func != NULL, NULL);
	return Array_unique(self, cmp_func);
}

void array_set_free_func(Array *self, FreeFunc free_func)
{
	return_if_fail(IS_ARRAY(self));
	self->ff = free_func;
}

void* array_steal(Array *self, size_t *len)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return Array_steal(self, len);
}

ssize_t array_get_length(const Array *self)
{
	return_val_if_fail(IS_ARRAY(self), -1);
	return self->len;
}

/* }}} */

/* Init {{{ */

static void stringer_interface_init(StringerInterface *iface)
{
	iface->string = Array_string;
}

static void array_class_init(ArrayClass *klass)
{
	OBJECT_CLASS(klass)->ctor = Array_ctor;
	OBJECT_CLASS(klass)->dtor = Array_dtor;
	OBJECT_CLASS(klass)->set = Array_set;
	OBJECT_CLASS(klass)->get = Array_get;
	OBJECT_CLASS(klass)->cpy = Array_cpy;
}

/* }}} */

/* vim: set fdm=marker : */
