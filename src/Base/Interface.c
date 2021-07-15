#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Base.h"

/* Predefinitions {{{ */

#define IFACE_MAGIC_NUM 0xB00BA
#define IFACE_TYPE_MAGIC_NUM 0xAAAAA

typedef struct _InterfaceType InterfaceType;

struct _InterfaceType
{
	unsigned long magic;
	const char *name;
	InterfaceType **itypes;
	size_t itypes_count;
	size_t size;
};

typedef struct
{
	unsigned long magic;
	const InterfaceType* itype;
	void (*init)(Interface* interface);
	Interface **ifaces;
	size_t ifaces_count;
} InterfaceData;

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
#define i_data(s) ((InterfaceData*) (s)->private)

/* }}} */

/* Type checking {{{ */

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

	if (i_data(iface)->magic != IFACE_MAGIC_NUM)
	{
		msg_warn("interface isn't interface!");
		return NULL;
	}

	return iface;
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

/* }}} */

/* Interface {{{ */

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

	InterfaceData *idata = i_data(iface);

	idata->magic = IFACE_MAGIC_NUM;
	idata->itype = itype;
	idata->init = main_init;
	idata->ifaces_count = itype->itypes_count;
	idata->ifaces = NULL;

	if (idata->ifaces_count != 0)
	{
		idata->ifaces = (Interface**)calloc(idata->ifaces_count, sizeof(Interface*));

		if (idata->ifaces == NULL)
		{
			msg_critical("couldn't allocate memory for interfaces of interface of type '%s'!", itype->name);
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < idata->ifaces_count; ++i) 
		{
			idata->ifaces[i] = interface_new((Type) itype->itypes[i], NULL);
		}
	}

	return iface;
}

Interface* interface_find(Type itype, Interface **ifaces, size_t ifaces_count)
{
	if (ifaces == NULL || itype == 0 || ifaces_count == 0)
		return NULL;

	for (int i = 0; i < ifaces_count; ++i) 
	{
		InterfaceData *idata = i_data(ifaces[i]);

		if (itype == (Type) idata->itype)
			return ifaces[i];

		if (idata->ifaces_count != 0 && idata->ifaces != NULL)
		{
			Interface *result = interface_find(itype, idata->ifaces, idata->ifaces_count);

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

	InterfaceData *idata = i_data(iface);

	Interface *result = (Interface*)calloc(1, idata->itype->size);

	if (result == NULL)
	{
		msg_critical("couldn't allocate memory for copy of interface of type '%s'!", 
				idata->itype->name);
		exit(EXIT_FAILURE);
	}

	InterfaceData *rdata = i_data(result);

	rdata->magic = IFACE_MAGIC_NUM;
	rdata->itype = idata->itype;
	rdata->ifaces_count = idata->ifaces_count;
	rdata->init = idata->init;
	rdata->ifaces = NULL;

	if (idata->ifaces_count != 0 && idata->ifaces != NULL)
	{
		rdata->ifaces = (Interface**)calloc(rdata->ifaces_count, sizeof(Interface*));

		if (rdata->ifaces == NULL)
		{
			msg_critical("couldn't allocate memory for interfaces of interface of type '%s'!",
					idata->itype->name);
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < rdata->ifaces_count; ++i) 
		{
			rdata->ifaces[i] = interface_copy(idata->ifaces[i]);
		}
	}

	return result;
}

void interface_init_all(Interface **ifaces, size_t ifaces_count)
{
	if (ifaces == NULL || ifaces_count == 0)
		return;

	for (int i = 0; i < ifaces_count; ++i) 
	{
		InterfaceData *idata = i_data(ifaces[i]);

		exit_if_fail(idata->init != NULL);
		idata->init(ifaces[i]);

		if (idata->ifaces_count != 0 && idata->ifaces != NULL)
			interface_init_all(idata->ifaces, idata->ifaces_count);
	}
}

/* }}} */

/* InterfaceType {{{ */

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

/* }}} */
