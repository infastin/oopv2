#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Interface.h"
#include "Messages.h"
#include "SList.h"
#include "StringerInterface.h"
#include "Object.h"
#include "Utils.h"

/* Predefinitions {{{ */

static void stringer_interface_init(StringerInterface *iface);

DEFINE_TYPE_WITH_IFACES(SList, slist, object, 1,
		USE_INTERFACE(STRINGER_INTERFACE_TYPE, stringer_interface_init));

typedef struct _SListData SListData;

struct _SListData
{
	size_t elemsize;
	FreeFunc ff;
};

#define sl_data(s) ((SListData*) (s)->data)

/* }}} */

/* Private methods {{{ */

static SListNode* SListNode_new(size_t elemsize, const void *data)
{
	SListNode *res = salloc(SListNode, 1);
	return_val_if_fail(res != NULL, NULL);

	void *dt = calloc(1, elemsize);
	
	if (dt == NULL)
	{
		free(res);
		return_val_if_fail(dt != NULL, NULL);
	}

	if (data != NULL)
		memcpy(dt, data, elemsize);
	else
		memset(dt, 0, elemsize);

	res->data = dt;
	res->next = NULL;

	return res;
}

static SList* SList_rf_val(SList *self, const void *target, CmpFunc cmp_func, bool to_all, bool to_free)
{
	SListNode *current = self->start;
	SListNode *prev = NULL;

	while (current != NULL) 
	{
		if (cmp_func(current->data, target) == 0)
		{
			if (prev == NULL)
			{
				self->start = current->next;
				
				if (to_free)
					sl_data(self)->ff(current->data);
				
				free(current);
				current = self->start;

				if (current == NULL)
					self->end = NULL;
			}
			else 
			{
				prev->next = current->next;
				
				if (to_free)
					sl_data(self)->ff(current->data);
				
				free(current);
				current = prev->next;

				if (current == NULL)
					self->end = prev;
			}

			self->len--;

			if (to_all)
				continue;

			break;
		}

		prev = current;
		current = current->next;
	}

	return self;
}

static SList* SList_rf_sibling(SList *self, SListNode *sibling, bool to_free)
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

				if (to_free)
					sl_data(self)->ff(current->data);

				free(current);
				current = self->start;

				if (current == NULL)
					self->end = NULL;
			}
			else
			{
				prev->next = current->next;

				if (to_free)
					sl_data(self)->ff(current->data);

				free(current);
				current = prev->next;

				if (current == NULL)
					self->end = prev;
			}

			self->len--;
			break;
		}

		prev = current;
		current = current->next;
	}

	return self;
}

/* }}} */

/* Public methods {{{ */

static Object* SList_ctor(Object *_self, va_list *ap)
{
	SList *self = SLIST(object_super_ctor(SLIST_TYPE, _self, ap));

	sl_data(self)->elemsize = va_arg(*ap, size_t);
	sl_data(self)->ff = free;

	self->start = NULL;
	self->end = NULL;
	self->len = 0;

	return _self;
}

static Object* SList_dtor(Object *_self, va_list *ap)
{
	SList *self = SLIST(_self);

	bool free_data = (bool) va_arg(*ap, int);

	if (self->start != NULL)
	{
		SListNode *current = self->start;

		while (current != NULL) 
		{
			if (free_data)
				sl_data(self)->ff(current->data);

			SListNode *next = current->next;
			free(current);
			current = next;
		}
	}

	return (Object*) self;
}

static Object* SList_cpy(const Object *_self, Object *_object)
{
	const SList *self = SLIST(_self);
	SList *object = (SList*) object_super_cpy(SLIST_TYPE, _self, _object);

	sl_data(object)->elemsize = sl_data(self)->elemsize;
	sl_data(object)->ff = sl_data(self)->ff;

	object->start = NULL;
	object->end = NULL;
	object->len = self->len;

	if (self->start == NULL)
		return (Object*) object;

	SListNode *start = SListNode_new(sl_data(object)->elemsize, self->start->data);
	
	if (start == NULL)
	{
		object_delete((Object*) object, 0);
		return_val_if_fail(start != NULL, NULL);
	}

	SListNode *s_current = self->start->next;
	SListNode *o_prev = object->start;

	while (s_current != NULL) 
	{
		SListNode *o_prev_next = SListNode_new(sl_data(object)->elemsize, s_current->data);

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

static Object* SList_set(Object *_self, va_list *ap)
{
	SList *self = SLIST(_self);

	SListNode *sibling = va_arg(*ap, SListNode*);
	const void *data = va_arg(*ap, const void*);

	if (data != NULL)
		memcpy(sibling->data, data, sl_data(self)->elemsize);
	else
		memset(sibling->data, 0, sl_data(self)->elemsize);

	return _self;
}

static void SList_get(const Object *_self, va_list *ap)
{
	const SList *self = SLIST(_self);

	SListNode *sibling = va_arg(*ap, SListNode*);
	void *ret = va_arg(*ap, void*);

	memcpy(ret, sibling->data, sl_data(self)->elemsize);
}

static SListNode* SList_append(SList *self, const void *data)
{
	if (self->start == NULL)
	{
		self->start = SListNode_new(sl_data(self)->elemsize, data);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;

		return self->start;
	}

	SListNode *end = SListNode_new(sl_data(self)->elemsize, data);
	return_val_if_fail(end, NULL);

	self->end->next = end;
	self->end = end;
	self->len++;
		
	return end;
}

static SListNode* SList_prepend(SList *self, const void *data)
{
	if (self->start == NULL)
	{
		self->start = SListNode_new(sl_data(self)->elemsize, data);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;

		return self->start;
	}

	SListNode *start = SListNode_new(sl_data(self)->elemsize, data);
	return_val_if_fail(start != NULL, NULL);
	
	start->next = self->start;
	self->start = start;
	self->len++;

	return start;
}

static SListNode* SList_insert_before(SList *self, SListNode *sibling, const void *data)
{
	SListNode *current = self->start;
	SListNode *prev = NULL;

	while (current != NULL) 
	{
		if (current == sibling)
		{
			SListNode *before = SListNode_new(sl_data(self)->elemsize, data);
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

static SListNode* SList_insert_before_val(SList *self, const void *target, CmpFunc cmp_func, const void *data)
{
	SListNode *current = self->start;
	SListNode *prev = NULL;

	while (current != NULL) 
	{
		if (cmp_func(current->data, target) == 0)
		{
			SListNode *before = SListNode_new(sl_data(self)->elemsize, data);
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

static SList* SList_remove_val(SList *self, const void *target, CmpFunc cmp_func, bool remove_all)
{
	return SList_rf_val(self, target, cmp_func, remove_all, false);
}

static SList* SList_remove_sibling(SList *self, SListNode *sibling)
{
	return SList_rf_sibling(self, sibling, false);
}

static SList* SList_free_sibling(SList *self, SListNode *sibling)
{
	return SList_rf_sibling(self, sibling, true);
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

static SList* SList_free_val(SList *self, const void *target, CmpFunc cmp_func, bool free_all)
{
	return SList_rf_val(self, target, cmp_func, free_all, true);
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

	if (str_func == NULL)
		return;

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

static void SList_set_free_func(SList *self, FreeFunc free_func)
{
	sl_data(self)->ff = free_func;
}

static SListNode* SList_insert(SList *self, size_t index, const void *data)
{
	if (self->start == NULL)
	{
		self->start = SListNode_new(sl_data(self)->elemsize, data);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;

		return self->start;
	}

	SListNode *node = SListNode_new(sl_data(self)->elemsize, data);
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

/* Selectors {{{ */

SList* slist_new(size_t elemsize)
{
	return (SList*) object_new(SLIST_TYPE, elemsize);
}

void slist_delete(SList *self, bool free_data)
{
	return_if_fail(IS_SLIST(self));
	object_delete((Object*) self, free_data);
}

SList* slist_copy(const SList *self)
{
	return_val_if_fail(IS_SLIST(self), NULL);
	return (SList*) object_copy((Object*) self);
}

SList* slist_set(SList* self, SListNode *sibling, const void *data)
{
	return_val_if_fail(IS_SLIST(self), NULL);
	return (SList*) object_set((Object*) self, sibling, data);
}

void slist_get(const SList* self, SListNode *sibling, void *ret)
{
	return_if_fail(IS_SLIST(self));
	object_get((Object*) self, sibling, ret);
}

SListNode* slist_append(SList *self, const void *data)
{
	return_val_if_fail(IS_SLIST(self), NULL);

	SListClass *klass = SLIST_GET_CLASS(self);

	return_val_if_fail(klass->append != NULL, NULL);
	return klass->append(self, data);
}

SListNode* slist_insert(SList *self, size_t index, const void *data)
{
	return_val_if_fail(IS_SLIST(self), NULL);

	SListClass *klass = SLIST_GET_CLASS(self);

	return_val_if_fail(klass->insert != NULL, NULL);
	return klass->insert(self, index, data);
}

SListNode* slist_prepend(SList *self, const void *data)
{
	return_val_if_fail(IS_SLIST(self), NULL);

	SListClass *klass = SLIST_GET_CLASS(self);

	return_val_if_fail(klass->prepend != NULL, NULL);
	return klass->prepend(self, data);
}

SListNode* slist_insert_before(SList *self, SListNode *sibling, const void *data)
{
	return_val_if_fail(IS_SLIST(self), NULL);

	SListClass *klass = SLIST_GET_CLASS(self);

	return_val_if_fail(klass->insert_before != NULL, NULL);
	return klass->insert_before(self, sibling, data);
}

SListNode* slist_find(SList* self, const void *target, CmpFunc cmp_func)
{
	return_val_if_fail(IS_SLIST(self), NULL);

	SListClass *klass = SLIST_GET_CLASS(self);

	return_val_if_fail(klass->find != NULL, NULL);
	return klass->find(self, target, cmp_func);
}

SList* slist_reverse(SList* self)
{
	return_val_if_fail(IS_SLIST(self), NULL);

	SListClass *klass = SLIST_GET_CLASS(self);

	return_val_if_fail(klass->reverse != NULL, NULL);
	return klass->reverse(self);
}

SList* slist_remove_val(SList *self, const void *target, CmpFunc cmp_func, bool remove_all)
{
	return_val_if_fail(IS_SLIST(self), NULL);

	SListClass *klass = SLIST_GET_CLASS(self);

	return_val_if_fail(klass->remove_val != NULL, NULL);
	return klass->remove_val(self, target, cmp_func, remove_all);
}

SList* slist_free_val(SList *self, const void *target, CmpFunc cmp_func, bool free_all)
{
	return_val_if_fail(IS_SLIST(self), NULL);

	SListClass *klass = SLIST_GET_CLASS(self);

	return_val_if_fail(klass->free_val != NULL, NULL);
	return klass->free_val(self, target, cmp_func, free_all);
}

void slist_set_free_func(SList *self, FreeFunc free_func)
{
	return_if_fail(IS_SLIST(self));

	SListClass *klass = SLIST_GET_CLASS(self);

	return_if_fail(klass->set_free_func != NULL);
	return klass->set_free_func(self, free_func);
}

size_t slist_count(const SList *self, const void *target, CmpFunc cmp_func)
{
	return_val_if_fail(IS_SLIST(self), 0);

	SListClass *klass = SLIST_GET_CLASS(self);

	return_val_if_fail(klass->count != NULL, 0);
	return klass->count(self, target, cmp_func);
}

void slist_foreach(SList *self, JustFunc func, void *userdata)
{
	return_if_fail(IS_SLIST(self));

	SListClass *klass = SLIST_GET_CLASS(self);

	return_if_fail(klass->foreach != NULL);
	return klass->foreach(self, func, userdata);
}

SList* slist_remove_sibling(SList *self, SListNode *sibling)
{
	return_val_if_fail(IS_SLIST(self), NULL);

	SListClass *klass = SLIST_GET_CLASS(self);

	return_val_if_fail(klass->remove_sibling != NULL, NULL);
	return klass->remove_sibling(self, sibling);
}

SList* slist_free_sibling(SList *self, SListNode *sibling)
{
	return_val_if_fail(IS_SLIST(self), NULL);

	SListClass *klass = SLIST_GET_CLASS(self);

	return_val_if_fail(klass->free_sibling != NULL, NULL);
	return klass->free_sibling(self, sibling);
}

SListNode* slist_insert_before_val(SList *self, const void *target, CmpFunc cmp_func, const void *data)
{
	return_val_if_fail(IS_SLIST(self), NULL);

	SListClass *klass = SLIST_GET_CLASS(self);

	return_val_if_fail(klass->insert_before_val != NULL, NULL);
	return klass->insert_before_val(self, target, cmp_func, data);
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
	OBJECT_CLASS(klass)->set = SList_set;
	OBJECT_CLASS(klass)->get = SList_get;

	klass->append = SList_append;
	klass->prepend = SList_prepend;
	klass->insert = SList_insert;
	klass->insert_before = SList_insert_before;
	klass->reverse = SList_reverse;
	klass->find = SList_find;
	klass->remove_val = SList_remove_val;
	klass->free_val = SList_free_val;
	klass->set_free_func = SList_set_free_func;
	klass->count = SList_count;
	klass->foreach = SList_foreach;
	klass->remove_sibling = SList_remove_sibling;
	klass->free_sibling = SList_free_sibling;
	klass->insert_before_val = SList_insert_before_val;
}

/* }}} */
