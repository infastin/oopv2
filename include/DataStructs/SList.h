#ifndef SLIST_H_G08NRKFN
#define SLIST_H_G08NRKFN

#include <stddef.h>

#include "Base.h"
#include "Interfaces/StringerInterface.h"

#define SLIST_TYPE (slist_get_type())
DECLARE_TYPE(SList, slist, SLIST, Object);

typedef struct _SListNode SListNode;

SList* slist_new(void);
void   slist_delete(SList *self);
SList* slist_copy(const SList *self);
SListNode* slist_node_set(SListNode *self, const void *data, size_t size);
SListNode* slist_append(SList *self, const void *data, size_t size); 
SListNode* slist_prepend(SList *self, const void *data, size_t size); 
SListNode* slist_insert(SList *self, size_t index, const void *data, size_t size); 
SListNode* slist_insert_before(SList *self, SListNode *sibling, const void *data, size_t size);
SListNode* slist_find(SList *self, const void *target, CmpFunc cmp_func);
SListNode* slist_insert_before_val(SList *self, const void *target, CmpFunc cmp_func, const void *data, size_t size);
SList* slist_reverse(SList *self);
SList* slist_remove_val(SList *self, const void *target, CmpFunc cmp_func, bool remove_all);
void slist_set_free_func(SList *self, FreeFunc free_func);
ssize_t slist_count(const SList *self, const void *target, CmpFunc cmp_func);
void slist_foreach(SList *self, JustFunc func, void *userdata);
SList* slist_remove_sibling(SList *self, SListNode *sibling);
ssize_t slist_get_length(const SList *self);
void slist_node_data(const SListNode *self, void *ret);
SListNode* slist_node_next(const SListNode *self);
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
