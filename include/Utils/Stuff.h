#ifndef UTILS_H_VEC3QZ7Z
#define UTILS_H_VEC3QZ7Z

#include <stdarg.h>
#include <string.h>

char* strdup_printf(const char *fmt, ...);
char* strdup_vprintf(const char *fmt, va_list *ap);

unsigned int pow2(unsigned int value);
unsigned long pow2l(unsigned long value);
unsigned long long pow2ll(unsigned long long value);

#endif /* end of include guard: UTILS_H_VEC3QZ7Z */
