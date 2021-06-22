#ifndef DLIST_H_WSFB7UXG
#define DLIST_H_WSFB7UXG

#include <stddef.h>

#include "Object.h"
#include "StringerInterface.h"

#define DLIST_TYPE (dlist_get_type())
DECLARE_DERIVABLE_TYPE(DList, dlist, DLIST, Object);

typedef struct _DlistNode DListNode;

struct _DListClass
{
	ObjectClass parent;

	DListNode* 	(*append)(DList *self, const void *data);
	DListNode* 	(*prepend)(DList *self, const void *data);
	DListNode* 	(*insert)(DList *self, size_t index, const void *data);
	DListNode* 	(*find)(DList *self, const void *target, CmpFunc cmp_func);
	DListNode* 	(*insert_before)(DList *self, DListNode *sibling, const void *data);
	DListNode* 	(*insert_before_val)(DList *self, const void *target, CmpFunc cmp_func, const void *data);
	DList* 		(*remove_val)(DList *self, const void *target, CmpFunc cmp_func, bool remove_all);
	DList* 		(*free_val)(DList *self, const void *target, CmpFunc cmp_func, bool free_all);
	void 		(*set_free_func)(DList *self, FreeFunc free_func);
	void 		(*foreach)(DList *self, JustFunc func, void *userdata);
	DList* 		(*swap)(DList *self, DListNode *a_sib, DListNode *b_sib);
	size_t 	    (*count)(const DList *self, const void *target, CmpFunc cmp_func);
	DList* 	   	(*remove_sibling)(DList *self, DListNode *sibling);
	DList* 	   	(*free_sibling)(DList *self, DListNode *sibling);
	DList* 		(*splice)(DList *self, DListNode *l_sib, DListNode *r_sib, DList *other, DListNode *o_sib);
};

struct _DlistNode
{
	DListNode *next;
	DListNode *prev;
	void *data;
};

struct _DList
{
	Object parent;
	void *data[2];
	size_t len;
	DListNode *start;
	DListNode *end;
};

typedef enum {
	DLIST_OUTPUT_TO_RIGHT = 174,
	DLIST_OUTPUT_TO_LEFT,
} DListOutputDirection;

DList* dlist_new(size_t elemsize);
void   dlist_delete(DList *self, bool free_data);
DList* dlist_copy(const DList *self);
DList* dlist_set(DList *self, DListNode *sibling, const void *data);
void   dlist_get(const DList *self, DListNode *sibling, void *ret);
DListNode* dlist_append(DList *self, const void *data);
DListNode* dlist_prepend(DList *self, const void *data);
DListNode* dlist_insert(DList *self, size_t index, const void *data);
DListNode* dlist_insert_before_val(DList *self, const void *target, CmpFunc cmp_func, const void *data);
DListNode* dlist_insert_before(DList *self, DListNode *sibling, const void *data);
DListNode* dlist_find(DList *self, const void *target, CmpFunc cmp_func);
DList* dlist_remove_val(DList *self, const void *target, CmpFunc cmp_func, bool remove_all);
void   dlist_set_free_func(DList *self, FreeFunc free_func);
void   dlist_foreach(DList *self, JustFunc func, void *userdata);
size_t dlist_count(const DList *self, const void *target, CmpFunc cmp_func);
DList* dlist_free_val(DList *self, const void *target, CmpFunc cmp_func, bool free_all);
DList* dlist_remove_sibling(DList *self, DListNode *sibling);
DList* dlist_free_sibling(DList *self, DListNode *sibling);
DList* dlist_swap(DList *self, DListNode *a_sib, DListNode *b_sib);
DList* dlist_splice(DList *self, DListNode *l_sib, DListNode *r_sib, DList *other, DListNode *o_sib);

#define dlist_output(self, str_func, od...) (stringer_output((Stringer*) self, str_func, od))
#define dlist_outputln(self, str_func, od...) (stringer_outputln((Stringer*) self, str_func, od))

#endif /* end of include guard: DLIST_H_WSFB7UXG */
