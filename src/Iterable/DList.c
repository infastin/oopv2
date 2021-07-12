#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Iterable/DList.h"

/* Predefinitions {{{ */

static void stringer_interface_init(StringerInterface *iface);

DEFINE_TYPE_WITH_IFACES(DList, dlist, object, 1, 
		USE_INTERFACE(STRINGER_INTERFACE_TYPE, stringer_interface_init));

typedef struct _DlistData DListData;

struct _DlistData
{
	size_t elemsize;
	FreeFunc ff;
};

#define dl_data(s) ((DListData*) (s)->data)

/* }}} */

/* Private methods {{{ */

static DListNode* DListNode_new(size_t elemsize, const void *data)
{
	DListNode *res = salloc(DListNode, 1);
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
	res->prev = NULL;

	return res;
}

static DList* DList_rf_val(DList *self, const void *target, CmpFunc cmp_func, bool to_all, bool to_free)
{
	DListNode *current = self->start;

	while (current != NULL) 
	{
		if (cmp_func(current->data, target) == 0)
		{
			if (current == self->start)
			{
				self->start = current->next;
				
				if (to_free)
					dl_data(self)->ff(current->data);
				
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

				if (to_free)
					dl_data(self)->ff(current->data);

				free(current);
				current = tmp->next;
			}

			self->len--;

			if (to_all)
				continue;

			break;
		}

		current = current->next;
	}

	return self;
}

static DList* DList_rf_sibling(DList *self, DListNode *sibling, bool to_free)
{
	DListNode *current = self->start;

	while (current != NULL) 
	{
		if (current == sibling)
		{
			if (current == self->start)
			{
				self->start = current->next;
				
				if (to_free)
					dl_data(self)->ff(current->data);

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

				if (to_free)
					dl_data(self)->ff(current->data);

				free(current);
			}

			self->len--;
			break;
		}

		current = current->next;
	}

	return self;
}

/* }}} */

/* Public methods {{{ */

static Object* DList_ctor(Object *_self, va_list *ap)
{
	DList *self = DLIST(object_super_ctor(DLIST_TYPE, _self, ap));

	dl_data(self)->elemsize = va_arg(*ap, size_t);
	dl_data(self)->ff = free;
	
	self->start = NULL;
	self->end = NULL;
	self->len = 0;

	return _self;
}

static Object* DList_dtor(Object *_self, va_list *ap)
{
	DList *self = DLIST(_self);

	bool free_data = (bool) va_arg(*ap, int);

	if (self->start != NULL)
	{
		DListNode *current = self->start;

		while (current != NULL) 
		{
			if (free_data)
				dl_data(self)->ff(current->data);

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
	DList *object = (DList*) object_super_cpy(DLIST_TYPE, _self, _object);

	dl_data(object)->elemsize = dl_data(self)->elemsize;
	dl_data(object)->ff = dl_data(self)->ff;

	object->start = NULL;
	object->end = NULL;
	object->len = self->len;

	if (self->start == NULL)
		return (Object*) object;

	DListNode *start = DListNode_new(dl_data(object)->elemsize, self->start->data);
	
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
		DListNode *o_prev_next = DListNode_new(dl_data(object)->elemsize, s_current->data);

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

static Object* DList_set(Object *_self, va_list *ap)
{
	DList *self = DLIST(_self);

	DListNode *sibling = va_arg(*ap, DListNode*);
	const void *data = va_arg(*ap, const void*);

	if (data != NULL)
		memcpy(sibling->data, data, dl_data(self)->elemsize);
	else
		memset(sibling->data, 0, dl_data(self)->elemsize);

	return _self;
}

static void DList_get(const Object *_self, va_list *ap)
{
	const DList *self = DLIST(_self);

	DListNode *sibling = va_arg(*ap, DListNode*);
	void *ret = va_arg(*ap, void*);

	memcpy(ret, sibling->data, dl_data(self)->elemsize);
}

static DListNode* DList_append(DList *self, const void *data)
{
	if (self->start == NULL && self->end == NULL)
	{
		self->start = DListNode_new(dl_data(self)->elemsize, data);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;
		
		return self->start;
	}

	DListNode *end = DListNode_new(dl_data(self)->elemsize, data);
	return_val_if_fail(end, NULL);

	end->prev = self->end;
	self->end->next = end;
	self->end = end;
	self->len++;

	return end;
}

static DListNode* DList_prepend(DList *self, const void *data)
{
	if (self->start == NULL && self->end == NULL)
	{
		self->start = DListNode_new(dl_data(self)->elemsize, data);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;

		return self->start;
	}

	DListNode *start = DListNode_new(dl_data(self)->elemsize, data);
	return_val_if_fail(start != NULL, NULL);

	start->next = self->start;
	self->start->prev = start;
	self->start = start;
	self->len++;

	return start;
}

static DListNode* DList_insert_before(DList *self, DListNode *sibling, const void *data)
{
	DListNode *before = DListNode_new(dl_data(self)->elemsize, data);
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

static DList* DList_remove_val(DList *self, const void *target, CmpFunc cmp_func, bool remove_all)
{
	return DList_rf_val(self, target, cmp_func, remove_all, false);
}

static DList* DList_remove_sibling(DList *self, DListNode *sibling)
{
	return DList_rf_sibling(self, sibling, false);
}

static DList* DList_free_sibling(DList *self, DListNode *sibling)
{
	return DList_rf_sibling(self, sibling, true);
}

static DList* DList_free_val(DList *self, const void *target, CmpFunc cmp_func, bool free_all)
{
	return DList_rf_val(self, target, cmp_func, free_all, true);
}

static void DList_string(const Stringer *_self, va_list *ap)
{
	const DList *self = DLIST((const Object*) _self);

	StringFunc str_func = va_arg(*ap, StringFunc);
	DListOutputDirection side = va_arg(*ap, DListOutputDirection);

	if (str_func == NULL)
		return;

	printf("[");

	DListNode *current;
	
	if (side == DLIST_OUTPUT_TO_LEFT)
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
	else if (side == DLIST_OUTPUT_TO_RIGHT)
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

static void DList_set_free_func(DList *self, FreeFunc free_func)
{
	dl_data(self)->ff = free_func;
}

static DListNode* DList_insert(DList *self, size_t index, const void *data)
{
	if (self->start == NULL && self->end == NULL)
	{
		self->start = DListNode_new(dl_data(self)->elemsize, data);
		return_val_if_fail(self->start != NULL, NULL);
		self->end = self->start;
		self->len++;

		return self->start;
	}
	
	DListNode *node;
	DListNode *current;
	size_t i;
	
	node = DListNode_new(dl_data(self)->elemsize, data);
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

static void DList_foreach(DList *self, JustFunc func, void *userdata)
{
	DListNode *current = self->start;

	while (current != NULL) 
	{
		func(current->data, userdata);
		current = current->next;
	}
}

static DList* DList_swap(DList *self, DListNode *a_sib, DListNode *b_sib)
{
	void *tmp = a_sib->data;
	
	a_sib->data = b_sib->data;
	b_sib->data = tmp;

	return self;
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

static DList* DList_splice(DList *self, DListNode *l_sib, DListNode *r_sib, DList *other, DListNode *o_sib)
{
	DListNode *l_spliced = l_sib->next;
	DListNode *r_spliced = r_sib->prev;

	l_sib->next = r_sib;
	r_sib->prev = l_sib;

	l_spliced->prev = o_sib;
	r_spliced->next = o_sib->next;
	o_sib->next = l_spliced;

	if (other->end == o_sib)
		other->end = r_spliced;

	size_t elems = 1;

	for(DListNode *current = l_spliced; 
			current != r_spliced && current != NULL; 
			current = current->next, elems++);

	self->len -= elems;
	other->len += elems;

	return other;
}

static DListNode* DList_insert_before_val(DList *self, const void *target, CmpFunc cmp_func, const void *data)
{
	DListNode *current = self->start;

	while (current != NULL) 
	{
		if (cmp_func(current->data, target) == 0)
		{
			DListNode *before = DListNode_new(dl_data(self)->elemsize, data);
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

/* Selectors {{{ */

DList* dlist_new(size_t elemsize)
{
	return (DList*)object_new(DLIST_TYPE, elemsize);
}

DListNode* dlist_append(DList *self, const void *data)
{
	return_val_if_fail(IS_DLIST(self), NULL);

	DListClass *klass = DLIST_GET_CLASS(self);

	return_val_if_fail(klass->append != NULL, NULL);
	return klass->append(self, data);
}

DListNode* dlist_prepend(DList *self, const void *data)
{
	return_val_if_fail(IS_DLIST(self), NULL);

	DListClass *klass = DLIST_GET_CLASS(self);

	return_val_if_fail(klass->prepend != NULL, NULL);
	return klass->prepend(self, data);
}

DListNode* dlist_insert(DList *self, size_t index, const void *data)
{
	return_val_if_fail(IS_DLIST(self), NULL);

	DListClass *klass = DLIST_GET_CLASS(self);

	return_val_if_fail(klass->insert != NULL, NULL);
	return klass->insert(self, index, data);
}

DList* dlist_remove_val(DList *self, const void *target, CmpFunc cmp_func, bool remove_all)
{
	return_val_if_fail(IS_DLIST(self), NULL);

	DListClass *klass = DLIST_GET_CLASS(self);

	return_val_if_fail(klass->remove_val != NULL, NULL);
	return klass->remove_val(self, target, cmp_func, remove_all);
}

DList* dlist_free_val(DList *self, const void *target, CmpFunc cmp_func, bool free_all)
{
	return_val_if_fail(IS_DLIST(self), NULL);

	DListClass *klass = DLIST_GET_CLASS(self);

	return_val_if_fail(klass->free_val != NULL, NULL);
	return klass->free_val(self, target, cmp_func, free_all);
}

void dlist_foreach(DList *self, JustFunc func, void *userdata)
{
	return_if_fail(IS_DLIST(self));

	DListClass *klass = DLIST_GET_CLASS(self);

	return_if_fail(klass->foreach != NULL);
	klass->foreach(self, func, userdata);
}

void dlist_set_free_func(DList *self, FreeFunc free_func)
{
	return_if_fail(IS_DLIST(self));

	DListClass *klass = DLIST_GET_CLASS(self);

	return_if_fail(klass->set_free_func != NULL);
	klass->set_free_func(self, free_func);
}

size_t dlist_count(const DList *self, const void *target, CmpFunc cmp_func)
{
	return_val_if_fail(IS_DLIST(self), 0);

	DListClass *klass = DLIST_GET_CLASS(self);

	return_val_if_fail(klass->count != NULL, 0);
	return klass->count(self, target, cmp_func);
}

DListNode* dlist_insert_before(DList *self, DListNode *sibling, const void *data)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(sibling != NULL, NULL);

	DListClass *klass = DLIST_GET_CLASS(self);

	return_val_if_fail(klass->insert_before != NULL, NULL);
	return klass->insert_before(self, sibling, data);
}

DListNode* dlist_find(DList *self, const void *target, CmpFunc cmp_func)
{
	return_val_if_fail(IS_DLIST(self), NULL);

	DListClass *klass = DLIST_GET_CLASS(self);

	return_val_if_fail(klass->find != NULL, NULL);
	return klass->find(self, target, cmp_func);
}

DList* dlist_remove_sibling(DList *self, DListNode *sibling)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(sibling != NULL, NULL);

	DListClass *klass = DLIST_GET_CLASS(self);

	return_val_if_fail(klass->remove_sibling != NULL, NULL);
	return klass->remove_sibling(self, sibling);
}

DList* dlist_free_sibling(DList *self, DListNode *sibling)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(sibling != NULL, NULL);

	DListClass *klass = DLIST_GET_CLASS(self);

	return_val_if_fail(klass->remove_sibling != NULL, NULL);
	return klass->remove_sibling(self, sibling);
}

DList* dlist_set(DList *self, DListNode *sibling, const void *data)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(sibling != NULL, NULL);

	return (DList*)object_set((Object*) self, sibling, data);
}

void dlist_get(const DList *self, DListNode *sibling, void *ret)
{
	return_if_fail(IS_DLIST(self));
	return_if_fail(sibling != NULL);

	object_get((const Object*) self, sibling, ret);
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

DList* dlist_swap(DList *self, DListNode *a_sib, DListNode *b_sib)
{
	return_val_if_fail(IS_DLIST(self), NULL);

	DListClass *klass = DLIST_GET_CLASS(self);

	return_val_if_fail(klass->swap != NULL, NULL);
	return klass->swap(self, a_sib, b_sib);
}

DList* dlist_splice(DList *self, DListNode *l_sib, DListNode *r_sib, DList *other, DListNode *o_sib)
{
	return_val_if_fail(IS_DLIST(self), NULL);
	return_val_if_fail(IS_DLIST(other), NULL);

	return_val_if_fail(l_sib != NULL, NULL);
	return_val_if_fail(r_sib != NULL, NULL);
	return_val_if_fail(o_sib != NULL, NULL);

	DListClass *klass = DLIST_GET_CLASS(self);

	return_val_if_fail(klass->splice != NULL, NULL);
	return klass->splice(self, l_sib, r_sib, other, o_sib);
}

DListNode* dlist_insert_before_val(DList *self, const void *target, CmpFunc cmp_func, const void *data)
{
	return_val_if_fail(IS_DLIST(self), NULL);

	DListClass *klass = DLIST_GET_CLASS(self);

	return_val_if_fail(klass->insert_before_val != NULL, NULL);
	return klass->insert_before_val(self, target, cmp_func, data);
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
	OBJECT_CLASS(klass)->set = DList_set;
	OBJECT_CLASS(klass)->get = DList_get;

	klass->append = DList_append;
	klass->prepend = DList_prepend;
	klass->insert_before = DList_insert_before;
	klass->find = DList_find;
	klass->insert = DList_insert;
	klass->set_free_func = DList_set_free_func;
	klass->remove_val = DList_remove_val;
	klass->free_val = DList_free_val;
	klass->foreach = DList_foreach;
	klass->swap = DList_swap;
	klass->count = DList_count;
	klass->remove_sibling = DList_remove_sibling;
	klass->free_sibling = DList_free_sibling;
	klass->splice = DList_splice;
	klass->insert_before_val = DList_insert_before_val;
}

/* }}} */
