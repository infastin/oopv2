#ifndef STRINGERINTERFACE_H_JBS40NEU
#define STRINGERINTERFACE_H_JBS40NEU

#include <stdarg.h>

#include "Base.h"

#define STRINGER_INTERFACE_TYPE (stringer_interface_get_type())
DECLARE_INTERFACE(Stringer, stringer, STRINGER);

typedef void (*StringFunc)(const void *a, va_list *ap);

struct _StringerInterface
{
	Interface parent;

	void (*string)(const Stringer *self, va_list *ap);
};

void stringer_outputln(const Stringer *self, ...);
void stringer_output(const Stringer *self, ...);

void stringer_va_output(const Stringer *self, va_list *ap);
void stringer_va_outputln(const Stringer *self, va_list *ap);

#endif /* end of include guard: STRINGERINTERFACE_H_JBS40NEU */
