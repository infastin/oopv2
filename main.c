#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Messages.h"
#include "Object.h"
#include "Array.h"
#include "SList.h"

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

void dynarr()
{
	/* Создание динамического массива */
	Array *arr = array_new(true, false, sizeof(int));

	/* Добавление одного значения */
	array_append(arr, GET_PTR(int, 42));
	array_insert(arr, 0, GET_PTR(int, 42));
	array_set(arr, 5, GET_PTR(int, 42));

	printf("arr: ");	
	array_output(arr, int_str);
	printf("\n");

	/* Добавление нескольких значений */
	srand(time(0));
	int add_many[10];

	printf("Случайный массив:\n[");
	for (int i = 0; i < 10; ++i) 
	{
		int tmp = rand() % 100;
		add_many[i] = tmp;
		printf("%d", add_many[i]);
		if (i != 9)
			printf(" ");
	}
	printf("]\n\n");

	array_append_many(arr, add_many, 10);
	array_prepend_many(arr, add_many, 10);
	array_insert_many(arr, 5, add_many, 10);

	/* Вывод массива */
	printf("Вывод массива arr:\n");
	array_output(arr, int_str);
	printf("\n\n");

	/* Создать массив без дупликатов */
	Array *uni = array_unique(arr, int_cmp);

	printf("Массив без дупликатов:\n");	
	array_output(uni, int_str);
	printf("\n\n");

	/* Найти элемент по значению */
	int target = 42;
	size_t index;

	if (array_binary_search(arr, &target, int_cmp, &index))
	{
		printf("Нашел %d в [%lu]\n\n", target, index);
	}
	else
	{
		printf("Не нашел %d\n\n", target);
	}

	printf("Массив после поиска:\n");	
	array_output(arr, int_str);
	printf("\n\n");

	/* Удалить элемент по индексу */
	array_remove_index(arr, index, false);
	printf("Массив после удаления по индексу:\n");	
	array_output(arr, int_str);
	printf("\n\n");

	/* Удалить облаcть */
	array_remove_range(arr, 10, 2, false);
	printf("Массив после удаления области:\n");	
	array_output(arr, int_str);
	printf("\n\n");

	/* Удалить элемент по значению */
	array_remove_val(arr, GET_PTR(int, 0), int_cmp, false, false);
	printf("Массив после удаления по значению:\n");	
	array_output(arr, int_str);
	printf("\n\n");

	/* Удалить все вхождения */
	array_remove_val(arr, GET_PTR(int, 0), int_cmp, false, true);
	printf("Массив после удаления всех вхождений:\n");	
	array_output(arr, int_str);
	printf("\n\n");

	/* Получить результирующий массив */
	Array *sarr = array_new(false, false, sizeof(char));

	char h[] = "Hello World!";
	array_append_many(sarr, h, sizeof(h));

	size_t len;
	char *ret = array_steal(sarr, &len);

	printf("Размер: %lu\n", len);
	printf("Массив: %s\n", ret);

	/* Удалить массив */
	array_delete(arr, false);
	array_delete(uni, false);
	array_delete(sarr, false);
}

int main(int argc, char *argv[])
{
	SList *sl = slist_new(4);

	srand(time(0));

	int last;

	for (int i = 0; i < 10; ++i) 
	{
		last = rand() % 100;
		slist_append(sl, GET_PTR(int, last));
	}

	slist_output(sl, int_str);
	printf("\n");

	SList *rev = slist_reverse(sl);
	slist_output(rev, int_str);
	printf("\n");

	SListNode *node = slist_find(rev, &last, int_cmp);
	if (node)
		slist_set(rev, node, GET_PTR(int, 0));
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

	slist_insert(rev, 0, GET_PTR(int, 10));
	slist_output(rev, int_str);
	printf("\n");
}
