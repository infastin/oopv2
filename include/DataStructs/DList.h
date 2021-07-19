#ifndef DLIST_H_WSFB7UXG
#define DLIST_H_WSFB7UXG

#include <stddef.h>

#include "Base.h"
#include "Interfaces/StringerInterface.h"

#define DLIST_TYPE (dlist_get_type())
DECLARE_TYPE(DList, dlist, DLIST, Object);

typedef struct _DlistNode DListNode;

/* Dont touch fields, if you want it to work correctly */

struct _DlistNode
{
	DListNode *next;
	DListNode *prev;
};

typedef enum {
	DLIST_OUTPUT_TO_RIGHT = 100,
	DLIST_OUTPUT_TO_LEFT,
} DListOutputDirection;

DList* dlist_new(size_t size, FreeFunc free_func, CpyFunc cpy_func);
void   dlist_delete(DList *self);
DList* dlist_copy(const DList *self);
DListNode* dlist_append(DList *self);
DListNode* dlist_prepend(DList *self);
DListNode* dlist_insert(DList *self, size_t index);
DListNode* dlist_insert_before_val(DList *self, const void *target, CmpFunc cmp_func);
DListNode* dlist_insert_before(DList *self, DListNode *sibling);
DListNode* dlist_find(DList *self, const void *target, CmpFunc cmp_func);
DList* dlist_remove_val(DList *self, const void *target, CmpFunc cmp_func, bool remove_all);
void   dlist_foreach(DList *self, JustFunc func, void *userdata);
ssize_t dlist_count(const DList *self, const void *target, CmpFunc cmp_func);
DList* dlist_remove_sibling(DList *self, DListNode *sibling);
DList* dlist_splice(DList *self, DListNode *l_sib, DListNode *r_sib, DList *other, DListNode *o_sib);
ssize_t dlist_get_length(const DList *self);
void dlist_sort(DList *self, CmpFunc cmp_func);
DList* dlist_reverse(DList *self);
DList* dlist_swap(DList *self, DListNode *a, DListNode *b);
DListNode* dlist_pop(DList *self);
bool dlist_is_empty(const DList *self);

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
