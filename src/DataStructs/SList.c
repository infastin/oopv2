#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Base.h"
#include "DataStructs/SList.h"
#include "Interfaces/StringerInterface.h"

/* Predefinitions {{{ */

struct _SListNode
{
	SListNode *next;
	void *data;
	size_t size;
};

struct _SList
{
	Object parent;
	SListNode *start;
	SListNode *end;
	FreeFunc ff;
	size_t len;
};

static void stringer_interface_init(StringerInterface *iface);

DEFINE_TYPE_WITH_IFACES(SList, slist, object, 1,
		USE_INTERFACE(STRINGER_INTERFACE_TYPE, stringer_interface_init));

/* }}} */

/* Private methods {{{ */

/* Sorting {{{ */

SListNode* _SList_merge(SListNode *first, SListNode *second, SListNode **end, CmpFunc cmp_func)
{
	SListNode *result = NULL;
	SListNode **linkp = &result;

	SListNode *f = first;
	SListNode *s = second;

	while (1) 
	{
		if (f == NULL && s == NULL)
			break;

		if (f == NULL)
		{
			*linkp = s;

			if (end != NULL)
				*end = s;

			break;
		}

		if (s == NULL)
		{
			*linkp = f;

			if (end != NULL)
				*end = f;

			break;
		}

		if (cmp_func(f->data, s->data) <= 0)
		{
			*linkp = f;
			linkp = &f->next;
			f = f->next;
		}
		else
		{
			*linkp = s;
			linkp = &s->next;
			s = s->next;
		}
	}

	return result;
}

SListNode* _SList_merge_sort(SListNode *start, SListNode **end, size_t len, CmpFunc cmp_func)
{
	if (start == NULL)
		return NULL;

	int arr_len = ULONG_BIT - __builtin_clzl(len);
	SListNode **arr = (SListNode**)calloc(arr_len, sizeof(SListNode*));
	SListNode *result;

	result = start;

	while (result != NULL) 
	{
		SListNode *next;
		int i;

		next = result->next;
		result->next = NULL;

		for (i = 0; (i < arr_len) && (arr[i] != NULL); i++)
		{
			result = _SList_merge(arr[i], result, NULL, cmp_func);
			arr[i] = NULL;
		}

		if (i == arr_len)
			i--;

		arr[i] = result;
		result = next;
	}

	for (size_t i = 0; i < arr_len; ++i)
		result = _SList_merge(arr[i], result, end, cmp_func);

	free(arr);

	return result;
}

/* }}} Sorting */

/* Other {{{ */

static SListNode* _SListNode_new(size_t size, const void *data)
{
	SListNode *res = salloc(SListNode, 1);
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

	return res;
}

/* }}} */

/* Removing {{{ */

static SList* _SList_rf_val(SList *self, const void *target, CmpFunc cmp_func, bool to_all)
{
	SListNode *current = self->start;
	SListNode *prev = NULL;
	bool was_removed = false;

	while (current != NULL) 
	{
		if (cmp_func(current->data, target) == 0)
		{
			if (prev == NULL)
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
				prev->next = current->next;

				if (self->ff != NULL)
					self->ff(*(void**) current->data);

				free(current->data);
				free(current);
				current = prev->next;

				if (current == NULL)
					self->end = prev;
			}

			self->len--;
			was_removed = true;

			if (to_all)
				continue;

			break;
		}

		prev = current;
		current = current->next;
	}

	return (was_removed) ? self : NULL;
}

static SList* _SList_rf_sibling(SList *self, SListNode *sibling)
{
	SListNode *current = self->start;
	SListNode *prev = NULL;

	while (current != NULL) 
	{
		if (current == sibling)
		{
			if (prev == NULL)
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
				prev->next = current->next;

				if (self->ff != NULL)
					self->ff(*(void**) current->data);

				free(current->data);
				free(current);
				current = prev->next;

				if (current == NULL)
					self->end = prev;
			}

			self->len--;
			return self;
		}

		prev = current;
		current = current->next;
	}

	return NULL;
}

/* }}} */

/* }}} */

/* Public methods {{{ */

/* Base {{{ */

static Object* SList_ctor(Object *_self, va_list *ap)
{
	SList *self = SLIST(OBJECT_CLASS(OBJECT_TYPE)->ctor(_self, ap));

	self->ff = NULL;
	self->start = NULL;
	self->end = NULL;
	self->len = 0;

	return _self;
}

static Object* SList_dtor(Object *_self, va_list *ap)
{
	SList *self = SLIST(_self);

	if (self->start != NULL)
	{
		SListNode *current = self->start;

		while (current != NULL) 
		{
			if (self->ff != NULL)
				self->ff(*(void**) current->data);

			free(current->data);

			SListNode *next = current->next;
			free(current);
			current = next;
		}
	}

	return (Object*) self;
}

static Object* SList_cpy(const Object *_self, Object *_object, va_list *ap)
{
	const SList *self = SLIST(_self);
	SList *object = SLIST(OBJECT_CLASS(OBJECT_TYPE)->cpy(_self, _object, ap));

	object->ff = self->ff;

	object->start = NULL;
	object->end = NULL;
	object->len = self->len;

	if (self->start == NULL)
		return (Object*) object;

	SListNode *start = _SListNode_new(self->start->size, self->start->data);

	if (start == NULL)
	{
		object_delete((Object*) object, 0);
		return_val_if_fail(start != NULL, NULL);
	}

	object->start = start;

	SListNode *s_current = self->start->next;
	SListNode *o_prev = object->start;

	while (s_current != NULL) 
	{
		SListNode *o_prev_next = _SListNode_new(s_current->size, s_current->data);

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

static SListNode* SList_append(SList *self, const void *data, size_t size)
{
	if (self->start == NULL)
	{
		self->start = _SListNode_new(size, data);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;

		return self->start;
	}

	SListNode *end = _SListNode_new(size, data);
	return_val_if_fail(end, NULL);

	self->end->next = end;
	self->end = end;
	self->len++;

	return end;
}

static SListNode* SList_prepend(SList *self, const void *data, size_t size)
{
	if (self->start == NULL)
	{
		self->start = _SListNode_new(size, data);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;

		return self->start;
	}

	SListNode *start = _SListNode_new(size, data);
	return_val_if_fail(start != NULL, NULL);

	start->next = self->start;
	self->start = start;
	self->len++;

	return start;
}

static SListNode* SList_insert_before(SList *self, SListNode *sibling, const void *data, size_t size)
{
	SListNode *current = self->start;
	SListNode *prev = NULL;

	while (current != NULL) 
	{
		if (current == sibling)
		{
			SListNode *before = _SListNode_new(size, data);
			return_val_if_fail(before != NULL, NULL);

			if (prev == NULL)
			{
				before->next = current;
				self->start = before;
			}
			else
			{
				before->next = current;
				prev->next = before;
			}

			self->len++;
			return before;
		}

		prev = current;
		current = current->next;
	}

	return NULL;
}

static SListNode* SList_insert_before_val(SList *self, const void *target, CmpFunc cmp_func, const void *data, size_t size)
{
	SListNode *current = self->start;
	SListNode *prev = NULL;

	while (current != NULL) 
	{
		if (cmp_func(current->data, target) == 0)
		{
			SListNode *before = _SListNode_new(size, data);
			return_val_if_fail(before != NULL, NULL);

			if (prev == NULL)
			{
				before->next = current;
				self->start = before;
			}
			else
			{
				before->next = current;
				prev->next = before;
			}

			self->len++;
			return before;
		}

		prev = current;
		current = current->next;
	}

	return NULL;
}

static SListNode* SList_insert(SList *self, size_t index, const void *data, size_t size)
{
	if (self->start == NULL)
	{
		self->start = _SListNode_new(size, data);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;

		return self->start;
	}

	SListNode *node = _SListNode_new(size, data);
	return_val_if_fail(node != NULL, NULL);

	if (index < self->len)
	{
		SListNode *current = self->start;
		SListNode *prev = NULL;
		size_t i = 0;

		while (current != NULL) 
		{
			if (i == index)
			{
				if (prev == NULL)
				{
					node->next = current;
					self->start = node;
				}
				else
				{
					prev->next = node;
					node->next = current;
				}

				self->len++;
				break;
			}

			prev = current;
			current = current->next;
			i++;
		}
	}
	else
	{
		self->end->next = node;
		self->end = node;

		self->len++;
	}

	return node;
}

/* }}} */

/* Removing values {{{ */

static SList* SList_remove_val(SList *self, const void *target, CmpFunc cmp_func, bool remove_all)
{
	return _SList_rf_val(self, target, cmp_func, remove_all);
}

static SList* SList_remove_sibling(SList *self, SListNode *sibling)
{
	return _SList_rf_sibling(self, sibling);
}

/* }}} */

/* Other {{{ */

static SListNode* SList_find(SList *self, const void *target, CmpFunc cmp_func)
{
	SListNode *current = self->start;

	while (current != NULL) 
	{
		if (cmp_func(current->data, target) == 0)
			return current;

		current = current->next;
	}

	return NULL;
}

static void SListNode_swap(SListNode *a, SListNode *b)
{
	void *tmp_data = a->data;
	size_t tmp_size = a->size;

	a->data = b->data;
	a->size = b->size;

	b->data = tmp_data;
	b->size = tmp_size;
}

static size_t SList_count(const SList *self, const void *target, CmpFunc cmp_func)
{
	SListNode *current = self->start;

	size_t count = 0;

	while (current != NULL) 
	{
		if (cmp_func(current->data, target) == 0)
			count++;

		current = current->next;
	}

	return count;
}

static SList* SList_reverse(SList *self)
{
	SListNode *current = self->start;
	SListNode *next = current->next;

	while (next != NULL) 
	{
		SListNode *tmp = next->next;
		next->next = current;
		current = next;
		next = tmp;
	}

	SListNode *oldstart = self->start;

	oldstart->next = NULL;
	self->start = self->end;
	self->end = oldstart;

	return self;
}

static void SList_string(const Stringer *_self, va_list *ap)
{
	const SList *self = SLIST((Object*) _self);

	StringFunc str_func = va_arg(*ap, StringFunc);
	return_if_fail(str_func != NULL);

	printf("[");

	SListNode *current = self->start;

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

	printf("]");
}

static void SList_foreach(SList *self, JustFunc func, void *userdata)
{
	SListNode *current = self->start;

	while (current != NULL) 
	{
		func(current->data, userdata);
		current = current->next;
	}
}

/* }}} */

/* }}} */

/* Selectors {{{ */

SList* slist_new(void)
{
	return (SList*)object_new(SLIST_TYPE);
}

void slist_delete(SList *self)
{
	return_if_fail(IS_SLIST(self));
	object_delete((Object*) self);
}

SList* slist_copy(const SList *self)
{
	return_val_if_fail(IS_SLIST(self), NULL);
	return (SList*)object_copy((const Object*) self);
}

SListNode* slist_append(SList *self, const void *data, size_t size)
{
	return_val_if_fail(IS_SLIST(self), NULL);
	return_val_if_fail(size != 0, NULL);
	return SList_append(self, data, size);
}

SListNode* slist_insert(SList *self, size_t index, const void *data, size_t size)
{
	return_val_if_fail(IS_SLIST(self), NULL);
	return_val_if_fail(size != 0, NULL);
	return SList_insert(self, index, data, size);
}

SListNode* slist_prepend(SList *self, const void *data, size_t size)
{
	return_val_if_fail(IS_SLIST(self), NULL);
	return_val_if_fail(size != 0, NULL);
	return SList_prepend(self, data, size);
}

SListNode* slist_insert_before(SList *self, SListNode *sibling, const void *data, size_t size)
{
	return_val_if_fail(IS_SLIST(self), NULL);
	return_val_if_fail(size != 0, NULL);
	return SList_insert_before(self, sibling, data, size);
}

SListNode* slist_find(SList* self, const void *target, CmpFunc cmp_func)
{
	return_val_if_fail(IS_SLIST(self), NULL);
	return_val_if_fail(cmp_func != NULL, NULL);
	return SList_find(self, target, cmp_func);
}

SList* slist_reverse(SList* self)
{
	return_val_if_fail(IS_SLIST(self), NULL);
	return SList_reverse(self);
}

SList* slist_remove_val(SList *self, const void *target, CmpFunc cmp_func, bool remove_all)
{
	return_val_if_fail(IS_SLIST(self), NULL);
	return_val_if_fail(cmp_func != NULL, NULL);
	return SList_remove_val(self, target, cmp_func, remove_all);
}

void slist_set_free_func(SList *self, FreeFunc free_func)
{
	return_if_fail(IS_SLIST(self));
	return_if_fail(free_func != NULL);
	self->ff = free_func;
}

ssize_t slist_get_length(const SList *self)
{
	return_val_if_fail(IS_SLIST(self), -1);
	return self->len;
}

ssize_t slist_count(const SList *self, const void *target, CmpFunc cmp_func)
{
	return_val_if_fail(IS_SLIST(self), -1);
	return_val_if_fail(cmp_func != NULL, -1);
	return SList_count(self, target, cmp_func);
}

void slist_foreach(SList *self, JustFunc func, void *userdata)
{
	return_if_fail(IS_SLIST(self));
	return_if_fail(func != NULL);
	return SList_foreach(self, func, userdata);
}

SList* slist_remove_sibling(SList *self, SListNode *sibling)
{
	return_val_if_fail(IS_SLIST(self), NULL);
	return SList_remove_sibling(self, sibling);
}

SListNode* slist_insert_before_val(SList *self, const void *target, CmpFunc cmp_func, const void *data, size_t size)
{
	return_val_if_fail(IS_SLIST(self), NULL);
	return_val_if_fail(size != 0, NULL);
	return_val_if_fail(cmp_func != NULL, NULL);
	return SList_insert_before_val(self, target, cmp_func, data, size);
}

SListNode* slist_node_set(SListNode *self, const void *data, size_t size)
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

void slist_sort(SList *self, CmpFunc cmp_func)
{
	return_if_fail(IS_SLIST(self));
	return_if_fail(cmp_func != NULL);

	self->start = _SList_merge_sort(self->start, &self->end, self->len, cmp_func);

	if (self->end->next != NULL)
	{
		SListNode *new_end = self->end;
		for (; new_end->next != NULL; new_end = new_end->next);
		self->end = new_end;
	}
}

void slist_node_data(const SListNode *self, void *ret)
{
	return_if_fail(self != NULL);
	return_if_fail(ret != NULL);
	memcpy(ret, self->data, self->size);
}

SListNode* slist_node_next(const SListNode *self)
{
	return_val_if_fail(self != NULL, NULL);
	return self->next;
}

void slist_node_swap(SListNode *a, SListNode *b)
{
	return_if_fail(a != NULL);
	return_if_fail(b != NULL);

	SListNode_swap(a, b);
}

/* }}} */

/* Init {{{ */

static void stringer_interface_init(StringerInterface *iface)
{
	iface->string = SList_string;
}

static void slist_class_init(SListClass *klass)
{
	OBJECT_CLASS(klass)->ctor = SList_ctor;
	OBJECT_CLASS(klass)->dtor = SList_dtor;
	OBJECT_CLASS(klass)->cpy = SList_cpy;
}

/* }}} */
