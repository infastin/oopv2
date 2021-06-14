#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Interface.h"
#include "Messages.h"
#include "Array.h"
#include "Object.h"
#include "IterableInterface.h"
#include "StringerInterface.h"
#include "Utils.h"

/* Predefinitions {{{ */

void iterable_interface_init(IterableInterface *iface);
void stringer_interface_init(StringerInterface *iface);

DEFINE_TYPE_WITH_IFACES(Array, array, object, 2,
		USE_INTERFACE(ITERABLE_INTERFACE_TYPE, iterable_interface_init),
		USE_INTERFACE(STRINGER_INTERFACE_TYPE, stringer_interface_init));

typedef struct _ArrayData ArrayData;

struct _ArrayData
{
	bool clear;
	bool zero_terminated;
	free_f ff;
	size_t elemsize;
	size_t capacity;
};

#define arr_mass(s) ((char*) &((ArrayData*) (s->data))[1])
#define arr_cell(s, i) (&((char*) &((ArrayData*) (s->data))[1])[(i) * (arr_data(self)->elemsize)])
#define arr_data(s) ((ArrayData*) (s->data))
#define mass_cell(m, e, i) (&((char*) m)[(i) * (e)])

/* }}} */

/* Sorting {{{ */

#define SORT_LEN_THRESHOLD 16

#define SWAP(a, b, elemsize) 				\
	{ 										\
		size_t __size = (elemsize); 		\
		char *__a = (a); char *__b = (b); 	\
		do 									\
		{ 									\
			char __tmp = *__a; 				\
			*__a++ = *__b; 					\
			*__b++ = __tmp; 				\
		} while (--__size > 0); 			\
	}

/* Insertion sort */
static inline void inssort(char *mass, size_t len, size_t elemsize, cmp_f cmp_func)
{
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
static inline void heap(char *mass, size_t start, size_t end, size_t elemsize, cmp_f cmp_func)
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

static inline void heapify(char *mass, size_t len, size_t elemsize, cmp_f cmp_func)
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

static inline void heapsort(char *mass, size_t len, size_t elemsize, cmp_f cmp_func)
{
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
static inline size_t quicksort_partition(char *mass, size_t left, size_t right, size_t pivot, size_t elemsize, cmp_f cmp_func)
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

static inline size_t find_median(char *mass, size_t a, size_t b, size_t c, size_t elemsize, cmp_f cmp_func)
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

static void quicksort_recursive(char *mass, size_t orig_left, size_t orig_right, size_t elemsize, cmp_f cmp_func)
{
	size_t left = orig_left;
	size_t right = orig_right;
	size_t mid;

	size_t pivot;
	size_t new_pivot;

	int loop_count = 0;
	const int max_loops = 64 - __builtin_clzll(right - left);

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
		}
	}
}

/* }}} */

/* Binary Search {{{ */

#define BINARY_SEARCH_LEN_THRESHOLD 32

static bool linear_search(char *mass, const void* target, size_t len, size_t elemsize, cmp_f cmp_func, size_t *index)
{
	for (size_t i = 0; i < len; ++i) 
	{
		if (cmp_func(mass_cell(mass, elemsize, i), target) == 0)
		{
			*index = i;
			return true;
		}
	}

	return false;
}

static bool binary_search(char *mass, const void* target, size_t left, size_t right, size_t elemsize, cmp_f cmp_func, size_t *index)
{
	while (left <= right) 
	{
		size_t mid = left + ((right - left) >> 1);
		
		if (cmp_func(mass_cell(mass, elemsize, mid), target) == 0)
		{
			*index = mid;
			return true;
		}

		if (cmp_func(mass_cell(mass, elemsize, mid), target) > 0)
			right = mid - 1;
		else
			left = mid + 1;
	}

	return false;
}

/* }}} */

/* Private methods {{{ */

static void _Array_growlen(Array *self, size_t add)
{
	if ((self->len + add) <= arr_data(self)->capacity)
		return;

	size_t mincap = self->len + add;

	size_t cap1 = (size_t) ((double) arr_data(self)->capacity * 1.25);
	size_t cap2 = arr_data(self)->capacity * 4;
	size_t cap3 = arr_data(self)->capacity * 8;

	if (mincap > 64 && mincap < cap1)
		mincap = cap1;
	else if (mincap > 16 && mincap < cap2)
		mincap = cap2;
	else if (mincap < cap3)
		mincap = cap3;
	else
		mincap = pow2l(mincap);

	void *data;

	data = (void*)realloc(self->data, sizeof(ArrayData) + mincap * arr_data(self)->elemsize);

	if (data == NULL)
	{
		msg_error("couldn't reallocate memory for array!");
		return;
	}

	self->data = data;

	if (arr_data(self)->clear)
		memset(arr_cell(self, arr_data(self)->capacity), 0, mincap - arr_data(self)->capacity);

	arr_data(self)->capacity = mincap;
}

static void _Array_growcap(Array *self, size_t add)
{
	if (add == 0)
		return;

	size_t mincap = arr_data(self)->capacity + add;

	size_t cap1 = (size_t) ((double) arr_data(self)->capacity * 1.25);
	size_t cap2 = arr_data(self)->capacity * 4;
	size_t cap3 = arr_data(self)->capacity * 8;

	if (mincap > 64 && mincap < cap1)
		mincap = cap1;
	else if (mincap > 16 && mincap < cap2)
		mincap = cap2;
	else if (mincap < cap3)
		mincap = cap3;
	else
		mincap = pow2l(mincap);

	void *data = (void*)realloc(self->data, sizeof(ArrayData) + mincap * arr_data(self)->elemsize);

	if (data == NULL)
	{
		msg_error("couldn't reallocate memory for array!");
		return;
	}

	self->data = data;

	if (arr_data(self)->clear)
		memset(arr_cell(self, arr_data(self)->capacity), 0, mincap - arr_data(self)->capacity);

	arr_data(self)->capacity = mincap;
}

static void _Array_insert_val(Array *self, size_t index, const void *value)
{
	int zt = 0;

	if (arr_data(self)->zero_terminated)
		zt = 1;

	if (index + zt >= arr_data(self)->capacity)
	{
		_Array_growcap(self, (index + zt + 1) - arr_data(self)->capacity);

		if (index + zt >= arr_data(self)->capacity)
			return;
	}
	else if (self->len >= arr_data(self)->capacity)
	{
		_Array_growcap(self, 1);

		if (self->len >= arr_data(self)->capacity)
			return;
	}

	if (index + 1 >= self->len && zt)
	{
		memset(arr_cell(self, index + 1), 0, arr_data(self)->elemsize);

		self->len = index + zt + 1;
	}
	else if (index >= self->len)
	{
		self->len = index + 1;
	}
	else
	{
		memmove(arr_cell(self, index + 1), arr_cell(self, index), (self->len - index) * arr_data(self)->elemsize);
		self->len++;
	}

	memcpy(arr_cell(self, index), (char*) value, arr_data(self)->elemsize);
}

static void _Array_insert_vals(Array *self, size_t index, const void *data, size_t len)
{
	int zt = 0;

	if (arr_data(self)->zero_terminated)
		zt = 1;

	if (index + zt + len >= arr_data(self)->capacity)
	{
		_Array_growcap(self, (index + zt + len) - arr_data(self)->capacity);

		if (index + zt + len >= arr_data(self)->capacity)
		{
			printf("\n%lu\n", index + zt + len);
			return;
		}
	}
	else if (self->len + len >= arr_data(self)->capacity)
	{
		_Array_growcap(self, len);

		if (self->len + len >= arr_data(self)->capacity)
			return;

	}

	if (index + 1 >= self->len && zt)
	{
		self->len = index + len + zt;
		memset(arr_cell(self, index + len), 0, arr_data(self)->elemsize);
	}
	else if (index >= self->len)
	{
		self->len = index + len;
	}
	else
	{
		memmove(arr_cell(self, index + len), arr_cell(self, index), (self->len - index) * arr_data(self)->elemsize);
		self->len += len;
	}

	for (size_t i = 0; i < len; ++i) 
		memcpy(arr_cell(self, index + i), mass_cell(data, arr_data(self)->elemsize, i), arr_data(self)->elemsize);
}

/* }}} */

/* Public methods {{{ */

static Object* Array_ctor(Object *_self, va_list *ap)
{
	Array *self = ARRAY(object_super_ctor(ARRAY_TYPE, _self, ap));

	bool clear = (bool) va_arg(*ap, int);
	bool zero_terminated = (bool) va_arg(*ap, int);
	size_t elemsize = va_arg(*ap, size_t);

	if (clear)
		self->data = (char*)calloc(1, sizeof(ArrayData) + elemsize);
	else
		self->data = (char*)malloc(sizeof(ArrayData) + elemsize);

	if (self->data == NULL)
	{
		object_delete((Object*) self);
		msg_error("couldn't allocate memory for array!");
		return NULL;
	}

	ArrayData *ad = (ArrayData*) self->data;
	ad->clear = clear;
	ad->zero_terminated = zero_terminated;
	ad->ff = free;
	ad->elemsize = elemsize;
	ad->capacity = 1;
	
	self->len = 0;

	return (Object*) self;
}

static Object* Array_dtor(Object *_self, va_list *ap)
{
	Array *self = ARRAY(_self);

	bool free_segment = (bool) va_arg(*ap, int);

	if (self->data != NULL)
	{
		if (free_segment)
			for (size_t i = 0; i < self->len; ++i) 
				arr_data(self)->ff(arr_cell(self, i));

		free(self->data);
	}

	return _self;
}

static Object* Array_cpy(const Object *_self, Object *_object)
{
	const Array *self = ARRAY(_self);
	Array *object = (Array*) object_super_cpy(ARRAY_TYPE, _self, _object);

	object->data = calloc(1, sizeof(ArrayData) + arr_data(self)->capacity * arr_data(self)->elemsize);

	if (object->data == NULL)
	{
		object_delete((Object*) object);
		msg_error("couldn't allocate memory for the copy of array!");
		return NULL;
	}

	arr_data(object)->clear = arr_data(self)->clear;
	arr_data(object)->zero_terminated = arr_data(self)->zero_terminated;
	arr_data(object)->capacity = arr_data(self)->capacity;
	arr_data(object)->elemsize = arr_data(self)->elemsize;

	object->len = self->len;

	return (Object*) object;
}

static void Array_set(Object *_self, va_list *ap)
{
	Array *self = ARRAY(_self);

	size_t index = va_arg(*ap, size_t);
	const void *value = va_arg(*ap, const void*);

	int zt = 0;

	if (arr_data(self)->zero_terminated)
		zt = 1;

	if (index + zt >= arr_data(self)->capacity)
	{
		_Array_growcap(self, (index + zt + 1) - arr_data(self)->capacity);

		if (index >= arr_data(self)->capacity)
			return;
	}

	memcpy(arr_cell(self, index), (char*) value, arr_data(self)->elemsize);

	if (index + 1 >= self->len && zt) 
		memset(arr_cell(self, index + 1), 0, arr_data(self)->elemsize);

	if (index + zt >= self->len)
		self->len = index + zt + 1;
}

static void Array_get(const Object *_self, va_list *ap)
{
	const Array *self = ARRAY(_self);

	size_t index = va_arg(*ap, size_t);
	void *ret = va_arg(*ap, void*);

	if (index >= self->len)
	{
		msg_warn("element at [%lu] is out of bounds!", index);
		return;
	}

	memcpy((char*) ret, arr_cell(self, index), arr_data(self)->elemsize);
}

static void Array_insert_val(Iterable *_self, size_t index, const void *value)
{
	Array *self = ARRAY((Object*) _self);

	_Array_insert_val(self, index, value);
}

static void Array_insert_vals(Iterable *_self, size_t index, const void *data, size_t len)
{
	Array *self = ARRAY((Object*) _self);

	_Array_insert_vals(self, index, data, len);
}

static void Array_append_val(Iterable *_self, const void *value)
{
	Array *self = ARRAY((Object*) _self);

	if (arr_data(self)->zero_terminated)
	{
		if (self->len == 0)
			_Array_insert_val(self, self->len, value);
		else
			_Array_insert_val(self, self->len - 1, value);
	}
	else
		_Array_insert_val(self, self->len, value);
}

static void Array_append_vals(Iterable *_self, const void *data, size_t len)
{
	Array *self = ARRAY((Object*) _self);

	if (arr_data(self)->zero_terminated)
	{
		if (self->len == 0)
			_Array_insert_vals(self, self->len, data, len);
		else
			_Array_insert_vals(self, self->len - 1, data, len);
	}
	else
		_Array_insert_vals(self, self->len, data, len);
}

static void Array_prepend_val(Iterable *_self, const void *value)
{
	Array *self = ARRAY((Object*) _self);

	_Array_insert_val(self, 0, value);
}

static void Array_prepend_vals(Iterable *_self, const void *data, size_t len)
{
	Array *self = ARRAY((Object*) _self);

	_Array_insert_vals(self, 0, data, len);
}

static void Array_remove_index(Iterable *_self, size_t index, bool to_free)
{
	Array *self = ARRAY((Object*) _self);

	if (index >= self->len)
	{
		msg_warn("element at [%lu] is out of bounds!", index);
		return;
	}

	if (arr_data(self)->zero_terminated && (index == self->len - 1))
	{
		msg_warn("can't remove terminating zero at [%lu]!", index);
		return;
	}

	if (to_free)
		arr_data(self)->ff(*((void**) arr_cell(self, index)));

	memmove(arr_cell(self, index), arr_cell(self, index + 1), (self->len - index - 1) * arr_data(self)->elemsize);
	
	self->len--;
}

static void Array_remove_range(Iterable *_self, size_t index, size_t len, bool to_free)
{
	Array *self = ARRAY((Object*) _self);

	if (index + len >= self->len)
	{
		msg_warn("range [%lu:%lu] is out of bounds!", index, index + len - 1);
		return;
	}

	if (arr_data(self)->zero_terminated && (index + len == self->len - 1))
	{
		msg_warn("can't remove range [%lu:%lu] since it's overlapping terminating zero!", index, index + len - 1);
		return;
	}

	if (to_free)
		for (size_t i = index; i < index + len; ++i) 
			arr_data(self)->ff(arr_cell(self, i));

	memmove(arr_cell(self, index), arr_cell(self, index + len), (self->len - len - index) * arr_data(self)->elemsize);

	self->len -= len;
}

static void Array_sort(Iterable *_self, cmp_f cmp_func)
{
	Array *self = ARRAY((Object*) _self);

	if (self->len <= 1)
		return;

	size_t len = (arr_data(self)->zero_terminated) ? (self->len - 1) : (self->len);

	quicksort_recursive(arr_mass(self), 0, len - 1, arr_data(self)->elemsize, cmp_func);
}

static bool Array_binary_search(Iterable *_self, const void *target, cmp_f cmp_func, size_t *index)
{
	Array *self = ARRAY((Object*) _self);

	if (self->len == 0)
		return false;

	size_t len = (arr_data(self)->zero_terminated) ? (self->len - 1) : (self->len);

	if (len < BINARY_SEARCH_LEN_THRESHOLD)
		return linear_search(arr_mass(self), target, len, arr_data(self)->elemsize, cmp_func, index);

	quicksort_recursive(arr_mass(self), 0, len - 1, arr_data(self)->elemsize, cmp_func);
	return binary_search(arr_mass(self), target, 0, len - 1, arr_data(self)->elemsize, cmp_func, index);
}

static Iterable* Array_unique(Iterable *_self, cmp_f cmp_func)
{
	Array *self = ARRAY((Object*) _self);
	Array *result = array_new(arr_data(self)->clear, arr_data(self)->zero_terminated, arr_data(self)->elemsize);

	return_val_if_fail(result != NULL, NULL);

	if (self->len == 0)
		return (Iterable*) result;

	size_t len = (arr_data(self)->zero_terminated) ? (self->len - 1) : (self->len);

	if (len == 1)
	{
		Array_append_val((Iterable*) result, arr_cell(self, 0));
		return (Iterable*) result;
	}

	quicksort_recursive(arr_mass(self), 0, len - 1, arr_data(self)->elemsize, cmp_func);

	size_t result_last = 0;
	Array_append_val((Iterable*) result, arr_cell(self, 0));

	for (size_t i = 1; i < len; ++i) 
	{
		if (cmp_func(arr_cell(self, i), arr_cell(result, result_last)) != 0)
		{
			Array_append_val((Iterable*) result, arr_cell(self, i));
			result_last++;
		}
	}

	return (Iterable*) result;
}

static void Array_set_free_func(Iterable *_self, free_f free_func)
{
	Array *self = ARRAY((Object*) _self);
	arr_data(self)->ff = free_func;
}

static void Array_string(const Stringer *_self, va_list *ap)
{
	const Array *self = ARRAY((Object*) _self);

	str_f str_func = va_arg(*ap, str_f);

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

/* }}} */

/* Selectors {{{ */

void array_set(Array *self, size_t index, const void *value)
{
	return_if_fail(IS_ARRAY(self));
	object_set((Object*) self, index, value);
}

void array_get(Array *self, size_t index, void *ret)
{
	return_if_fail(IS_ARRAY(self));
	object_get((Object*) self, index, ret);
}

void array_append_val(Array *self, const void *value)
{
	return_if_fail(IS_ARRAY(self));
	iterable_append_val((Iterable*) self, value);
}

void array_prepend_val(Array *self, const void *value)
{
	return_if_fail(IS_ARRAY(self));
	iterable_prepend_val((Iterable*) self, value);
}

void array_insert_val(Array *self, size_t index, const void *value)
{
	return_if_fail(IS_ARRAY(self));
	iterable_insert_val((Iterable*) self, index, value);
}

void array_append_vals(Array *self, const void *data, size_t len)
{
	return_if_fail(IS_ARRAY(self));
	iterable_append_vals((Iterable*) self, data, len);
}

void array_prepend_vals(Array *self, const void *data, size_t len)
{
	return_if_fail(IS_ARRAY(self));
	iterable_prepend_vals((Iterable*) self, data, len);
}

void array_insert_vals(Array *self, size_t index, const void *data, size_t len)
{
	return_if_fail(IS_ARRAY(self));
	iterable_insert_vals((Iterable*) self, index, data, len);
}

void array_remove_index(Array *self, size_t index, bool to_free)
{
	return_if_fail(IS_ARRAY(self));
	iterable_remove_index((Iterable*) self, index, to_free);
}

void array_remove_range(Array *self, size_t index, size_t len, bool to_free)
{
	return_if_fail(IS_ARRAY(self));
	iterable_remove_range((Iterable*) self, index, len, to_free);
}

void array_sort(Array *self, cmp_f cmp_func)
{
	return_if_fail(IS_ARRAY(self));
	iterable_sort((Iterable*) self, cmp_func);
}

bool array_binary_search(Array *self, const void *target, cmp_f cmp_func, size_t *index)
{
	return_val_if_fail(IS_ARRAY(self), false);
	return iterable_binary_search((Iterable*) self, target, cmp_func, index);
}

Array* array_unique(Array *self, cmp_f cmp_func)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return (Array*) iterable_unique((Iterable*) self, cmp_func);
}

void array_set_free_func(Array *self, free_f free_func)
{
	return_if_fail(IS_ARRAY(self));
	iterable_set_free_func((Iterable*) self, free_func);
}

Array* array_new(bool clear, bool zero_terminated, size_t elemsize)
{
	return (Array*) object_new(ARRAY_TYPE, clear, zero_terminated, elemsize);
}

void array_delete(Array *self, bool free_segment)
{
	return_if_fail(IS_ARRAY(self));
	object_delete((Object*) self, free_segment);
}

/* }}} */

/* Init {{{ */

void iterable_interface_init(IterableInterface *iface)
{
	iface->append_val = Array_append_val;
	iface->prepend_val = Array_prepend_val;
	iface->insert_val = Array_insert_val;
	iface->append_vals = Array_append_vals;
	iface->prepend_vals = Array_prepend_vals;
	iface->insert_vals = Array_insert_vals;
	iface->remove_index = Array_remove_index;
	iface->remove_range = Array_remove_range;
	iface->sort = Array_sort;
	iface->binary_search = Array_binary_search;
	iface->unique = Array_unique;
	iface->set_free_func = Array_set_free_func;
}

void stringer_interface_init(StringerInterface *iface)
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
