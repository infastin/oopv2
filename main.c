#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>

#include "Base.h"
#include "DataStructs/BigInt.h"
#include "DataStructs/SList.h"
#include "DataStructs/DList.h"
#include "DataStructs/Tree.h"

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

typedef struct _TestInt
{
	DListNode parent;
	int value;
} TestInt;

typedef struct _TestString
{
	DListNode parent;
	char *value;
} TestString;

typedef struct _TreeInt
{
	TreeNode parent;
	int value;
} TreeInt;

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

void test_int_cpy(void *_dst, const void *_src)
{
	TestInt *dst = _dst;
	const TestInt *src = _src;

	dst->value = src->value;
}

void test_string_str(const void *a, va_list *ap)
{
	const TestString *ia = a;
	printf("%s", ia->value);
}

int test_string_cmp(const void *a, const void *b)
{
	const TestString *ia = a;
	const TestString *ib = b;

	return strcmp(ia->value, ib->value);
}

void test_string_cpy(void *_dst, const void *_src)
{
	TestString *dst = _dst;
	const TestString *src = _src;

	dst->value = strdup(src->value);
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

void test1()
{
	SList *sl = slist_new(sizeof(TestInt), NULL, NULL);

	srand(time(0));

	int last;

	for (int i = 0; i < 10; ++i) 
	{
		last = rand() % 100;
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

	Tree *t = tree_new(sizeof(TreeInt), int_cmp, NULL, NULL, tree_int_cpy);

	clock_t start = clock();

	for (int i = 0; i < 100; ++i) 
	{
		TreeInt *n = (TreeInt*)tree_insert(t, INT_TO_PTR(rand() % 100));
		if (n != NULL)
			n->value = rand() % 1000;
	}

	clock_t end = clock();

	printf("%ld\n", (end - start) / CLOCKS_PER_SEC);

	tree_outputln(t, int_str, tree_int_str);

	for (int i = 0; i < 10; ++i)
	{
		tree_remove(t, INT_TO_PTR(rand() % 100));
	}

	tree_outputln(t, int_str, tree_int_str);

	Tree *ct = tree_copy(t);

	tree_outputln(ct, int_str, tree_int_str);
}

static char *rand_string(size_t size)
{
	char *str = malloc(size + 1);
	const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK";
	if (size) {
		--size;
		for (size_t n = 0; n < size; n++) {
			int key = rand() % (int) (sizeof charset - 1);
			str[n] = charset[key];
		}
		str[size] = '\0';
	}
	return str;
}

void test4()
{
	srand(time(0));

	DList *dl = dlist_new(sizeof(TestString), NULL, test_string_cpy);

	for (int i = 0; i < 10; ++i)
	{
		TestString *n = (TestString*)dlist_append(dl);
		n->value = rand_string(i + 1);
	}

	dlist_outputln(dl, test_string_str, DLIST_OUTPUT_TO_RIGHT);
	dlist_outputln(dl, test_string_str, DLIST_OUTPUT_TO_LEFT);

	dlist_sort(dl, test_string_cmp);

	dlist_outputln(dl, test_string_str, DLIST_OUTPUT_TO_RIGHT);
	dlist_outputln(dl, test_string_str, DLIST_OUTPUT_TO_LEFT);

	dlist_reverse(dl);

	dlist_outputln(dl, test_string_str, DLIST_OUTPUT_TO_RIGHT);
	dlist_outputln(dl, test_string_str, DLIST_OUTPUT_TO_LEFT);

	DList *cp = dlist_copy(dl);

	dlist_outputln(cp, test_string_str, DLIST_OUTPUT_TO_RIGHT);
	dlist_outputln(cp, test_string_str, DLIST_OUTPUT_TO_LEFT);
}

void test5()
{
	srand(time(0));

	SList *sl = slist_new(sizeof(TestInt), NULL, NULL);

	for (int i = 0; i < 40; ++i)
	{
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
