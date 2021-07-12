#ifndef DEFINITIONS_H_CLDPPAUZ
#define DEFINITIONS_H_CLDPPAUZ

#include <stddef.h>

typedef unsigned long Type;

typedef void (*VoidFunc)(void);
typedef int  (*CmpFunc)(const void *a, const void *b);
typedef void (*FreeFunc)(void *ptr);
typedef void (*JustFunc)(void *data, void *userdata);

#endif /* end of include guard: DEFINITIONS_H_CLDPPAUZ */
