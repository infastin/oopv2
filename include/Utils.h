#ifndef UTILS_H_VEC3QZ7Z
#define UTILS_H_VEC3QZ7Z

#include <stdarg.h>

#define STRFUNC ((const char*) (__PRETTY_FUNCTION__))

char* strdup_printf(const char *fmt, ...);
char* strdup_vprintf(const char *fmt, va_list *ap);

unsigned int pow2(unsigned int value);
unsigned long pow2l(unsigned long value);
unsigned long long pow2ll(unsigned long long value);


#define salloc(struct_type, n_structs) ((n_structs > 0) ? (malloc(sizeof(struct_type) * n_structs)) : (NULL))
#define salloc0(struct_type, n_structs) ((n_structs > 0) ? (calloc(sizeof(struct_type) * n_structs)) : (NULL))

#endif /* end of include guard: UTILS_H_VEC3QZ7Z */
