#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "Base.h"
#include "DataStructs/DList.h"
#include "Interfaces/StringerInterface.h"

/* Predefinitions {{{ */

struct _DlistNode
{
	DListNode *next;
	DListNode *prev;
	void *data;
	size_t size;
};

struct _DList
{
	Object parent;
	DListNode *start;
	DListNode *end;
	FreeFunc ff;
	size_t len;
};

static void stringer_interface_init(StringerInterface *iface);

DEFINE_TYPE_WITH_IFACES(DList, dlist, object, 1, 
		USE_INTERFACE(STRINGER_INTERFACE_TYPE, stringer_interface_init));

/* }}} */

/* Private methods {{{ */

/* Sorting {{{ */

DListNode* _DList_merge(DListNode *first, DListNode *second, DListNode **end, CmpFunc cmp_func)
{
	DListNode *result = NULL;
	DListNode *prev = NULL;
	DListNode **linkp = &result;

	DListNode *f = first;
	DListNode *s = second;

	while (1) 
	{
		if (f == NULL && s == NULL)
			break;

		if (f == NULL)
		{
			s->prev = prev;
			*linkp = s;

			if (end != NULL)
				*end = s;

			break;
		}

		if (s == NULL)
		{
			f->prev = prev;
			*linkp = f;

			if (end != NULL)
				*end = f;

			break;
		}

		if (cmp_func(f->data, s->data) <= 0)
		{
			f->prev = prev;
			prev = *linkp = f;
			linkp = &f->next;
			f = f->next;
		}
		else
		{
			s->prev = prev;
			prev = *linkp = s;
			linkp = &s->next;
			s = s->next;
		}
	}

	return result;
}

DListNode* _DList_merge_sort(DListNode *start, DListNode **end, size_t len, CmpFunc cmp_func)
{
	if (start == NULL)
		return NULL;

	int arr_len = ULONG_BIT - __builtin_clzl(len);
	DListNode **arr = (DListNode**)calloc(arr_len, sizeof(DListNode*));
	DListNode *result;

	result = start;

	while (result != NULL) 
	{
		DListNode *next;
		int i;

		next = result->next;
		result->next = NULL;

		for (i = 0; (i < arr_len) && (arr[i] != NULL); i++)
		{
			result = _DList_merge(arr[i], result, NULL, cmp_func);
			arr[i] = NULL;
		}

		if (i == arr_len)
			i--;

		arr[i] = result;
		result = next;
	}

	for (size_t i = 0; i < arr_len; ++i)
		result = _DList_merge(arr[i], result, end, cmp_func);

	free(arr);

	return result;
}

/* }}} Sorting */

/* Other {{{ */

static DListNode* _DListNode_new(size_t size, const void *data)
{
	DListNode *res = salloc(DListNode, 1);
	return_val_if_fail(res != NULL, NULL);

	void *dt = malloc(size);

	if (dt == NULL)
	{
		free(res);
		return_val_if_fail(dt != NULL, NULL);
	}

	if (data != NULL)
		memcpy(dt, data, size);
	else
		memset(dt, 0, size);

	res->size = size;
	res->data = dt;
	res->next = NULL;
	res->prev = NULL;

	return res;
}

/* }}} Other */

/* Removing {{{ */

static DList* _DList_rf_val(DList *self, const void *target, CmpFunc cmp_func, bool to_all)
{
	DListNode *current = self->start;
	bool was_removed = false;

	while (current != NULL) 
	{
		if (cmp_func(current->data, target) == 0)
		{
			if (current == self->start)
			{
				self->start = current->next;

				if (self->ff != NULL)
					self->ff(*(void**) current->data);

				free(current->data);
				free(current);
				current = self->start;

				if (current == NULL)
					self->end = NULL;
			}
			else 
			{
				current->prev->next = current->next;

				if (current->next != NULL)
					current->next->prev = current->prev;
				else
					self->end = current->prev;

				DListNode *tmp = current->prev;

				if (self->ff != NULL)
					self->ff(*(void**) current->data);

				free(current->data);
				free(current);
				current = tmp->next;
			}

			self->len--;
			was_removed = true;

			if (to_all)
				continue;

			break;
		}

		current = current->next;
	}

	return (was_removed) ? self : NULL;
}

static DList* _DList_rf_sibling(DList *self, DListNode *sibling)
{
	DListNode *current = self->start;

	while (current != NULL) 
	{
		if (current == sibling)
		{
			if (current == self->start)
			{
				self->start = current->next;

				if (self->ff != NULL)
					self->ff(*(void**) current->data);

				free(current->data);
				free(current);
				current = self->start;

				if (current == NULL)
					self->end = NULL;
				else
					current->prev = NULL;
			}
			else
			{
				current->prev->next = current->next;

				if (current->next != NULL)
					current->next->prev = current->prev;
				else
					self->end = current->prev;

				if (self->ff != NULL)
					self->ff(*(void**) current->data);

				free(current->data);
				free(current);
			}

			self->len--;
			return self;
		}

		current = current->next;
	}

	return NULL;
}

/* }}} */

/* }}} */

/* Public methods {{{ */

/* Base {{{ */

static Object* DList_ctor(Object *_self, va_list *ap)
{
	DList *self = DLIST(OBJECT_CLASS(OBJECT_TYPE)->ctor(_self, ap));

	self->ff = NULL;
	self->start = NULL;
	self->end = NULL;
	self->len = 0;

	return _self;
}

static Object* DList_dtor(Object *_self, va_list *ap)
{
	DList *self = DLIST(_self);

	if (self->start != NULL)
	{
		DListNode *current = self->start;

		while (current != NULL) 
		{
			if (self->ff != NULL)
				self->ff(*(void**) current->data);

			free(current->data);

			DListNode *next = current->next;
			free(current);
			current = next;
		}
	}

	return (Object*) self;
}

static Object* DList_cpy(const Object *_self, Object *_object)
{
	const DList *self = DLIST(_self);
	DList *object = (DList*) OBJECT_CLASS(OBJECT_TYPE)->cpy(_self, _object);

	object->ff = self->ff;

	object->start = NULL;
	object->end = NULL;
	object->len = self->len;

	if (self->start == NULL)
		return (Object*) object;

	DListNode *start = _DListNode_new(self->start->size, self->start->data);

	if (start == NULL)
	{
		object_delete((Object*) object, 0);
		return_val_if_fail(start != NULL, NULL);
	}

	object->start = start;

	DListNode *s_current = self->start->next;
	DListNode *o_prev = object->start;

	while (s_current != NULL) 
	{
		DListNode *o_prev_next = _DListNode_new(s_current->size, s_current->data);

		if (o_prev_next == NULL)
		{
			object_delete((Object*) object, 0);
			return_val_if_fail(o_prev_next != NULL, NULL);
		}

		o_prev->next = o_prev_next;
		o_prev = o_prev->next;
		s_current = s_current->next;
	}

	return (Object*) object;
}

/* }}} */

/* Adding values {{{ */

static DListNode* DList_append(DList *self, const void *data, size_t size)
{
	if (self->start == NULL && self->end == NULL)
	{
		self->start = _DListNode_new(size, data);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;

		return self->start;
	}

	DListNode *end = _DListNode_new(size, data);
	return_val_if_fail(end, NULL);

	end->prev = self->end;
	self->end->next = end;
	self->end = end;
	self->len++;

	return end;
}

static DListNode* DList_prepend(DList *self, const void *data, size_t size)
{
	if (self->start == NULL && self->end == NULL)
	{
		self->start = _DListNode_new(size, data);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;

		return self->start;
	}

	DListNode *start = _DListNode_new(size, data);
	return_val_if_fail(start != NULL, NULL);

	start->next = self->start;
	self->start->prev = start;
	self->start = start;
	self->len++;

	return start;
}

static DListNode* DList_insert_before(DList *self, DListNode *sibling, const void *data, size_t size)
{
	DListNode *before = _DListNode_new(size, data);
	return_val_if_fail(before != NULL, NULL);

	if (self->start == sibling)
	{
		before->next = sibling;
		sibling->prev = before;
		self->start = before;
	}
	else
	{
		sibling->prev->next = before;
		before->prev = sibling->prev;
		before->next = sibling;
		sibling->prev = before;
	}

	self->len++;
	return before;
}

static DListNode* DList_insert(DList *self, size_t index, const void *data,  size_t size)
{
	if (self->start == NULL && self->end == NULL)
	{
		self->start = _DListNode_new(size, data);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;

		return self->start;
	}

	DListNode *node;
	DListNode *current;
	size_t i;

	node = _DListNode_new(size, data);
	return_val_if_fail(node != NULL, NULL);

	if (index <= (self->len / 2))
	{
		current = self->start;
		i = 0;

		while (current != NULL)
		{
			if (i == index)
			{
				if (current == self->start)
				{
					node->next = current;
					current->prev = current;
					self->end = node;
				}
				else
				{
					current->prev->next = node;
					node->prev = current->prev;
					node->next = current;
					current->prev = node;
				}

				self->len++;
				break;
			}

			current = current->next;
			i++;
		}
	}
	else if (index < self->len)
	{
		current = self->end;
		i = self->len;

		while (current != NULL) 
		{
			if (i == index)
			{
				current->prev->next = node;
				node->prev = current->prev;
				node->next = current;
				current->prev = node;

				self->len++;
				break;
			}

			current = current->prev;
			i--;
		}
	}
	else
	{
		current = self->end;

		current->next = node;
		node->prev = current;
		self->end = node;

		self->len++;
	}

	return node;
}

static DListNode* DList_insert_before_val(DList *self, const void *target, CmpFunc cmp_func, const void *data,  size_t size)
{
	DListNode *current = self->start;

	while (current != NULL) 
	{
		if (cmp_func(current->data, target) == 0)
		{
			DListNode *before = _DListNode_new(size, data);
			return_val_if_fail(before != NULL, NULL);

			if (self->start == current)
			{
				before->next = current;
				current->prev = before;
				self->start = before;
			}
			else
			{
				current->prev->next = before;
				before->next = current;
				before->prev = current->prev;
				current->prev = before;
			}

			self->len++;
			return before;
		}

		current = current->next;
	}

	return NULL;
}

/* }}} */

/* Removing nodes {{{ */

static DList* DList_remove_val(DList *self, const void *target, CmpFunc cmp_func, bool remove_all)
{
	return _DList_rf_val(self, target, cmp_func, remove_all);
}

static DList* DList_remove_sibling(DList *self, DListNode *sibling)
{
	return _DList_rf_sibling(self, sibling);
}

/* }}} */

/* Other {{{ */

static DListNode* DList_find(DList *self, const void *target, CmpFunc cmp_func)
{
	DListNode *current = self->start;

	while (current != NULL) 
	{
		if (cmp_func(current->data, target) == 0)
			return current;

		current = current->next;
	}

	return NULL;
}

static void DList_string(const Stringer *_self, va_list *ap)
{
	const DList *self = DLIST((const Object*) _self);

	StringFunc str_func = va_arg(*ap, StringFunc);
	return_if_fail(str_func != NULL);

	DListOutputDirection side = va_arg(*ap, DListOutputDirection);

	printf("[");

	DListNode *current;

	if (side == DLIST_OUTPUT_TO_RIGHT)
	{
		current = self->start;

		while (current != NULL) 
		{
			va_list ap_copy;
			va_copy(ap_copy, *ap);

			str_func(current->data, &ap_copy);
			if (current->next != NULL)
				printf(" ");

			va_end(ap_copy);

			current = current->next;
		}
	}
	else if (side == DLIST_OUTPUT_TO_LEFT)
	{
		current = self->end;

		while (current != NULL)
		{
			va_list ap_copy;
			va_copy(ap_copy, *ap);

			str_func(current->data, &ap_copy);
			if (current->prev != NULL)
				printf(" ");

			va_end(ap_copy);

			current = current->prev;
		}
	}

	printf("]");
}

static void DList_foreach(DList *self, JustFunc func, void *userdata)
{
	DListNode *current = self->start;

	while (current != NULL) 
	{
		func(current->data, userdata);
		current = current->next;
	}
}

static void DListNode_swap(DListNode *a, DListNode *b)
{
	void *tmp_data = a->data;
	size_t tmp_size = a->size;

	a->data = b->data;
	a->size = b->size;

	b->data = tmp_data;
	b->size = tmp_size;
}

static size_t DList_count(const DList *self, const void *target, CmpFunc cmp_func)
{
	DListNode *current = self->start;

	size_t count = 0;

	while (current != NULL)
	{
		if (cmp_func(current->data, target) == 0)
			count++;

		current = current->next;
	}

	return count;
}

static DList* DList_reverse(DList *self)
{
	DListNode *current = self->start;

	while (current != NULL) 
	{
		DListNode *tmp = current->prev;
		current->prev = current->next;
		current->next = tmp;
		current = current->prev;
	}

	DListNode *oldstart = self->start;

	oldstart->next = NULL;
	self->start = self->end;
	self->end = oldstart;

	return self;
}

/* }}} */

/* }}} */

/* Selectors {{{ */

DList* dlist_new(void)
{
	return (DList*)object_new(DLIST_TYPE);
}

DListNode* dlist_append(DList *self, const void *data, size_t size)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(size != 0, NULL);
	return DList_append(self, data, size);
}

DListNode* dlist_prepend(DList *self, const void *data, size_t size)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(size != 0, NULL);
	return DList_prepend(self, data, size);
}

DListNode* dlist_insert(DList *self, size_t index, const void *data, size_t size)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(size != 0, NULL);
	return DList_insert(self, index, data, size);
}

DList* dlist_remove_val(DList *self, const void *target, CmpFunc cmp_func, bool remove_all)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(cmp_func != NULL, NULL);
	return DList_remove_val(self, target, cmp_func, remove_all);
}

void dlist_foreach(DList *self, JustFunc func, void *userdata)
{
	return_if_fail(IS_DLIST(self));
	return_if_fail(func != NULL);
	DList_foreach(self, func, userdata);
}

void dlist_set_free_func(DList *self, FreeFunc free_func)
{
	return_if_fail(IS_DLIST(self));
	return_if_fail(free_func != NULL);
	self->ff = free_func;
}

ssize_t dlist_get_length(const DList *self)
{
	return_val_if_fail(IS_DLIST(self), -1);
	return self->len;
}

ssize_t dlist_count(const DList *self, const void *target, CmpFunc cmp_func)
{
	return_val_if_fail(IS_DLIST(self), -1);
	return_val_if_fail(cmp_func != NULL, -1);
	return DList_count(self, target, cmp_func);
}

DListNode* dlist_insert_before(DList *self, DListNode *sibling, const void *data, size_t size)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(sibling != NULL, NULL);
	return_val_if_fail(size != 0, NULL);
	return DList_insert_before(self, sibling, data, size);
}

DListNode* dlist_find(DList *self, const void *target, CmpFunc cmp_func)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(cmp_func != NULL, NULL);
	return DList_find(self, target, cmp_func);
}

DList* dlist_remove_sibling(DList *self, DListNode *sibling)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(sibling != NULL, NULL);
	return DList_remove_sibling(self, sibling);
}

void dlist_delete(DList *self, bool free_data)
{
	return_if_fail(IS_DLIST(self));
	object_delete((Object*) self, free_data);
}

DList* dlist_copy(const DList *self)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return (DList*)object_copy((const Object*) self);
}

void dlist_node_swap(DListNode *a, DListNode *b)
{
	return_if_fail(a != NULL);
	return_if_fail(b != NULL);

	DListNode_swap(a, b);
}

DListNode* dlist_insert_before_val(DList *self, const void *target, CmpFunc cmp_func, const void *data, size_t size)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(cmp_func != NULL, NULL);
	return_val_if_fail(size != 0, NULL);
	return DList_insert_before_val(self, target, cmp_func, data, size);
}

DListNode* dlist_node_set(DListNode *self, const void *data, size_t size)
{
	return_val_if_fail(size != 0, NULL);
	void *new_data = malloc(size);
	return_val_if_fail(new_data != NULL, NULL);

	self->data = new_data;
	self->size = size;

	if (data != NULL)
		memcpy(self->data, data, self->size);
	else
		memset(self->data, 0, self->size);

	return self;
}

void dlist_node_data(const DListNode *self, void *ret)
{
	return_if_fail(self != NULL);
	return_if_fail(ret != NULL);
	memcpy(ret, self->data, self->size);
}

DListNode* dlist_node_next(const DListNode *self)
{
	return_val_if_fail(self != NULL, NULL);
	return self->next;
}

DListNode* dlist_node_prev(const DListNode *self)
{
	return_val_if_fail(self != NULL, NULL);
	return self->prev;
}

void dlist_sort(DList *self, CmpFunc cmp_func)
{
	return_if_fail(IS_DLIST(self));
	return_if_fail(cmp_func != NULL);

	self->start = _DList_merge_sort(self->start, &self->end, self->len, cmp_func);

	if (self->end->next != NULL)
	{
		DListNode *new_end = self->end;
		for (; new_end->next != NULL; new_end = new_end->next);
		self->end = new_end;
	}
}

DList* dlist_reverse(DList *self)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return DList_reverse(self);
}

/* }}} */

/* Init {{{ */

static void stringer_interface_init(StringerInterface *iface)
{
	iface->string = DList_string;
}

static void dlist_class_init(DListClass *klass)
{
	OBJECT_CLASS(klass)->ctor = DList_ctor;
	OBJECT_CLASS(klass)->dtor = DList_dtor;
	OBJECT_CLASS(klass)->cpy = DList_cpy;
}

/* }}} */
