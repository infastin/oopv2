#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "Base.h"
#include "DataStructs/Tree.h"

typedef struct _TreeInt
{
	TreeNode parent;
	int value;
} TreeInt;

int int_cmp(const void *a, const void *b)
{
	int ia = PTR_TO_INT(a);
	int ib = PTR_TO_INT(b);

	return ia - ib;
}

void int_str(const void *a, va_list *ap)
{
	int ia = PTR_TO_INT(a);

	printf("%d", ia);
}

void tree_int_cpy(void *_dst, const void *_src)
{
	TreeInt *dst = _dst;
	const TreeInt *src = _src;

	dst->value = src->value;
}

void tree_int_str(const void *a, va_list *ap)
{
	const TreeInt *ia = a;
	printf("%d", ia->value);
}

void speed_test_insert(void)
{
	Tree *t = tree_new(sizeof(TreeInt), int_cmp, NULL, NULL, tree_int_cpy);

	clock_t start = clock();

	for (int i = 0; i < 5000000; ++i) 
	{
		TreeInt *n = (TreeInt*)tree_insert(t, INT_TO_PTR(i));
		if (n != NULL)
			n->value = rand();
	}

	clock_t end = clock();

	printf("Вставка 5000000 эл.: %lf секунд\n", (double) (end - start) / CLOCKS_PER_SEC);

	start = clock();
	TreeInt *f = (TreeInt*)tree_lookup(t, INT_TO_PTR(1000000));
	end = clock();

	printf("Поиск среди 5000000 эл.: %lf секунд\n", (double) (end - start) / CLOCKS_PER_SEC);

	start = clock();
	tree_remove(t, INT_TO_PTR(1000000));
	end = clock();

	printf("Удаление среди 5000000 эл.: %lf секунд\n", (double) (end - start) / CLOCKS_PER_SEC);

	start = clock();
	Tree *ct = tree_copy(t);
	end = clock();

	printf("Копирование дерева из 5000000 эл.: %lf секунд\n", (double) (end - start) / CLOCKS_PER_SEC);

	start = clock();
	tree_delete(t);
	end = clock();

	printf("Освобождение дерева из 5000000 эл.: %lf секунд\n", (double) (end - start) / CLOCKS_PER_SEC);
}

int main(int argc, char *argv[])
{
	srand(time(0));

	Tree *t = tree_new(sizeof(TreeInt), int_cmp, NULL, NULL, tree_int_cpy);

	printf("Вставка:\n");

	for (int i = 0; i < 10; ++i) 
	{
		TreeInt *n = (TreeInt*)tree_insert(t, INT_TO_PTR(i));
		if (n != NULL)
			n->value = rand() % 100;
	}

	tree_outputln(t, int_str, tree_int_str);

	printf("Удаление:\n");

	tree_remove(t, INT_TO_PTR(3));
	tree_remove(t, INT_TO_PTR(9));
	tree_remove(t, INT_TO_PTR(1));
	tree_remove(t, INT_TO_PTR(6));

	tree_outputln(t, int_str, tree_int_str);

	printf("Поиск:\n");
	TreeInt *f = (TreeInt*)tree_lookup(t, INT_TO_PTR(8));
	printf("%d\n", f->value);

	printf("Копирование:\n");
	Tree *ct = tree_copy(t);
	tree_outputln(ct, int_str, tree_int_str);

	// Очистка
	tree_delete(t);
	tree_delete(ct);

	printf("\nТест на скорость:\n");
	speed_test_insert();

	return 0;
}
