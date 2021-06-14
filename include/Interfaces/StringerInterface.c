#include "StringerInterface.h"
#include "Interface.h"
#include "Messages.h"
#include <stdarg.h>

DEFINE_INTERFACE(Stringer, stringer);

void stringer_output(const Stringer *self, ...)
{
	return_if_fail(IS_STRINGER(self));

	StringerInterface *iface = STRINGER_INTERFACE(self);

	return_if_fail(iface->string != NULL);

	va_list ap;
	va_start(ap, self);
	iface->string(self, &ap);
	va_end(ap);
}

void stringer_output_va(const Stringer *self, va_list *ap)
{
	return_if_fail(IS_STRINGER(self));

	StringerInterface *iface = STRINGER_INTERFACE(self);

	return_if_fail(iface->string != NULL);

	iface->string(self, ap);
}
