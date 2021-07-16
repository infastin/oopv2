#ifndef TREE_H_42B0KQLC
#define TREE_H_42B0KQLC

#include "Base.h"
#include "Interfaces/StringerInterface.h"

#define TREE_TYPE (tree_get_type())
DECLARE_TYPE(Tree, tree, TREE, Object);

typedef enum
{
	BLACK,
	RED
} tn_color;

typedef struct _TreeNode TreeNode;

struct _TreeNode
{
	TreeNode *parent;
	TreeNode *left;
	TreeNode *right;
	void *key;
	tn_color color;
};

Tree* tree_new(size_t size, CmpFunc key_cmp_func, FreeFunc key_free_func, FreeFunc node_free_func, CpyFunc node_cpy_func);
void tree_delete(Tree *self);
Tree* tree_copy(const Tree *self);
TreeNode* tree_insert(Tree *self, void *key);
TreeNode* tree_lookup(const Tree *self, const void *key);
Tree* tree_remove(Tree *self, const void *key);

#define tree_output(self, key_str_func, node_str_func...)                        \
	(                                                                            \
		(IS_TREE(self)) ?                                                        \
		(stringer_output((const Stringer*) self, key_str_func, node_str_func)) : \
		(return_if_fail_warning(STRFUNC, "IS_TREE("#self")"))                    \
	)

#define tree_outputln(self, key_str_func, node_str_func...)                        \
	(                                                                              \
		(IS_TREE(self)) ?                                                          \
		(stringer_outputln((const Stringer*) self, key_str_func, node_str_func)) : \
		(return_if_fail_warning(STRFUNC, "IS_TREE("#self")"))                      \
	)

#endif /* end of include guard: TREE_H_42B0KQLC */

