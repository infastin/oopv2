#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>

#include "Base.h"
#include "DataStructs/Array.h"

typedef struct _TestArray
{
	int key;
	int value;
} TestArray;

int test_array_cmp(const void *a, const void *b)
{
	const TestArray *ia = a;
	const TestArray *ib = b;

	return ia->key - ib->key;
}

Array* test_array_new(void)
{
	return array_new(false, false, sizeof(TestArray), NULL);
}

bool test_array_find(const Array *arr, int key, size_t *index)
{
	return array_linear_search(arr, GET_PTR(TestArray, key, 0), test_array_cmp, index);
}

void test_array_set(Array *arr, int key, int value)
{
	size_t index;
	bool check = test_array_find(arr, key, &index);

	if (check == true)
		array_set(arr, index, GET_PTR(TestArray, key, value));
	else
		array_append(arr, GET_PTR(TestArray, key, value));
}

void test_array_append(Array *arr, int key, int value, bool dont_check)
{
	if (dont_check == false)
	{
		size_t index;
		bool check = test_array_find(arr, key, &index);

		if (check == true)
			return;
	}

	array_append(arr, GET_PTR(TestArray, key, value));
}

void test_array_remove(Array *arr, int key)
{
	size_t index;
	bool check = test_array_find(arr, key, &index);

	if (check == true)
		array_remove_index(arr, index);
}

void test_array_get(const Array *arr, int key, TestArray *ta)
{
	size_t index;
	bool check = test_array_find(arr, key, &index);

	if (check == false)
		return;

	array_get(arr, index, ta);
}

int main(int argc, char *argv[])
{
	srand(time(0));

	Array *a = test_array_new();

	clock_t start = clock();

	for (int i = 0; i < 5000000; ++i)
	{
		test_array_append(a, i, rand(), true);
	}

	clock_t end = clock();

	printf("Вставка 5000000 эл.: %lf секунд\n", (double) (end - start) / CLOCKS_PER_SEC);

	start = clock();
	test_array_remove(a, 100000);
	end = clock();

	printf("Удаление среди 5000000 эл.: %lf секунд\n", (double) (end - start) / CLOCKS_PER_SEC);

	start = clock();
	TestArray ta;
	test_array_get(a, 10000, &ta);
	end = clock();

	printf("Поиск среди 5000000 эл.: %lf секунд\n", (double) (end - start) / CLOCKS_PER_SEC);

	start = clock();
	array_delete(a);
	end = clock();

	printf("Освобождение массива из 5000000 эл.: %lf секунд\n", (double) (end - start) / CLOCKS_PER_SEC);

	return 0;
}
