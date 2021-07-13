#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Base.h"

typedef struct
{
	const Object _;
	const char *name;
	const ObjectClass *super;
	Interface **ifaces;
	size_t ifaces_count;
	size_t size;
} ObjectClassData;

#define oc_data(s) ((ObjectClassData*) (s)->private)

Type isInterfaceType(Type _itype)
{
	if (_itype == 0)
	{
		msg_warn("type is NULL!");
		return 0;
	}

	InterfaceType *itype = (InterfaceType*) _itype;

	if (itype->magic != IFACE_TYPE_MAGIC_NUM)
	{
		msg_warn("type isn't type!");
		return 0;
	}

	return _itype;
}

const Interface* isInterface(const void *_iface)
{
	if (_iface == NULL)
	{
		msg_warn("interface is NULL!");
		return NULL;
	}

	const Interface *iface = _iface;

	if (iface->magic != IFACE_MAGIC_NUM)
	{
		msg_warn("interface isn't interface!");
		return NULL;
	}

	return iface;
}

Interface* interface_find(Type itype, Interface **ifaces, size_t ifaces_count)
{
	if (ifaces == NULL || itype == 0 || ifaces_count == 0)
		return NULL;

	for (int i = 0; i < ifaces_count; ++i) 
	{
		if (itype == (Type) ifaces[i]->itype)
			return ifaces[i];

		if (ifaces[i]->ifaces_count != 0 && ifaces[i]->ifaces != NULL)
		{
			Interface *result = interface_find(itype, ifaces[i]->ifaces, ifaces[i]->ifaces_count);

			if (result != NULL)
				return result;
		}
	}

	return NULL;
}

Interface* interface_copy(Interface *iface)
{
	if (iface == NULL)
		return NULL;

	exit_if_fail(isInterface((const void*) iface));

	Interface *result = (Interface*)calloc(1, iface->itype->size);

	if (result == NULL)
	{
		msg_critical("couldn't allocate memory for copy of interface of type '%s'!", 
				iface->itype->name);
		exit(EXIT_FAILURE);
	}

	result->magic = IFACE_MAGIC_NUM;
	result->itype = iface->itype;
	result->ifaces_count = iface->ifaces_count;
	result->init = iface->init;
	result->ifaces = NULL;

	if (iface->ifaces_count != 0 && iface->ifaces != NULL)
	{
		result->ifaces = (Interface**)calloc(result->ifaces_count, sizeof(Interface*));

		if (result->ifaces == NULL)
		{
			msg_critical("couldn't allocate memory for interfaces of interface of type '%s'!",
					iface->itype->name);
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < result->ifaces_count; ++i) 
		{
			result->ifaces[i] = interface_copy(iface->ifaces[i]);
		}
	}

	return result;
}

bool hasInterface(Type itype, const void *self)
{
	const ObjectClass *klass = OBJECT_GET_CLASS(self);
	return_val_if_fail(oc_data(klass)->ifaces != NULL && oc_data(klass)->ifaces_count != 0, false);

	void *result = interface_find(itype, oc_data(klass)->ifaces, oc_data(klass)->ifaces_count);
	return_val_if_fail(result != NULL, false);

	return true;
}

void* interface_cast(Type itype, const void *self)
{
	const ObjectClass *klass = OBJECT_GET_CLASS(self);
	exit_if_fail(oc_data(klass)->ifaces != NULL && oc_data(klass)->ifaces_count != 0);

	void *result = interface_find(itype, oc_data(klass)->ifaces, oc_data(klass)->ifaces_count);
	exit_if_fail(result != NULL);

	return result;
}

void interface_init_all(Interface **ifaces, size_t ifaces_count)
{
	if (ifaces == NULL || ifaces_count == 0)
		return;

	for (int i = 0; i < ifaces_count; ++i) 
	{
		exit_if_fail(ifaces[i]->init != NULL);
		ifaces[i]->init(ifaces[i]);

		if (ifaces[i]->ifaces_count != 0 && ifaces[i]->ifaces != NULL)
			interface_init_all(ifaces[i]->ifaces, ifaces[i]->ifaces_count);
	}
}

Type interface_type_new(char *name, size_t size, size_t itypes_count, ...)
{
	InterfaceType *itype;

	exit_if_fail(size != 0);

	itype = (InterfaceType*)calloc(1, sizeof(InterfaceType));

	if (itype == NULL)
	{
		msg_critical("couldn't allocate memory for interface type '%s'!", name);
		exit(EXIT_FAILURE);
	}

	itype->magic = IFACE_TYPE_MAGIC_NUM;
	itype->name = name;
	itype->size = size;
	itype->itypes_count = itypes_count;
	itype->itypes = NULL;

	if (itypes_count != 0)
	{
		itype->itypes = (InterfaceType**)calloc(itypes_count, sizeof(InterfaceType*));

		if (itype->itypes == NULL)
		{
			msg_critical("couldn't allocate memory for interface types of type '%s'!", name);
			exit(EXIT_FAILURE);
		}

		va_list ap;
		va_start(ap, itypes_count);

		for (int i = 0; i < itypes_count; ++i) 
		{
			itype->itypes[i] = va_arg(ap, InterfaceType*);
		}

		va_end(ap);
	}

	return (Type) itype;
}

Interface* interface_new(Type interface_type, void (*main_init)(Interface *iface))
{
	if (interface_type == 0)
		exit(EXIT_FAILURE);

	InterfaceType *itype = (InterfaceType*)isInterfaceType(interface_type);
	exit_if_fail(itype != NULL);
	exit_if_fail(itype->size != 0);

	Interface *iface = (Interface*)calloc(1, itype->size);

	if (iface == NULL)
	{
		msg_critical("couldn't allocate memory for interface of type '%s'!", itype->name);
		exit(EXIT_FAILURE);
	}

	iface->magic = IFACE_MAGIC_NUM;
	iface->itype = itype;
	iface->init = main_init;
	iface->ifaces_count = itype->itypes_count;
	iface->ifaces = NULL;

	if (iface->ifaces_count != 0)
	{
		iface->ifaces = (Interface**)calloc(iface->ifaces_count, sizeof(Interface*));

		if (iface->ifaces == NULL)
		{
			msg_critical("couldn't allocate memory for interfaces of interface of type '%s'!", itype->name);
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < iface->ifaces_count; ++i) 
		{
			iface->ifaces[i] = interface_new((Type) itype->itypes[i], NULL);
		}
	}

	return iface;
}
