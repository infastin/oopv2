#ifndef SLIST_H_G08NRKFN
#define SLIST_H_G08NRKFN

#include <stddef.h>

#include "Object.h"
#include "StringerInterface.h"

#define SLIST_TYPE (slist_get_type())
DECLARE_DERIVABLE_TYPE(SList, slist, SLIST, Object);

typedef struct _SListNode SListNode;

struct _SListClass
{
	ObjectClass parent;

	SListNode* 	(*append)(SList *self, const void *data);
	SListNode* 	(*prepend)(SList *self, const void *data);
	SListNode* 	(*find)(SList *self, const void *target, CmpFunc cmp_func);
	SListNode* 	(*insert)(SList *self, size_t index, const void *data);
	SListNode* 	(*insert_before)(SList *self, SListNode *sibling, const void *data);
	SListNode* 	(*insert_before_val)(SList *self, const void *target, CmpFunc cmp_func, const void *data);
	SList* 		(*reverse)(SList *self);
	SList* 		(*remove_val)(SList *self, const void *target, CmpFunc cmp_func, bool remove_all);
	SList* 		(*free_val)(SList *self, const void *target, CmpFunc cmp_func, bool free_all);
	void 		(*set_free_func)(SList *self, FreeFunc free_func);
	size_t 		(*count)(const SList *self, const void *target, CmpFunc cmp_func);
	void 		(*foreach)(SList *self, JustFunc func, void *userdata);
	SList* 		(*remove_sibling)(SList *self, SListNode *sibling);
	SList* 		(*free_sibling)(SList *self, SListNode *sibling);
};

struct _SListNode
{
	SListNode *next;
	void *data;
};

struct _SList
{
	Object parent;
	void *data[2];
	size_t len;
	SListNode *start;
	SListNode *end;
};

SList* slist_new(size_t elemsize);
void   slist_delete(SList *self, bool free_data);
SList* slist_copy(const SList *self);
SList* slist_set(SList* self, SListNode *sibling, const void *data);
void   slist_get(const SList* self, SListNode *sibling, void *ret);
SListNode* slist_append(SList *self, const void *data); 
SListNode* slist_prepend(SList *self, const void *data); 
SListNode* slist_insert(SList *self, size_t index, const void *data); 
SListNode* slist_insert_before(SList *self, SListNode *sibling, const void *data);
SListNode* slist_find(SList *self, const void *target, CmpFunc cmp_func);
SListNode* slist_insert_before_val(SList *self, const void *target, CmpFunc cmp_func, const void *data);
SList* slist_reverse(SList *self);
SList* slist_remove_val(SList *self, const void *target, CmpFunc cmp_func, bool remove_all);
SList* slist_free_val(SList *self, const void *target, CmpFunc cmp_func, bool free_all);
void slist_set_free_func(SList *self, FreeFunc free_func);
size_t slist_count(const SList *self, const void *target, CmpFunc cmp_func);
void slist_foreach(SList *self, JustFunc func, void *userdata);
SList* slist_remove_sibling(SList *self, SListNode *sibling);
SList* slist_free_sibling(SList *self, SListNode *sibling);

#define slist_output(self, str_func...) (stringer_output((Stringer*) self, str_func))
#define slist_outputln(self, str_func...) (stringer_outputln((Stringer*) self, str_func))

#endif /* end of include guard: SLIST_H_G08NRKFN */
