#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "Base.h"
#include "DataStructs/DList.h"

typedef struct _TestString
{
	DListNode parent;
	char *value;
} TestString;

void test_string_str(const void *a, va_list *ap)
{
	const TestString *ia = a;
	printf("%s", ia->value);
}

int test_string_cmp(const void *a, const void *b)
{
	const TestString *ia = a;
	const TestString *ib = b;

	size_t a_len = strlen(ia->value);
	size_t b_len = strlen(ib->value);

	if (a_len > b_len)
		return 1;
	else if (a_len < b_len)
		return -1;

	return strcmp(ia->value, ib->value);
}

void test_string_cpy(void *_dst, const void *_src)
{
	TestString *dst = _dst;
	const TestString *src = _src;

	dst->value = strdup(src->value);
}

void test_string_free(void *_self)
{
	TestString *self = _self;
	free(self->value);
	free(self);
}

char *mkrndstr(size_t length) { 

	static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!"; // could be const
	char *randomString = NULL;

	if (length) {
		randomString = malloc(length + 1); 

		if (randomString) {
			int l = (int) (sizeof(charset) - 1); 
			int key;
			for (int n = 0; n < length; ++n) {        
				key = rand() % l;
				randomString[n] = charset[key];
			}

			randomString[length] = '\0';
		}
	}

	return randomString;
}

int main(int argc, char *argv[])
{
	srand(time(0));

	DList *dl = dlist_new(sizeof(TestString), test_string_free, test_string_cpy);

	for (int i = 0; i < 50; ++i)
	{
		TestString *n = (TestString*)dlist_append(dl);
		n->value = mkrndstr((rand() % 50) + 1);
	}

	printf("До сортировки:\n");
	dlist_outputln(dl, test_string_str, DLIST_OUTPUT_TO_RIGHT);
	dlist_sort(dl, test_string_cmp);
	printf("После сортировки:\n");
	dlist_outputln(dl, test_string_str, DLIST_OUTPUT_TO_RIGHT);

	return 0;
}
