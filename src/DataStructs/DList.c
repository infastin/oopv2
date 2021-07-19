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

struct _DList
{
	Object parent;
	DListNode *start;
	DListNode *end;
	FreeFunc ff; // Node free func
	CpyFunc cpf; // Node cpy func
	size_t size;
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

		if (cmp_func(f, s) <= 0)
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

static DListNode* _DListNode_new(size_t size)
{
	DListNode *res = (DListNode*)calloc(1, size);
	return_val_if_fail(res != NULL, NULL);

	res->next = NULL;
	res->prev = NULL;

	return res;
}

static void _DListNode_swap_case1(DListNode *a, DListNode *b)
{
	DListNode *old_a_prev = a->prev;
	DListNode *old_b_next = b->next;

	a->prev = b;
	a->next = old_b_next;

	b->next = a;
	b->prev = old_a_prev;

	if (old_a_prev != NULL)
		old_a_prev->next = b;

	if (old_b_next != NULL)
		old_b_next->prev = a;
}

static void _DListNode_swap_case2(DListNode *a, DListNode *b)
{
	DListNode *old_a_prev = a->prev;
	DListNode *old_a_next = a->next;

	DListNode *old_b_prev = b->prev;
	DListNode *old_b_next = b->next;

	if (old_a_prev != NULL)
		old_a_prev->next = b;

	if (old_a_next != NULL)
		old_a_next->prev = b;

	if (old_b_prev != NULL)
		old_b_prev->next = a;

	if (old_b_next != NULL)
		old_b_next->prev = a;

	a->prev = old_b_prev;
	a->next = old_b_next;

	b->prev = old_a_prev;
	b->next = old_a_next;
}

static void _DListNode_swap(DListNode *a, DListNode *b)
{
	if (a->next == b)
		_DListNode_swap_case1(a, b);
	else if (a->prev == b)
		_DListNode_swap_case1(b, a);
	else
		_DListNode_swap_case2(a, b);
}

/* }}} Other */

/* Removing {{{ */

static DList* _DList_rf_val(DList *self, const void *target, CmpFunc cmp_func, bool to_all)
{
	DListNode *current = self->start;
	bool was_removed = false;

	while (current != NULL) 
	{
		if (cmp_func(current, target) == 0)
		{
			if (current == self->start)
			{
				self->start = current->next;
				self->ff(current);
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

				self->ff(current);
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
				self->ff(current);
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

				self->ff(current);
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

	size_t size = va_arg(*ap, size_t);
	FreeFunc free_func = va_arg(*ap, FreeFunc);
	CpyFunc cpy_func = va_arg(*ap, CpyFunc);

	if (size < sizeof(DListNode))
	{
		object_delete(_self);
		return_val_if_fail(size >= sizeof(DListNode), NULL);
	}

	if (free_func == NULL)
		self->ff = free;
	else
		self->ff = free_func;

	self->cpf = cpy_func;
	self->start = NULL;
	self->end = NULL;
	self->len = 0;
	self->size = size;

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
			DListNode *next = current->next;
			self->ff(current);
			current = next;
		}
	}

	return (Object*) self;
}

static Object* DList_cpy(const Object *_self, Object *_object, va_list *ap)
{
	const DList *self = DLIST(_self);
	DList *object = DLIST(OBJECT_CLASS(OBJECT_TYPE)->cpy(_self, _object, ap));

	object->cpf = self->cpf;
	object->ff = self->ff;
	object->start = NULL;
	object->end = NULL;
	object->len = self->len;
	object->size = self->size;

	if (self->start == NULL)
		return (Object*) object;

	DListNode *start = _DListNode_new(self->size);

	if (start == NULL)
	{
		object_delete((Object*) object);
		return_val_if_fail(start != NULL, NULL);
	}

	if (object->cpf != NULL)
	{
		va_list ap_copy;
		va_copy(*ap, ap_copy);
		object->cpf(start, self->start);
		va_end(ap_copy);
	}

	object->start = start;

	DListNode *s_current = self->start->next;
	DListNode *o_prev = object->start;

	while (s_current != NULL) 
	{
		DListNode *o_prev_next = _DListNode_new(object->size);

		if (o_prev_next == NULL)
		{
			object_delete((Object*) object);
			return_val_if_fail(o_prev_next != NULL, NULL);
		}

		if (object->cpf != NULL)
		{
			va_list ap_copy;
			va_copy(*ap, ap_copy);
			object->cpf(o_prev_next, s_current);
			va_end(ap_copy);
		}

		o_prev->next = o_prev_next;
		o_prev_next->prev = o_prev;
		o_prev = o_prev_next;
		s_current = s_current->next;
	}

	object->end = o_prev;

	return (Object*) object;
}

/* }}} */

/* Adding values {{{ */

static DListNode* DList_append(DList *self)
{
	if (self->start == NULL && self->end == NULL)
	{
		self->start = _DListNode_new(self->size);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;

		return self->start;
	}

	DListNode *end = _DListNode_new(self->size);
	return_val_if_fail(end, NULL);

	end->prev = self->end;
	self->end->next = end;
	self->end = end;
	self->len++;

	return end;
}

static DListNode* DList_prepend(DList *self)
{
	if (self->start == NULL && self->end == NULL)
	{
		self->start = _DListNode_new(self->size);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;

		return self->start;
	}

	DListNode *start = _DListNode_new(self->size);
	return_val_if_fail(start != NULL, NULL);

	start->next = self->start;
	self->start->prev = start;
	self->start = start;
	self->len++;

	return start;
}

static DListNode* DList_insert_before(DList *self, DListNode *sibling)
{
	DListNode *before = _DListNode_new(self->size);
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

static DListNode* DList_insert(DList *self, size_t index)
{
	if (self->start == NULL && self->end == NULL)
	{
		self->start = _DListNode_new(self->size);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;

		return self->start;
	}

	DListNode *node;
	DListNode *current;
	size_t i;

	node = _DListNode_new(self->size);
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

static DListNode* DList_insert_before_val(DList *self, const void *target, CmpFunc cmp_func)
{
	DListNode *current = self->start;

	while (current != NULL) 
	{
		if (cmp_func(current, target) == 0)
		{
			DListNode *before = _DListNode_new(self->size);
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

static DListNode* DList_pop(DList *self)
{
	if (self->len == 0)
		return NULL;

	DListNode *res;

	if (self->len == 1)
	{
		res = self->start;
		self->start = NULL;
		self->end = NULL;
	}
	else
	{
		res = self->end;
		self->end = self->end->prev;
		self->end->next = NULL;
		res->prev = NULL;
	}

	self->len--;

	return res;
}

/* }}} */

/* Other {{{ */

static DListNode* DList_find(DList *self, const void *target, CmpFunc cmp_func)
{
	DListNode *current = self->start;

	while (current != NULL) 
	{
		if (cmp_func(current, target) == 0)
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

			str_func(current, &ap_copy);
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

			str_func(current, &ap_copy);
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
		func(current, userdata);
		current = current->next;
	}
}

static DList* DListNode_swap(DList *self, DListNode *a, DListNode *b)
{
	_DListNode_swap(a, b);

	if (a == self->start)
		self->start = b;
	else if (b == self->start)
		self->start = a;

	if (a == self->end)
		self->end = b;
	else if (b == self->end)
		self->end = a;

	return self;
}

static size_t DList_count(const DList *self, const void *target, CmpFunc cmp_func)
{
	DListNode *current = self->start;

	size_t count = 0;

	while (current != NULL)
	{
		if (cmp_func(current, target) == 0)
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

DList* dlist_new(size_t size, FreeFunc free_func, CpyFunc cpy_func)
{
	return_val_if_fail(size >= sizeof(DListNode), NULL);
	return (DList*)object_new(DLIST_TYPE, size, free_func, cpy_func);
}

DListNode* dlist_append(DList *self)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return DList_append(self);
}

DListNode* dlist_prepend(DList *self)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return DList_prepend(self);
}

DListNode* dlist_insert(DList *self, size_t index)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return DList_insert(self, index);
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

DListNode* dlist_insert_before(DList *self, DListNode *sibling)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(sibling != NULL, NULL);
	return DList_insert_before(self, sibling);
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

void dlist_delete(DList *self)
{
	return_if_fail(IS_DLIST(self));
	object_delete((Object*) self);
}

DList* dlist_copy(const DList *self)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return (DList*)object_copy((const Object*) self);
}

DList* dlist_swap(DList *self, DListNode *a, DListNode *b)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(a != NULL, NULL);
	return_val_if_fail(b != NULL, NULL);

	return DListNode_swap(self, a, b);
}

DListNode* dlist_insert_before_val(DList *self, const void *target, CmpFunc cmp_func)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(cmp_func != NULL, NULL);
	return DList_insert_before_val(self, target, cmp_func);
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

DListNode* dlist_pop(DList *self)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return DList_pop(self);
}

bool dlist_is_empty(const DList *self)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return (self->len == 0) ? true : false;
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
