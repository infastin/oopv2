#ifndef TREE_H_42B0KQLC
#define TREE_H_42B0KQLC

#include "Base.h"
#include "Interfaces/StringerInterface.h"

#define TREE_TYPE (tree_get_type())
DECLARE_TYPE(Tree, tree, TREE, Object);

typedef struct _TreeNode TreeNode;

Tree* tree_new(CmpFunc key_cmp_func);
void tree_delete(Tree *self);
Tree* tree_copy(const Tree *self);
Tree* tree_set(Tree *self, const void *key, const void *value, size_t keysize, size_t valuesize);
void tree_get(const Tree *self, const void *key, void *ret);
TreeNode* tree_insert(Tree *self, const void *key, const void *value, size_t keysize, size_t valuesize);
TreeNode* tree_lookup_node(const Tree *self, const void *key);
void tree_lookup(const Tree *self, const void *key, void *ret);
void tree_set_value_free_func(Tree *self, FreeFunc value_free_func);
void tree_set_key_free_func(Tree *self, FreeFunc key_free_func);
void tree_node_key(const TreeNode *self, void *ret);
void tree_node_value(const TreeNode *self, void *ret);
Tree* tree_remove(Tree *self, const void *key);
Tree* tree_replace(Tree *self, const void *key, const void *value, size_t valuesize);
Tree* tree_replace_node(Tree *self, TreeNode *node, const void *value, size_t valuesize);

#define tree_output(self, str_func...)                        \
	(                                                         \
		(IS_TREE(self)) ?                                     \
		(stringer_output((const Stringer*) self, str_func)) : \
		(return_if_fail_warning(STRFUNC, "IS_TREE("#self")")) \
	)

#define tree_outputln(self, str_func...)                        \
	(                                                           \
		(IS_TREE(self)) ?                                       \
		(stringer_outputln((const Stringer*) self, str_func)) : \
		(return_if_fail_warning(STRFUNC, "IS_TREE("#self")"))   \
	)

#endif /* end of include guard: TREE_H_42B0KQLC */

