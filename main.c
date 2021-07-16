#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#include "Base.h"
#include "DataStructs/BigInt.h"
#include "DataStructs/SList.h"
#include "DataStructs/DList.h"
#include "DataStructs/Tree.h"

int int_cmp(const void *a, const void *b)
{
	const int *ia = a; const int *ib = b;
	return *ia - *ib;
}

void int_str(const void *a, va_list *ap)
{
	const int *ia = a;
	printf("%d", *ia);
}

void test1()
{
	SList *sl = slist_new();

	srand(time(0));

	int last;

	for (int i = 0; i < 10; ++i) 
	{
		last = rand() % 100;
		slist_append(sl, GET_PTR(int, last), 4);
	}

	slist_output(sl, int_str);
	printf("\n");

	SList *rev = slist_reverse(sl);
	slist_output(rev, int_str);
	printf("\n");

	size_t count = slist_count(sl, GET_PTR(int, 0), int_cmp);
	printf("0: %lu\n", count);

	slist_remove_val(rev, GET_PTR(int, 0), int_cmp, true);
	slist_output(rev, int_str);
	printf("\n");

	SList *cp = slist_copy(sl);
	slist_output(cp, int_str);
	printf("\n");

	slist_insert(rev, 0, GET_PTR(int, 10), 4);
	slist_output(rev, int_str);
	printf("\n");

	slist_delete(rev);
}

void test2()
{
	srand(time(0));

	for (int i = 0; i < 10; ++i)
	{
		BigInt *bi1 = bi_new_int(rand());
		BigInt *bi2 = bi_new_int(rand());

		BigInt *mul = bi_mul(bi1, bi2);
		BigInt *add = bi_add(bi1, bi2);
		BigInt *sub = bi_sub(bi1, bi2);
		BigInt *div = bi_div(bi1, bi2);
		BigInt *mod = bi_mod(bi1, bi2);

		printf("bi1: ");
		bi_outputln(bi1);
		
		printf("bi2: ");
		bi_outputln(bi2);

		printf("mul: ");
		bi_outputln(mul);

		printf("add: ");
		bi_outputln(add);

		printf("sub: ");
		bi_outputln(sub);

		printf("div: ");
		bi_outputln(div);

		printf("mod: ");
		bi_outputln(mod);

		printf("\n");
	}
}

void test3()
{
	srand(time(0));

	Tree *t = tree_new(int_cmp);

	clock_t start = clock();

	for (int i = 0; i < 100; ++i) 
	{
		tree_insert(t, GET_PTR(int, rand() % 100), GET_PTR(int, rand()), 4, 4);
	}

	tree_insert(t, GET_PTR(int, 10), GET_PTR(int, 0xB00BA), 4, 4);

	clock_t end = clock();

	printf("%ld\n", (end - start) / CLOCKS_PER_SEC);

	tree_outputln(t, int_str);

	for (int i = 0; i < 10; ++i)
	{
		tree_remove(t, GET_PTR(int, rand() % 100));
	}

	tree_outputln(t, int_str);

	TreeNode *tn = tree_lookup_node(t, GET_PTR(int, 10));

	if (tn != 0)
	{
		int val;
		tree_node_value(tn, &val);
		printf("%d\n", val);
	}

	Tree *ct = tree_copy(t);

	tree_outputln(t, int_str);
	tree_outputln(ct, int_str);
}

typedef struct _TestInt
{
	DListNode parent;
	int value;
} TestInt;

void test_int_str(const void *a, va_list *ap)
{
	const TestInt *ia = a;
	printf("%d", ia->value);
}

int test_int_cmp(const void *a, const void *b)
{
	const TestInt *ia = a;
	const TestInt *ib = b;

	return ia->value - ib->value;
}

void test4()
{
	srand(time(0));

	DList *dl = dlist_new(sizeof(TestInt), NULL);

	for (int i = 0; i < 40; ++i)
	{
		TestInt *n = (TestInt*)dlist_append(dl);
		n->value = rand() % 100;
	}

	dlist_outputln(dl, test_int_str, DLIST_OUTPUT_TO_RIGHT);
	dlist_outputln(dl, test_int_str, DLIST_OUTPUT_TO_LEFT);

	dlist_sort(dl, test_int_cmp);

	dlist_outputln(dl, test_int_str, DLIST_OUTPUT_TO_RIGHT);
	dlist_outputln(dl, test_int_str, DLIST_OUTPUT_TO_LEFT);

	dlist_reverse(dl);

	dlist_outputln(dl, test_int_str, DLIST_OUTPUT_TO_RIGHT);
	dlist_outputln(dl, test_int_str, DLIST_OUTPUT_TO_LEFT);
}

void test5()
{
	srand(time(0));

	SList *sl = slist_new();

	for (int i = 0; i < 40; ++i)
	{
		slist_append(sl, GET_PTR(int, rand() % 100), 4);
	}

	slist_outputln(sl, int_str);

	slist_sort(sl, int_cmp);

	slist_outputln(sl, int_str);

	slist_reverse(sl);

	slist_outputln(sl, int_str);
}

int main(int argc, char *argv[])
{
	test4();

	return 0;
}
