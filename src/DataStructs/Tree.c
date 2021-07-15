#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Base.h"
#include "DataStructs/Tree.h"

/* Predefinitions {{{ */

typedef enum
{
	BLACK,
	RED
} tn_color;

typedef enum
{
	LEFT,
	RIGHT
} r_dir;

struct _TreeNode
{
	TreeNode *parent;
	TreeNode *left;
	TreeNode *right;
	void *key;
	void *value;
	size_t keysize;
	size_t valuesize;
	tn_color color;
};

struct _Tree
{
	Object parent;
	TreeNode *root;
	FreeFunc vff;
	FreeFunc kff;
	CmpFunc kcf;
};

static void stringer_interface_init(StringerInterface *iface);

DEFINE_TYPE_WITH_IFACES(Tree, tree, object, 1,
		USE_INTERFACE(STRINGER_INTERFACE_TYPE, stringer_interface_init));

#define color(n) ((n == NULL) ? BLACK : (n)->color)

/* }}} Predefinitions */

/* Private methods {{{ */

static void _TreeNode_free(TreeNode *node, FreeFunc vff, FreeFunc kff)
{
	if (vff != NULL)
		vff(*(void**) node->value);

	if (kff != NULL)
		kff(*(void**) node->key);

	free(node->value);
	free(node->key);
	free(node);
}

static void _TreeNode_free_full(TreeNode *node, FreeFunc vff, FreeFunc kff)
{
	if (node == NULL)
		return;

	_TreeNode_free_full(node->left, vff, kff);
	_TreeNode_free_full(node->right, vff, kff);

	if (vff != NULL)
		vff(*(void**) node->value);

	if (kff != NULL)
		kff(*(void**) node->key);

	free(node->value);
	free(node->key);
	free(node);
}

static TreeNode* _TreeNode_new(const void *key, const void *value, size_t keysize, size_t valuesize)
{
	TreeNode *res = salloc(TreeNode, 1);
	return_val_if_fail(res != NULL, NULL);

	void *k = malloc(keysize);

	if (k == NULL)
	{
		free(res);
		return_val_if_fail(k != NULL, NULL);
	}

	void *v = malloc(valuesize);

	if (v == NULL)
	{
		free(res);
		free(k);
		return_val_if_fail(v != NULL, NULL);
	}

	if (v != NULL)
		memcpy(v, value, valuesize);
	else
		memset(v, 0, valuesize);

	memcpy(k, key, keysize);

	res->key = k;
	res->value = v;
	res->keysize = keysize;
	res->valuesize = valuesize;
	res->parent = NULL;
	res->left = NULL;
	res->right = NULL;
	res->color = RED;

	return res;
}

static TreeNode* _TreeNode_clone(const TreeNode *node)
{
	TreeNode *res = salloc(TreeNode, 1);
	return_val_if_fail(res != NULL, NULL);

	void *k = malloc(node->keysize);

	if (k == NULL)
	{
		free(res);
		return_val_if_fail(k != NULL, NULL);
	}

	void *v = malloc(node->valuesize);

	if (v == NULL)
	{
		free(res);
		free(k);
		return_val_if_fail(v != NULL, NULL);
	}

	memcpy(k, node->key, node->keysize);
	memcpy(v, node->value, node->valuesize);

	res->key = k;
	res->value = v;
	res->keysize = node->keysize;
	res->valuesize = node->valuesize;
	res->parent = NULL;
	res->left = NULL;
	res->right = NULL;
	res->color = node->color;

	return res;
}

static TreeNode* _TreeNode_cpy(const TreeNode *node)
{
	if (node == NULL)
		return NULL;

	TreeNode *cpy_node = _TreeNode_clone(node);
	return_val_if_fail(cpy_node != NULL, NULL);

	TreeNode *cpy_left = _TreeNode_cpy(node->left);
	TreeNode *cpy_right = _TreeNode_cpy(node->right);

	cpy_node->left = cpy_left;
	cpy_node->right = cpy_right;

	if (cpy_left != NULL)
		cpy_left->parent = cpy_node;
	
	if (cpy_right != NULL)
		cpy_right->parent = cpy_node;

	return cpy_node;
}

static void _Tree_string(const TreeNode *node, StringFunc str_func, va_list *ap)
{
	if (node == NULL)
		return;

	va_list ap_copy;

	va_copy(ap_copy, *ap);
	_Tree_string(node->left, str_func, &ap_copy);
	va_end(ap_copy);

	va_copy(ap_copy, *ap);
	str_func(node->key, &ap_copy);
	printf(" ");
	va_end(ap_copy);

	va_copy(ap_copy, *ap);
	_Tree_string(node->right, str_func, &ap_copy);
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

static void _TreeNode_prepare_remove(TreeNode *node, TreeNode **n, TreeNode **c)
{
	if (node->left != NULL && node->right != NULL)
	{
		TreeNode *s = node->right;
		for (; s->left != NULL; s = s->left);

		TreeNode *tmp_left = node->left;
		TreeNode *tmp_right = node->right;
		TreeNode *tmp_parent = node->parent;
		tn_color  tmp_color = node->color;

		node->left = s->left;
		node->right = s->right;
		node->parent = s->parent;
		node->color = s->color;

		s->left = tmp_left;
		s->right = tmp_right;
		s->parent = tmp_parent;
		s->color = tmp_color;

		node = s;
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

	CmpFunc key_cmp_func = va_arg(*ap, CmpFunc);

	if (key_cmp_func == NULL)
	{
		object_delete(_self, false);
		return_val_if_fail(key_cmp_func != NULL, NULL);
	}

	self->kcf = key_cmp_func;
	self->kff = NULL;
	self->vff = NULL;
	self->root = NULL;

	return _self;
}

static Object* Tree_dtor(Object *_self, va_list *ap)
{
	Tree *self = TREE(_self);

	if (self->root != NULL)
		_TreeNode_free_full(self->root, self->vff, self->kff);

	return _self;
}

static TreeNode* Tree_insert(Tree *self, const void *key, const void *value, size_t keysize, size_t valuesize);
static Tree* Tree_replace(Tree *self, const void *key, const void *value, size_t valuesize);
static void Tree_lookup(const Tree *self, const void *key, void *ret);

static Object* Tree_set(Object *_self, va_list *ap)
{
	Tree *self = TREE(_self);

	const void *key = va_arg(*ap, const void*);
	const void *value = va_arg(*ap, const void*);
	size_t keysize = va_arg(*ap, size_t);
	size_t valuesize = va_arg(*ap, size_t);

	return_val_if_fail(key != NULL, NULL);
	return_val_if_fail(valuesize != 0, NULL);
	return_val_if_fail(keysize != 0, NULL);

	if (Tree_replace(self, key, value, valuesize) == NULL)
		return_val_if_fail(Tree_insert(self, key, value, keysize, valuesize) != NULL, NULL);

	return (Object*) self;
}

static void Tree_get(const Object *_self, va_list *ap)
{
	Tree *self = TREE(_self);

	const void *key = va_arg(*ap, const void*);
	void *ret = va_arg(*ap, void*);

	return_if_fail(key != NULL);
	return_if_fail(ret != NULL);

	Tree_lookup(self, key, ret);
}

static Object* Tree_cpy(const Object *_self, Object *_object)
{
	const Tree *self = TREE(_self);
	Tree *object = TREE(OBJECT_CLASS(OBJECT_TYPE)->cpy(_self, _object));

	object->vff = self->vff;
	object->kff = self->kff;
	object->kcf = self->kcf;
	object->root = _TreeNode_cpy(self->root);

	return (Object*) object;
}

static void Tree_string(const Stringer *_self, va_list *ap)
{
	const Tree *self = TREE((const Object*) _self);

	StringFunc str_func = va_arg(*ap, StringFunc);
	return_if_fail(str_func != NULL);

	_Tree_string(self->root, str_func, ap);
}

/* }}} Base */

static TreeNode* Tree_insert(Tree *self, const void *key, const void *value, size_t keysize, size_t valuesize)
{
	if (self->root == NULL)
	{
		self->root = _TreeNode_new(key, value, keysize, valuesize);
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

	TreeNode *node = _TreeNode_new(key, value, keysize, valuesize);
	return_val_if_fail(node != NULL, NULL);

	if (lr == 0)
		current->left = node;
	else
		current->right = node;

	node->parent = current;

	_Tree_recolor(self, node);

	return node;
}

static TreeNode* Tree_lookup_node(const Tree *self, const void *key)
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

static void Tree_lookup(const Tree *self, const void *key, void *ret)
{
	TreeNode *node = Tree_lookup_node(self, key);

	if (node == NULL)
		return;

	memcpy(ret, node->value, node->valuesize);
}

static Tree* Tree_replace(Tree *self, const void *key, const void *value, size_t valuesize)
{
	TreeNode *node = Tree_lookup_node(self, key);

	if (node == NULL)
		return NULL;

	void *v = malloc(valuesize);
	return_val_if_fail(v != NULL, NULL);

	if (v != NULL)
		memcpy(v, value, valuesize);
	else
		memset(v, 0, valuesize);

	if (self->vff != NULL)
		self->vff(*(void**) node->value);

	free(node->value);
	node->value = v;

	return self;
}

static Tree* Tree_replace_node(Tree *self, TreeNode *node, const void *value, size_t valuesize)
{
	void *v = malloc(valuesize);
	return_val_if_fail(v != NULL, NULL);

	if (v != NULL)
		memcpy(v, value, valuesize);
	else
		memset(v, 0, valuesize);

	if (self->vff != NULL)
		self->vff(*(void**) node->value);

	free(node->value);
	node->value = v;

	return self;
}

static Tree* Tree_remove(Tree *self, const void *key)
{
	TreeNode *node = Tree_lookup_node(self, key);

	if (node == NULL)
		return NULL;

	TreeNode *n, *c, *p, *s;

	_TreeNode_prepare_remove(node, &n, &c);

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

	_TreeNode_free(n, self->vff, self->kff);

	return self;
}

/* }}} Public methods */

/* Selectors {{{ */

Tree* tree_new(CmpFunc key_cmp_func)
{
	return_val_if_fail(key_cmp_func != NULL, NULL);
	return (Tree*)object_new(TREE_TYPE, key_cmp_func);
}

void tree_delete(Tree *self)
{
	return_if_fail(IS_TREE(self));
	object_delete((Object*) self);
}

Tree* tree_set(Tree *self, const void *key, const void *value, size_t keysize, size_t valuesize)
{
	return_val_if_fail(IS_TREE(self), NULL);
	return_val_if_fail(key != NULL, NULL);
	return_val_if_fail(keysize != 0, NULL);
	return_val_if_fail(valuesize != 0, NULL);

	return (Tree*)object_set((Object*) self, key, value, keysize, valuesize);
}

Tree* tree_copy(const Tree *self)
{
	return_val_if_fail(IS_TREE(self), NULL);
	return (Tree*)object_copy((Object*) self);
}

void tree_get(const Tree *self, const void *key, void *ret)
{
	return_if_fail(IS_TREE(self));
	return_if_fail(key != NULL);
	return_if_fail(ret != NULL);

	object_get((Object*) self, key, ret);
}

TreeNode* tree_insert(Tree *self, const void *key, const void *value, size_t keysize, size_t valuesize)
{
	return_val_if_fail(IS_TREE(self), NULL);
	return_val_if_fail(key != NULL, NULL);
	return_val_if_fail(keysize != 0, NULL);
	return_val_if_fail(valuesize != 0, NULL);

	return Tree_insert(self, key, value, keysize, valuesize);
}

TreeNode* tree_lookup_node(const Tree *self, const void *key)
{
	return_val_if_fail(IS_TREE(self), NULL);
	return_val_if_fail(key != NULL, NULL);

	return Tree_lookup_node(self, key);
}

void tree_lookup(const Tree *self, const void *key, void *ret)
{
	return_if_fail(IS_TREE(self));
	return_if_fail(key != NULL);
	return_if_fail(ret != NULL);

	Tree_lookup(self, key, ret);
}

void tree_set_value_free_func(Tree *self, FreeFunc value_free_func)
{
	return_if_fail(IS_TREE(self));
	return_if_fail(value_free_func != NULL);

	self->vff = value_free_func;
}

void tree_set_key_free_func(Tree *self, FreeFunc key_free_func)
{
	return_if_fail(IS_TREE(self));
	return_if_fail(key_free_func != NULL);

	self->kff = key_free_func;
}

void tree_node_key(const TreeNode *self, void *ret)
{
	return_if_fail(self != NULL);
	memcpy(ret, self->key, self->keysize);
}

void tree_node_value(const TreeNode *self, void *ret)
{
	return_if_fail(self != NULL);
	memcpy(ret, self->value, self->valuesize);
}

Tree* tree_remove(Tree *self, const void *key)
{
	return_val_if_fail(IS_TREE(self), NULL);
	return_val_if_fail(key != NULL, NULL);
	return Tree_remove(self, key);
}

Tree* tree_replace(Tree *self, const void *key, const void *value, size_t valuesize)
{
	return_val_if_fail(IS_TREE(self), NULL);
	return_val_if_fail(key != NULL, NULL);
	return_val_if_fail(valuesize != 0, NULL);

	return Tree_replace(self, key, value, valuesize);
}

Tree* tree_replace_node(Tree *self, TreeNode *node, const void *value, size_t valuesize)
{
	return_val_if_fail(IS_TREE(self), NULL);
	return_val_if_fail(node != NULL, NULL);
	return_val_if_fail(valuesize != 0, NULL);

	return Tree_replace_node(self, node, value, valuesize);
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
	OBJECT_CLASS(klass)->set = Tree_set;
	OBJECT_CLASS(klass)->get = Tree_get;
}

/* }}} Init */
