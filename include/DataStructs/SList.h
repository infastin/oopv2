#ifndef SLIST_H_G08NRKFN
#define SLIST_H_G08NRKFN

#include <stddef.h>

#include "Base.h"
#include "Interfaces/StringerInterface.h"

#define SLIST_TYPE (slist_get_type())
DECLARE_TYPE(SList, slist, SLIST, Object);

typedef struct _SListNode SListNode;

struct _SListNode
{
	SListNode *next;
};

SList* slist_new(size_t size, FreeFunc free_func, CpyFunc cpy_func);
void   slist_delete(SList *self);
SList* slist_copy(const SList *self);
SListNode* slist_append(SList *self); 
SListNode* slist_prepend(SList *self); 
SListNode* slist_insert(SList *self, size_t index); 
SListNode* slist_insert_before(SList *self, SListNode *sibling);
SListNode* slist_find(SList *self, const void *target, CmpFunc cmp_func);
SListNode* slist_insert_before_val(SList *self, const void *target, CmpFunc cmp_func);
SList* slist_reverse(SList *self);
SList* slist_remove_val(SList *self, const void *target, CmpFunc cmp_func, bool remove_all);
ssize_t slist_count(const SList *self, const void *target, CmpFunc cmp_func);
void slist_foreach(SList *self, JustFunc func, void *userdata);
SList* slist_remove_sibling(SList *self, SListNode *sibling);
ssize_t slist_get_length(const SList *self);
void slist_sort(SList *self, CmpFunc cmp_func);
void slist_node_swap(SListNode *a, SListNode *b);

#define slist_output(self, str_func...)                        \
	(                                                          \
		(IS_SLIST(self)) ?                                     \
		(stringer_output((const Stringer*) self, str_func)) :  \
		(return_if_fail_warning(STRFUNC, "IS_SLIST("#self")")) \
	)

#define slist_outputln(self, str_func...)                       \
	(                                                           \
		(IS_SLIST(self)) ?                                      \
		(stringer_outputln((const Stringer*) self, str_func)) : \
		(return_if_fail_warning(STRFUNC, "IS_SLIST("#self")"))  \
	)

#endif /* end of include guard: SLIST_H_G08NRKFN */
