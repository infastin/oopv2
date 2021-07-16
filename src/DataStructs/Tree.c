#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Base.h"
#include "DataStructs/Tree.h"

/* Predefinitions {{{ */

typedef enum
{
	LEFT,
	RIGHT
} r_dir;

struct _Tree
{
	Object parent;
	TreeNode *root;
	FreeFunc nff;   // Node free func
	FreeFunc kff;   // Key free func
	CmpFunc kcf;    // Key cmp func
	CpyFunc ncpf;   // Node cpy func
	size_t size;
};

static void stringer_interface_init(StringerInterface *iface);

DEFINE_TYPE_WITH_IFACES(Tree, tree, object, 1,
		USE_INTERFACE(STRINGER_INTERFACE_TYPE, stringer_interface_init));

#define color(n) ((n == NULL) ? BLACK : (n)->color)

/* }}} Predefinitions */

/* Private methods {{{ */

static void _TreeNode_free(TreeNode *node, FreeFunc nff, FreeFunc kff)
{
	if (kff)
		kff(node->key);

	nff(node);
}

static void _TreeNode_free_full(TreeNode *node, FreeFunc nff, FreeFunc kff)
{
	if (node == NULL)
		return;

	_TreeNode_free_full(node->left, nff, kff);
	_TreeNode_free_full(node->right, nff, kff);

	if (kff)
		kff(node->key);

	nff(node);
}

static TreeNode* _TreeNode_new(size_t size, void *key)
{
	TreeNode *res = (TreeNode*)calloc(1, size);
	return_val_if_fail(res != NULL, NULL);

	res->key = key;
	res->parent = NULL;
	res->left = NULL;
	res->right = NULL;
	res->color = RED;

	return res;
}

static TreeNode* _TreeNode_clone(const TreeNode *node, size_t size, CpyFunc cpy_func)
{
	TreeNode *res = (TreeNode*)calloc(1, size);
	return_val_if_fail(res != NULL, NULL);

	if (cpy_func)
		cpy_func(res, node);

	res->key = node->key;
	res->parent = NULL;
	res->left = NULL;
	res->right = NULL;
	res->color = node->color;

	return res;
}

static TreeNode* _TreeNode_cpy(const TreeNode *node, size_t size, CpyFunc cpy_func)
{
	if (node == NULL)
		return NULL;

	TreeNode *cpy_node = _TreeNode_clone(node, size, cpy_func);
	return_val_if_fail(cpy_node != NULL, NULL);

	TreeNode *cpy_left = _TreeNode_cpy(node->left, size, cpy_func);
	TreeNode *cpy_right = _TreeNode_cpy(node->right, size, cpy_func);

	cpy_node->left = cpy_left;
	cpy_node->right = cpy_right;

	if (cpy_left != NULL)
		cpy_left->parent = cpy_node;
	
	if (cpy_right != NULL)
		cpy_right->parent = cpy_node;

	return cpy_node;
}

static void _Tree_string(const TreeNode *node, StringFunc key_str_func, StringFunc node_str_func, va_list *ap)
{
	if (node == NULL)
		return;

	va_list ap_copy;

	va_copy(ap_copy, *ap);
	_Tree_string(node->left, key_str_func, node_str_func, &ap_copy);
	va_end(ap_copy);

	if (node->left != NULL)
		printf(", ");

	va_copy(ap_copy, *ap);
	key_str_func(node->key, &ap_copy);
	printf(" => ");
	node_str_func(node, &ap_copy);
	va_end(ap_copy);

	if (node->right != NULL)
		printf(", ");

	va_copy(ap_copy, *ap);
	_Tree_string(node->right, key_str_func, node_str_func, &ap_copy);
	va_end(ap_copy);
}

static TreeNode* _TreeNode_uncle(const TreeNode* node)
{
	TreeNode *p = node->parent;
	TreeNode *g = p->parent;

	return (p == g->left) ? g->right : g->left;
}

static TreeNode* _TreeNode_sibling(const TreeNode *node)
{
	TreeNode *p = node->parent;

	return (node == p->left) ? p->right : p->left; 
}

static void _Tree_rotate_root(Tree *self, TreeNode *p, r_dir dir)
{
	TreeNode *n;
	TreeNode *g = p->parent;

	if (dir == LEFT)
	{
		n = p->right;

		p->right = n->left;
		n->left = p;
		n->parent = g;
		p->parent = n;

		if (p->right != NULL)
			p->right->parent = p;
	}
	else
	{
		n = p->left;

		p->left = n->right;
		n->right = p;
		n->parent = g;
		p->parent = n;

		if (p->left != NULL)
			p->left->parent = p;
	}

	if (g != NULL)
	{
		if (p == g->right)
			g->right = n;
		else
			g->left = n;
	}
	else
		self->root = n;
}

static void _Tree_recolor(Tree *self, TreeNode *node)
{
	TreeNode *p, *g, *u, *n;

	n = node;

	while (1) 
	{
		if (n->color == RED && self->root == n)
			n->color = BLACK;

		if ((p = n->parent) == NULL)
			return;

		if (p->color == RED && self->root == p)
			p->color = BLACK;

		if (p->color == BLACK)
			return;

		g = p->parent;
		u = _TreeNode_uncle(node);

		if (p->color == RED && color(u) == RED)
		{
			p->color = BLACK;
			u->color = BLACK;
			g->color = RED;

			n = g;
		}
		else if (color(u) == BLACK)
		{
			if (n == p->right && p == g->left)
			{
				_Tree_rotate_root(self, p, LEFT);
				n = p;
				p = g->left;
			}
			else if (n == p->left && p == g->right)
			{
				_Tree_rotate_root(self, p, RIGHT);
				n = p;
				p = g->right;
			}

			if (n == p->right && p == g->right)
				_Tree_rotate_root(self, g, LEFT);
			else
				_Tree_rotate_root(self, g, RIGHT);

			p->color = BLACK;
			g->color = RED;

			return;
		}
	}
}

static void _TreeNode_swap_case1(TreeNode *a, TreeNode *b)
{
	TreeNode *old_a_left = a->left;
	TreeNode *old_a_right = a->right;

	TreeNode *old_b_parent = b->parent;
	TreeNode *old_b_left = b->left;
	TreeNode *old_b_right = b->right;

	b->parent = a;
	a->parent = old_b_parent;

	if (old_b_parent != NULL)
	{
		if (old_b_parent->left == b)
			old_b_parent->left = a;
		else
			old_b_parent->right = a;
	}

	b->left = old_a_left;
	b->right = old_a_left;

	if (old_a_left != NULL)
		old_a_left->parent = b;

	if (old_a_right != NULL)
		old_a_right->parent = b;

	if (old_b_left == a)
	{
		a->left = b;
		a->right = old_b_right;

		if (old_b_right != NULL)
			old_b_right->parent = a;
	}
	else
	{
		a->right = b;
		a->left = old_b_left;

		if (old_b_left != NULL)
			old_b_left->parent = a;
	}
}

static void _TreeNode_swap_case2(TreeNode *a, TreeNode *b)
{
	TreeNode *old_a_parent = a->parent;
	TreeNode *old_a_left = a->left;
	TreeNode *old_a_right = a->right;

	TreeNode *old_b_parent = b->parent;
	TreeNode *old_b_left = b->left;
	TreeNode *old_b_right = b->right;

	a->parent = old_b_parent;

	if (old_b_parent != NULL)
	{
		if (old_b_parent->left == b)
			old_b_parent->left = a;
		else
			old_b_parent->right = a;
	}

	b->parent = old_a_parent;

	if (old_a_parent != NULL)
	{
		if (old_a_parent->left == a)
			old_a_parent->left = b;
		else
			old_a_parent->right = b;
	}

	a->left = old_b_left;
	a->right = old_b_right;

	if (old_b_left != NULL)
		old_b_left->parent = a;

	if (old_b_right != NULL)
		old_b_right->parent = a;

	b->left = old_a_left;
	b->right = old_a_right;


	if (old_a_left != NULL)
		old_a_left->parent = b;

	if (old_a_right != NULL)
		old_a_right->parent = b;
}

static void _TreeNode_swap(TreeNode *a, TreeNode *b)
{
	if (a->parent == b)
		_TreeNode_swap_case1(a, b);
	else if (b->parent == a)
		_TreeNode_swap_case1(b, a);
	else
		_TreeNode_swap_case2(a, b);

	tn_color tmp = a->color;
	
	a->color = b->color;
	b->color = tmp;
}

static void _TreeNode_prepare_remove(Tree *self, TreeNode *node, TreeNode **n, TreeNode **c)
{
	if (node->left != NULL && node->right != NULL)
	{
		TreeNode *s = node->right;
		for (; s->left != NULL; s = s->left);

		_TreeNode_swap(node, s);

		if (self->root == node)
			self->root = s;
	}

	*n = node;

	if (node->left != NULL)
		*c = node->left;

	*c = node->right;
}

static void _Tree_replace_child(Tree *self, TreeNode *n, TreeNode *c)
{
	if (c != NULL)
		c->parent = n->parent;

	if (n->parent != NULL)
	{
		if (n == n->parent->left)
			n->parent->left = c;
		else
			n->parent->right = c;
	}
	else
		self->root = c;
}

/* }}} Private methods */

/* Public methods {{{ */

/* Base {{{ */

static Object* Tree_ctor(Object *_self, va_list *ap)
{
	Tree *self = TREE(OBJECT_CLASS(OBJECT_TYPE)->ctor(_self, ap));

	size_t size = va_arg(*ap, size_t);
	CmpFunc key_cmp_func = va_arg(*ap, CmpFunc);
	FreeFunc key_free_func = va_arg(*ap, FreeFunc);
	FreeFunc node_free_func = va_arg(*ap, FreeFunc);
	CpyFunc node_cpy_func = va_arg(*ap, CpyFunc);

	if (size < sizeof(TreeNode) || key_cmp_func == NULL)
	{
		object_delete(_self, false);
		return_val_if_fail(size >= sizeof(TreeNode), NULL);
		return_val_if_fail(key_cmp_func != NULL, NULL);
	}

	if (node_free_func == NULL)
		self->nff = free;
	else
		self->nff = node_free_func;

	self->kff = key_free_func;
	self->kcf = key_cmp_func;
	self->ncpf = node_cpy_func;
	self->size = size;
	self->root = NULL;

	return _self;
}

static Object* Tree_dtor(Object *_self, va_list *ap)
{
	Tree *self = TREE(_self);

	if (self->root != NULL)
		_TreeNode_free_full(self->root, self->nff, self->kff);

	return _self;
}

static Object* Tree_cpy(const Object *_self, Object *_object, va_list *ap)
{
	const Tree *self = TREE(_self);
	Tree *object = TREE(OBJECT_CLASS(OBJECT_TYPE)->cpy(_self, _object, ap));

	object->nff = self->nff;
	object->ncpf = self->ncpf;
	object->size = self->size;
	object->kff = self->kff;
	object->kcf = self->kcf;
	object->root = _TreeNode_cpy(self->root, self->size, self->ncpf);

	return (Object*) object;
}

static void Tree_string(const Stringer *_self, va_list *ap)
{
	const Tree *self = TREE((const Object*) _self);

	StringFunc key_str_func = va_arg(*ap, StringFunc);
	return_if_fail(key_str_func != NULL);

	StringFunc node_str_func = va_arg(*ap, StringFunc);
	return_if_fail(node_str_func != NULL);

	printf("[");
	_Tree_string(self->root, key_str_func, node_str_func, ap);
	printf("]");
}

/* }}} Base */

static TreeNode* Tree_insert(Tree *self, void *key)
{
	if (self->root == NULL)
	{
		self->root = _TreeNode_new(self->size, key);
		return_val_if_fail(self->root != NULL, NULL);

		self->root->color = BLACK;
		return self->root;
	}

	TreeNode *current = self->root;
	int lr = 0;

	while (1) 
	{
		int cmp = self->kcf(current->key, key);

		if (cmp > 0)
		{
			if (current->left == NULL)
			{
				lr = 0;
				break;
			}

			current = current->left;
		}
		else if (cmp < 0)
		{
			if (current->right == NULL)
			{
				lr = 1;
				break;
			}

			current = current->right;
		}
		else
		{
			msg_warn("the same key was found in the tree!");
			return NULL;
		}
	}

	TreeNode *node = _TreeNode_new(self->size, key);
	return_val_if_fail(node != NULL, NULL);

	if (lr == 0)
		current->left = node;
	else
		current->right = node;

	node->parent = current;

	_Tree_recolor(self, node);

	return node;
}

static TreeNode* Tree_lookup(const Tree *self, const void *key)
{
	if (self->root == NULL)
		return NULL;

	TreeNode *current = self->root;

	while (current != NULL) 
	{
		int cmp = self->kcf(current->key, key);

		if (cmp > 0)
			current = current->left;
		else if (cmp < 0)
			current = current->right;
		else
			return current;
	}

	return NULL;
}

static Tree* Tree_remove(Tree *self, const void *key)
{
	TreeNode *node = Tree_lookup(self, key);

	if (node == NULL)
		return NULL;

	TreeNode *n, *c, *p, *s;

	_TreeNode_prepare_remove(self, node, &n, &c);

	if (n->color == BLACK)
	{
		n->color = color(c);

		while ((p = n->parent) != NULL) 
		{
			s = _TreeNode_sibling(n);

			if (s->color == RED)
			{
				p->color = RED;
				s->color = BLACK;

				if (n == p->left)
					_Tree_rotate_root(self, p, LEFT);
				else
					_Tree_rotate_root(self, p, RIGHT);

				s = _TreeNode_sibling(n);
			}

			if (p->color == BLACK &&
				s->color == BLACK &&
				color(s->left) == BLACK &&
				color(s->right) == BLACK)
			{
				s->color = RED;
				n = p;
			}
			else 
			{
				if (p->color == RED &&
					s->color == BLACK &&
					color(s->left) == BLACK &&
					color(s->right) == BLACK)
				{
					s->color = RED;
					p->color = BLACK;
				}
				else 
				{
					if (n == n->parent->left &&
						s->color == BLACK &&
						color(s->left) == RED &&
						color(s->right) == BLACK)
					{
						s->color = RED;
						s->left->color = BLACK;
						_Tree_rotate_root(self, s, RIGHT);
						s = _TreeNode_sibling(n);
					}
					else if (n == n->parent->right &&
							s->color == BLACK &&
							color(s->left) == BLACK &&
							color(s->right) == RED)
					{
						s->color = RED;
						s->right->color = BLACK;
						_Tree_rotate_root(self, s, LEFT);
						s = _TreeNode_sibling(n);
					}

					s->color = p->color;
					p->color = BLACK;

					if (n == p->left)
					{
						s->right->color = BLACK;
						_Tree_rotate_root(self, p, LEFT);
					}
					else
					{
						s->left->color = BLACK;
						_Tree_rotate_root(self, p, RIGHT);
					}
				}

				break;
			}
		}
	}

	_Tree_replace_child(self, n, c);
	if (n->parent == NULL && c != NULL)
		c->color = BLACK;

	_TreeNode_free(n, self->nff, self->kff);

	return self;
}

/* }}} Public methods */

/* Selectors {{{ */

Tree* tree_new(size_t size, CmpFunc key_cmp_func, FreeFunc key_free_func, FreeFunc node_free_func, CpyFunc node_cpy_func)
{
	return_val_if_fail(size >= sizeof(TreeNode), NULL);
	return_val_if_fail(key_cmp_func != NULL, NULL);
	return (Tree*)object_new(TREE_TYPE, size, key_cmp_func, key_free_func, node_free_func, node_cpy_func);
}

void tree_delete(Tree *self)
{
	return_if_fail(IS_TREE(self));
	object_delete((Object*) self);
}

Tree* tree_copy(const Tree *self)
{
	return_val_if_fail(IS_TREE(self), NULL);
	return (Tree*)object_copy((Object*) self);
}

TreeNode* tree_insert(Tree *self, void *key)
{
	return_val_if_fail(IS_TREE(self), NULL);
	return_val_if_fail(key != NULL, NULL);

	return Tree_insert(self, key);
}

TreeNode* tree_lookup(const Tree *self, const void *key)
{
	return_val_if_fail(IS_TREE(self), NULL);
	return_val_if_fail(key != NULL, NULL);

	return Tree_lookup(self, key);
}

Tree* tree_remove(Tree *self, const void *key)
{
	return_val_if_fail(IS_TREE(self), NULL);
	return_val_if_fail(key != NULL, NULL);
	return Tree_remove(self, key);
}

/* }}} Selectors */

/* Init {{{ */

static void stringer_interface_init(StringerInterface *iface)
{
	iface->string = Tree_string;
}

static void tree_class_init(TreeClass *klass)
{
	OBJECT_CLASS(klass)->ctor = Tree_ctor;
	OBJECT_CLASS(klass)->dtor = Tree_dtor;
	OBJECT_CLASS(klass)->cpy = Tree_cpy;
}

/* }}} Init */
