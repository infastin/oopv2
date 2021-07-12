#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "Utils/Stuff.h"

char* strdup_printf(const char *fmt, ...)
{
	va_list ap, ap_copy;
	va_start(ap, fmt);
	va_copy(ap_copy, ap);


	int len = vsnprintf(NULL, 0, fmt, ap_copy);
	char *result = (char*)calloc(sizeof(char), len + 1);
	vsnprintf(result, len + 1, fmt, ap);

	va_end(ap_copy);
	va_end(ap);

	return result;
}

char* strdup_vprintf(const char *fmt, va_list *ap)
{
	va_list ap_copy;
	va_copy(ap_copy, *ap);


	int len = vsnprintf(NULL, 0, fmt, ap_copy);
	char *result = (char*)calloc(sizeof(char), len + 1);
	vsnprintf(result, len + 1, fmt, *ap);

	va_end(ap_copy);

	return result;
}

unsigned int pow2(unsigned int value)
{
	return (1 << (32 - __builtin_clz(value)));
}

unsigned long pow2l(unsigned long value)
{
	return (1 << (64 - __builtin_clzl(value)));
}

unsigned long long pow2ll(unsigned long long value)
{
	return (1 << (64 - __builtin_clzl(value)));
}
