#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Messages.h"
#include "Object.h"
#include "Array.h"
#include "IterableInterface.h"

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
	/* Создание динамического массива */
	Array *arr = array_new(true, true, sizeof(int));

	/* Добавление одного значения */
	int add = 42;
	array_append_val(arr, &add);
	array_insert_val(arr, 0, &add);
	array_set(arr, 5, &add);

	/* Добавление нескольких значений */
	srand(time(0));
	int add_many[10];

	printf("Случайный массив: [");
	for (int i = 0; i < 10; ++i) 
	{
		int tmp = rand() % 100;
		add_many[i] = tmp;
		printf("%d", add_many[i]);
		if (i != 9)
			printf(" ");
	}
	printf("]\n");

	array_append_vals(arr, add_many, 10);
	array_prepend_vals(arr, add_many, 10);
	array_insert_vals(arr, 5, add_many, 10);

	/* Вывод массива */
	printf("arr: ");
	array_output(arr, int_str);
	printf("\n");

	/* Создать массив без дупликатов */
	Array *uni = array_unique(arr, int_cmp);
	
	printf("uni: ");	
	array_output(uni, int_str);
	printf("\n");

	/* Найти элемент по значению */
	int target = 42;
	size_t index;

	if (array_binary_search(arr, &target, int_cmp, &index))
	{
		printf("Нашел %d в [%lu]\n", target, index);
	}
	else
	{
		printf("Не нашел %d\n", target);
	}

	/* Удалить элемент по индексу */
	array_remove_index(arr, index, false);
	array_output(arr, int_str);
	printf("\n");

	/* Удалить облать */
	array_remove_range(arr, index, 2, false);
	array_output(arr, int_str);
	printf("\n");

	return 0;
}
