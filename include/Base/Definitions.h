#ifndef DEFINITIONS_H_CLDPPAUZ
#define DEFINITIONS_H_CLDPPAUZ

#include <stddef.h>
#include <stdarg.h>

#define UINT_BIT (sizeof(unsigned int) * 8)
#define ULONG_BIT (sizeof(unsigned long) * 8)
#define ULLONG_BIT (sizeof(unsigned long long) * 8)

typedef unsigned long Type;

typedef void (*VoidFunc)(void);
typedef int  (*CmpFunc)(const void *a, const void *b);
typedef void (*FreeFunc)(void *ptr);
typedef void (*JustFunc)(void *data, void *userdata);
typedef void (*CpyFunc)(void *dst, const void *src);

#endif /* end of include guard: DEFINITIONS_H_CLDPPAUZ */
