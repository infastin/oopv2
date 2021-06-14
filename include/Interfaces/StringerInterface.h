#ifndef STRINGERINTERFACE_H_JBS40NEU
#define STRINGERINTERFACE_H_JBS40NEU

#include "Interface.h"
#include "Object.h"
#include <stdarg.h>

#define STRINGER_INTERFACE_TYPE (stringer_interface_get_type())
DECLARE_INTERFACE(Stringer, stringer, STRINGER);

typedef void (*str_f)(const void *a, va_list *ap);

struct _StringerInterface
{
	Interface parent;

	void (*string)(const Stringer *self, va_list *ap);
};

void stringer_output(const Stringer *self, ...);

#endif /* end of include guard: STRINGERINTERFACE_H_JBS40NEU */
