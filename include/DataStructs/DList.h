#ifndef DLIST_H_WSFB7UXG
#define DLIST_H_WSFB7UXG

#include <stddef.h>

#include "Base.h"
#include "Interfaces/StringerInterface.h"

#define DLIST_TYPE (dlist_get_type())
DECLARE_TYPE(DList, dlist, DLIST, Object);

typedef struct _DlistNode DListNode;

struct _DlistNode
{
	DListNode *next;
	DListNode *prev;
	void *data;
	size_t size;
};

typedef enum {
	DLIST_OUTPUT_TO_RIGHT = 100,
	DLIST_OUTPUT_TO_LEFT,
} DListOutputDirection;

DList* dlist_new(void);
void   dlist_delete(DList *self, bool free_data);
DList* dlist_copy(const DList *self);
DListNode* dlist_node_set(DListNode *self, const void *data, size_t size);
DListNode* dlist_append(DList *self, const void *data, size_t size);
DListNode* dlist_prepend(DList *self, const void *data, size_t size);
DListNode* dlist_insert(DList *self, size_t index, const void *data, size_t size);
DListNode* dlist_insert_before_val(DList *self, const void *target, CmpFunc cmp_func, const void *data, size_t size);
DListNode* dlist_insert_before(DList *self, DListNode *sibling, const void *data, size_t size);
DListNode* dlist_find(DList *self, const void *target, CmpFunc cmp_func);
DList* dlist_remove_val(DList *self, const void *target, CmpFunc cmp_func, bool remove_all);
void   dlist_set_free_func(DList *self, FreeFunc free_func);
void   dlist_foreach(DList *self, JustFunc func, void *userdata);
ssize_t dlist_count(const DList *self, const void *target, CmpFunc cmp_func);
DList* dlist_free_val(DList *self, const void *target, CmpFunc cmp_func, bool free_all);
DList* dlist_remove_sibling(DList *self, DListNode *sibling);
DList* dlist_free_sibling(DList *self, DListNode *sibling);
DList* dlist_swap(DList *self, DListNode *a_sib, DListNode *b_sib);
DList* dlist_splice(DList *self, DListNode *l_sib, DListNode *r_sib, DList *other, DListNode *o_sib);
ssize_t dlist_get_length(const DList *self);

#define dlist_output(self, str_func, side...)                      \
	(                                                              \
	   (IS_DLIST(self)) ?                                          \
	   (stringer_output((const Stringer*) self, str_func, side)) : \
	   (return_if_fail_warning(STRFUNC, "IS_DLIST("#self")"))      \
	)

#define dlist_outputln(self, str_func, side...)                       \
	(                                                                 \
	  	(IS_DLIST(self)) ?                                            \
	  	(stringer_outputln((const Stringer*) self, str_func, side)) : \
		(return_if_fail_warning(STRFUNC, "IS_DLIST("#self")"))        \
	)

#endif /* end of include guard: DLIST_H_WSFB7UXG */
