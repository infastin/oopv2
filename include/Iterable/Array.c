#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Interface.h"
#include "Messages.h"
#include "Array.h"
#include "Object.h"
#include "StringerInterface.h"
#include "Utils.h"

/* Predefinitions {{{ */

static void stringer_interface_init(StringerInterface *iface);

DEFINE_TYPE_WITH_IFACES(Array, array, object, 1,
		USE_INTERFACE(STRINGER_INTERFACE_TYPE, stringer_interface_init));

typedef struct _ArrayData ArrayData;

struct _ArrayData
{
	bool clear;
	bool zero_terminated;
	FreeFunc ff;
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
static inline void inssort(void *mass, size_t len, size_t elemsize, CmpFunc cmp_func)
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

static inline void heapsort(void *mass, size_t len, size_t elemsize, CmpFunc cmp_func)
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

static void quicksort_recursive(void *mass, size_t orig_left, size_t orig_right, size_t elemsize, CmpFunc cmp_func)
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
			left = new_pivot + 1;
		}
	}
}

/* }}} */

/* Binary Search {{{ */

#define BINARY_SEARCH_LEN_THRESHOLD 32

static bool linear_search(void *mass, const void* target, size_t len, size_t elemsize, CmpFunc cmp_func, size_t *index)
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

static bool binary_search(void *mass, const void* target, size_t left, size_t right, size_t elemsize, CmpFunc cmp_func, size_t *index)
{
	while (left <= right) 
	{
		size_t mid = left + ((right - left) >> 1);
		
		if (cmp_func(mass_cell(mass, elemsize, mid), target) == 0)
		{
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
		memset(arr_cell(self, arr_data(self)->capacity), 0, (mincap - arr_data(self)->capacity) * arr_data(self)->elemsize);

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
		memset(arr_cell(self, arr_data(self)->capacity), 0, (mincap - arr_data(self)->capacity) * arr_data(self)->elemsize);

	arr_data(self)->capacity = mincap;
}

static Array* _Array_insert(Array *self, size_t index, const void *data)
{
	int zt = 0;

	if (arr_data(self)->zero_terminated)
		zt = 1;

	if (index + zt >= arr_data(self)->capacity)
	{
		_Array_growcap(self, (index + zt + 1) - arr_data(self)->capacity);

		if (index + zt >= arr_data(self)->capacity)
			return NULL;
	}
	else if (self->len >= arr_data(self)->capacity)
	{
		_Array_growcap(self, 1);

		if (self->len >= arr_data(self)->capacity)
			return NULL;
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

	memcpy(arr_cell(self, index), data, arr_data(self)->elemsize);

	return self;
}

static Array* _Array_insert_many(Array *self, size_t index, const void *data, size_t len)
{
	int zt = 0;

	if (arr_data(self)->zero_terminated)
		zt = 1;

	if (index + zt + len >= arr_data(self)->capacity)
	{
		_Array_growcap(self, (index + zt + len) - arr_data(self)->capacity);

		if (index + zt + len >= arr_data(self)->capacity)
			return NULL;
	}
	else if (self->len + len >= arr_data(self)->capacity)
	{
		_Array_growcap(self, len);

		if (self->len + len >= arr_data(self)->capacity)
			return NULL;
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

	return self;
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
		self->data = calloc(1, sizeof(ArrayData) + elemsize);
	else
		self->data = malloc(sizeof(ArrayData) + elemsize);

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

	return _self;
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

	memcpy(arr_mass(object), arr_mass(self), object->len * arr_data(object)->elemsize);

	return _object;
}

static Object* Array_set(Object *_self, va_list *ap)
{
	Array *self = ARRAY(_self);

	size_t index = va_arg(*ap, size_t);
	const void *data = va_arg(*ap, const void*);

	int zt = 0;

	if (arr_data(self)->zero_terminated)
		zt = 1;

	if (index + zt >= arr_data(self)->capacity)
	{
		_Array_growcap(self, (index + zt + 1) - arr_data(self)->capacity);

		if (index >= arr_data(self)->capacity)
			return NULL;
	}

	memcpy(arr_cell(self, index), data, arr_data(self)->elemsize);

	if (index + 1 >= self->len && zt) 
		memset(arr_cell(self, index + 1), 0, arr_data(self)->elemsize);

	if (index + zt >= self->len)
		self->len = index + zt + 1;

	return _self;
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

	memcpy(ret, arr_cell(self, index), arr_data(self)->elemsize);
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
	if (arr_data(self)->zero_terminated)
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
	if (arr_data(self)->zero_terminated)
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

static Array* Array_remove_index(Array *self, size_t index, bool to_free)
{
	if (index >= self->len)
	{
		msg_warn("element at [%lu] is out of bounds!", index);
		return NULL;
	}

	if (arr_data(self)->zero_terminated && (index == self->len - 1))
	{
		msg_warn("can't remove terminating zero at [%lu]!", index);
		return NULL;
	}

	if (to_free)
		arr_data(self)->ff(*((void**) arr_cell(self, index)));

	memmove(arr_cell(self, index), arr_cell(self, index + 1), (self->len - index - 1) * arr_data(self)->elemsize);
	
	self->len--;

	return self;
}

static Array* Array_remove_range(Array *self, size_t index, size_t len, bool to_free)
{
	if (index + len >= self->len)
	{
		msg_warn("range [%lu:%lu] is out of bounds!", index, index + len - 1);
		return NULL;
	}

	if (arr_data(self)->zero_terminated && (index + len == self->len - 1))
	{
		msg_warn("can't remove range [%lu:%lu] since it's overlapping terminating zero!", index, index + len - 1);
		return NULL;
	}

	if (to_free)
		for (size_t i = index; i < index + len; ++i) 
			arr_data(self)->ff(arr_cell(self, i));

	memmove(arr_cell(self, index), arr_cell(self, index + len), (self->len - len - index) * arr_data(self)->elemsize);

	self->len -= len;

	return self;
}

static void Array_sort(Array *self, CmpFunc cmp_func)
{
	if (self->len <= 1)
		return;

	size_t len = (arr_data(self)->zero_terminated) ? (self->len - 1) : (self->len);

	quicksort_recursive(arr_mass(self), 0, len - 1, arr_data(self)->elemsize, cmp_func);
}

static bool Array_binary_search(Array *self, const void *target, CmpFunc cmp_func, size_t *index)
{
	if (self->len == 0)
		return false;

	size_t len = (arr_data(self)->zero_terminated) ? (self->len - 1) : (self->len);

	if (len < BINARY_SEARCH_LEN_THRESHOLD)
		return linear_search(arr_mass(self), target, len, arr_data(self)->elemsize, cmp_func, index);

	quicksort_recursive(arr_mass(self), 0, len - 1, arr_data(self)->elemsize, cmp_func);
	return binary_search(arr_mass(self), target, 0, len - 1, arr_data(self)->elemsize, cmp_func, index);
}

static Array* Array_remove_val(Array *self, const void *target, CmpFunc cmp_func, bool to_free, bool remove_all)
{
	bool was_deleted = false;
	size_t index;

	while (Array_binary_search(self, target, cmp_func, &index)) 
	{
		Array_remove_index(self, index, to_free);
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
	Array *result = array_new(arr_data(self)->clear, arr_data(self)->zero_terminated, arr_data(self)->elemsize);

	return_val_if_fail(result != NULL, NULL);

	if (self->len == 0)
		return result;

	size_t len = (arr_data(self)->zero_terminated) ? (self->len - 1) : (self->len);

	if (len == 1)
	{
		Array_append(result, arr_cell(self, 0));
		return result;
	}

	quicksort_recursive(arr_mass(self), 0, len - 1, arr_data(self)->elemsize, cmp_func);

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

static void Array_set_free_func(Array *self, FreeFunc free_func)
{
	arr_data(self)->ff = free_func;
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
	void *ret = calloc(self->len, arr_data(self)->elemsize);
	return_val_if_fail(ret != NULL, NULL);

	memcpy(ret, arr_mass(self), self->len * arr_data(self)->elemsize);
	*len = self->len;

	self->len = 0;
	memset(arr_mass(self), 0, self->len * arr_data(self)->elemsize);

	return ret;
}

/* }}} */

/* Selectors {{{ */

Array* array_set(Array *self, size_t index, const void *data)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return (Array*) object_set((Object*) self, index, data);
}

void array_get(Array *self, size_t index, void *ret)
{
	return_if_fail(IS_ARRAY(self));
	object_get((Object*) self, index, ret);
}

Array* array_copy(Array *self)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	return (Array*) object_copy((Object*) self);
}

Array* array_append(Array *self, const void *data)
{
	return_val_if_fail(IS_ARRAY(self), NULL);

	ArrayClass *klass = ARRAY_GET_CLASS(self);

	return_val_if_fail(klass->append != NULL, NULL);
	return klass->append(self, data);
}

Array* array_prepend(Array *self, const void *data)
{
	return_val_if_fail(IS_ARRAY(self), NULL);

	ArrayClass *klass = ARRAY_GET_CLASS(self);

	return_val_if_fail(klass->prepend != NULL, NULL);
	return klass->prepend(self, data);
}

Array* array_insert(Array *self, size_t index, const void *data)
{
	return_val_if_fail(IS_ARRAY(self), NULL);

	ArrayClass *klass = ARRAY_GET_CLASS(self);

	return_val_if_fail(klass->insert != NULL, NULL);
	return klass->insert(self, index, data);
}

Array* array_append_many(Array *self, const void *data, size_t len)
{
	return_val_if_fail(IS_ARRAY(self), NULL);

	ArrayClass *klass = ARRAY_GET_CLASS(self);

	return_val_if_fail(klass->append_many != NULL, NULL);
	return klass->append_many(self, data, len);
}

Array* array_prepend_many(Array *self, const void *data, size_t len)
{
	return_val_if_fail(IS_ARRAY(self), NULL);

	ArrayClass *klass = ARRAY_GET_CLASS(self);

	return_val_if_fail(klass->prepend_many != NULL, NULL);
	return klass->prepend_many(self, data, len);
}

Array* array_insert_many(Array *self, size_t index, const void *data, size_t len)
{
	return_val_if_fail(IS_ARRAY(self), NULL);

	ArrayClass *klass = ARRAY_GET_CLASS(self);

	return_val_if_fail(klass->insert_many != NULL, NULL);
	return klass->insert_many(self, index, data, len);
}

Array* array_remove_index(Array *self, size_t index, bool to_free)
{
	return_val_if_fail(IS_ARRAY(self), NULL);

	ArrayClass *klass = ARRAY_GET_CLASS(self);

	return_val_if_fail(klass->remove_index != NULL, NULL);
	return klass->remove_index(self, index, to_free);
}

Array* array_remove_val(Array *self, const void *target, CmpFunc cmp_func, bool to_free, bool remove_all)
{
	return_val_if_fail(IS_ARRAY(self), NULL);

	ArrayClass *klass = ARRAY_GET_CLASS(self);

	return_val_if_fail(klass->remove_val != NULL, NULL);
	return klass->remove_val(self, target, cmp_func, to_free, remove_all);
}

Array* array_remove_range(Array *self, size_t index, size_t len, bool to_free)
{
	return_val_if_fail(IS_ARRAY(self), NULL);

	ArrayClass *klass = ARRAY_GET_CLASS(self);

	return_val_if_fail(klass->remove_range != NULL, NULL);
	return klass->remove_range(self, index, len, to_free);
}

void array_sort(Array *self, CmpFunc cmp_func)
{
	return_if_fail(IS_ARRAY(self));


	ArrayClass *klass = ARRAY_GET_CLASS(self);

	return_if_fail(klass->sort != NULL);
	klass->sort(self, cmp_func);
}

bool array_binary_search(Array *self, const void *target, CmpFunc cmp_func, size_t *index)
{
	return_val_if_fail(IS_ARRAY(self), false);

	ArrayClass *klass = ARRAY_GET_CLASS(self);

	return_val_if_fail(klass->binary_search != NULL, false);
	return klass->binary_search(self, target, cmp_func, index);
}

Array* array_unique(Array *self, CmpFunc cmp_func)
{
	return_val_if_fail(IS_ARRAY(self), NULL);

	ArrayClass *klass = ARRAY_GET_CLASS(self);

	return_val_if_fail(klass->unique != NULL, NULL);
	return klass->unique(self, cmp_func);
}

void array_set_free_func(Array *self, FreeFunc free_func)
{
	return_if_fail(IS_ARRAY(self));

	ArrayClass *klass = ARRAY_GET_CLASS(self);

	return_if_fail(klass->set_free_func != NULL);
	klass->set_free_func(self, free_func);
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

void *array_steal(Array *self, size_t *len)
{
	return_val_if_fail(IS_ARRAY(self), NULL);
	
	ArrayClass *klass = ARRAY_GET_CLASS(self);
	
	return_val_if_fail(klass->steal != NULL, NULL);
	return klass->steal(self, len);
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

	klass->append = Array_append;
	klass->prepend = Array_prepend;
	klass->insert = Array_insert;
	klass->set_free_func = Array_set_free_func;
	klass->sort = Array_sort;
	klass->unique = Array_unique;
	klass->remove_index = Array_remove_index;
	klass->remove_val = Array_remove_val;
	klass->append_many = Array_append_many;
	klass->prepend_many = Array_prepend_many;
	klass->insert_many = Array_insert_many;
	klass->remove_range = Array_remove_range;
	klass->binary_search = Array_binary_search;
	klass->steal = Array_steal;
}

/* }}} */

/* vim: set fdm=marker : */
