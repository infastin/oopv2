#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#include "Base.h"
#include "DataStructs/BigInt.h"
#include "DataStructs/SList.h"

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
int main(int argc, char *argv[])
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

	slist_delete(rev, false);
}
