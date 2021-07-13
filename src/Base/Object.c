#include <stddef.h>
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>

#include "Base.h"

/* Predefinitions {{{ */

typedef struct
{
	unsigned int magic;
	const ObjectClass *klass;
} ObjectData;

typedef struct
{
	const Object _;
	const char *name;
	const ObjectClass *super;
	Interface **ifaces;
	size_t ifaces_count;
	size_t size;
} ObjectClassData;

#define o_data(s) ((ObjectData*) (s)->private)
#define oc_data(s) ((ObjectClassData*) (s)->private)

/* }}} */

/* Type checking, sizeOf, className {{{ */

const void* isObject(const void *_self)
{
	if (_self == NULL)
	{
		msg_warn("object is NULL!");
		return NULL;
	}

	const Object* self = _self;

	if (o_data(self)->magic != MAGIC_NUM)
	{
		msg_warn("object isn't object!");
		return NULL;
	}

	return self;
}

void* cast(Type _object_type, const void *_self)
{
	const Object *object_type = isObject((const Object*) _object_type);
	exit_if_fail(object_type != NULL);

	const Object *self = isObject(_self);
	exit_if_fail(self != NULL);

	if (_object_type != OBJECT_TYPE)
	{
		const ObjectClass *p = o_data(self)->klass;
		const ObjectClass *class = (const ObjectClass*) object_type;

		while (p != class) 
		{
			if ((Type) p == OBJECT_TYPE)
			{
				msg_critical("object can't be casted to '%s'!", oc_data(class)->name);
				exit(EXIT_FAILURE);
			}

			p = oc_data(p)->super;
		}
	}

	return (void*) _self;
}

bool isOf(const void *_self, Type _object_type)
{
	if (_self && _object_type)
	{
		const Object *object_type = isObject((const Object*) _object_type);
		return_val_if_fail(object_type != NULL, 0);

		const Object *self = isObject(_self);
		return_val_if_fail(self != NULL, 0);

		if (_object_type != OBJECT_TYPE)
		{
			const ObjectClass *p = o_data(self)->klass;
			const ObjectClass *class = (const ObjectClass*) object_type;

			while (p != class) 
			{
				if ((Type) p == OBJECT_TYPE)
					return 0;

				p = oc_data(p)->super;
			}
		}

		return 1;
	}

	return 0;
}

const void* classOf(const void *self)
{
	return o_data(OBJECT(self))->klass;
}

size_t sizeOf(const void *self)
{
	return oc_data(OBJECT_GET_CLASS(self))->size;
}

const char* className(const void *self)
{
	return oc_data(OBJECT_GET_CLASS(self))->name;
}

/* }}} */

/* Object {{{ */

static Object* object_ctor(Object *self, va_list *ap)
{
	return_val_if_fail(IS_OBJECT(self), NULL);
	return self;
}

static Object* object_dtor(Object *self, va_list *ap)
{
	return_val_if_fail(IS_OBJECT(self), NULL);
	return self;
}

static Object* object_cpy(const Object *self, Object *object)
{
	return_val_if_fail(IS_OBJECT(self), NULL);
	return object;
}

/* }}} */

/* ObjectClass {{{ */

static Object* object_class_ctor(Object *_self, va_list *ap)
{
	ObjectClass *self = OBJECT_CLASS(_self);

	ObjectClassData *private = (ObjectClassData*) self->private;

	private->name = va_arg(*ap, char*);
	private->super = va_arg(*ap, ObjectClass*);

	exit_if_fail(private->super != NULL);
	exit_if_fail(IS_OBJECT_CLASS(private->super));

	private->size = va_arg(*ap, size_t);

	const size_t offset = offsetof(ObjectClass, ctor);
	memcpy((char*) self + offset, 
			(char*) private->super + offset, 
			OBJECT_SIZE(private->super) - offset);

	typedef void (*init_class)(ObjectClass* klass);

	init_class ic = va_arg(*ap, init_class);

	if (ic != NULL)
		ic(self);

	size_t ifaces_count = va_arg(*ap, size_t);
	private->ifaces_count = oc_data(private->super)->ifaces_count + ifaces_count;
	private->ifaces = NULL;

	if (private->ifaces_count != 0)
	{
		typedef void (*init_interface)(Interface* iface);

		private->ifaces = (Interface**)calloc(sizeof(Interface*), private->ifaces_count);

		if (private->ifaces == NULL)
		{
			msg_critical("couldn't allocate memory for interfaces!");
			exit(EXIT_FAILURE);
		}

		if (oc_data(private->super)->ifaces != NULL && oc_data(private->super)->ifaces_count != 0)
		{
			for (int i = 0; i < oc_data(private->super)->ifaces_count; ++i) 
			{
				private->ifaces[i] = interface_copy(oc_data(private->super)->ifaces[i]);
			}
		}

		Type itype = va_arg(*ap, Type);

		for (int j = oc_data(private->super)->ifaces_count; itype != 0; itype = va_arg(*ap, Type)) 
		{
			init_interface ii = va_arg(*ap, init_interface);
			Interface *find;

			if ((find = interface_find(itype, private->ifaces, j)) != NULL)
				find->init = ii;
			else
				private->ifaces[j++] = interface_new(itype, ii);
		}

		interface_init_all(private->ifaces, private->ifaces_count);
	}

	return _self;
}

static Object* object_class_dtor(Object *_self, va_list *ap)
{
	ObjectClass *self = OBJECT_CLASS(_self);
	msg_info("can't destroy class '%s'!", oc_data(self)->name);

	return _self;
}

static Object* object_class_cpy(const Object *_self, Object *object)
{
	const ObjectClass *self = OBJECT_CLASS(_self);
	msg_info("can't copy class '%s'!", oc_data(self)->name);

	return object;
}

/* }}} */

/* Static Initialization {{{ */

static const ObjectClass __Object;
static const ObjectClass __ObjectClass;

static const ObjectClass __Object = {
	{
		(void*) MAGIC_NUM, (void*) &__ObjectClass,
		(void*) "Object", (void*) &__Object, (void*) NULL, (void*) 0, (void*) sizeof(Object)
	},
	object_ctor,
	object_dtor,
	object_cpy,
	NULL,
	NULL
};

static const ObjectClass __ObjectClass = {
	{
		(void*) MAGIC_NUM, (void*) &__ObjectClass,
		(void*) "ObjectClass", (void*) &__Object, (void*) NULL, (void*) 0, (void*) sizeof(ObjectClass)
	},
	object_class_ctor,
	object_class_dtor,
	object_class_cpy,
	NULL,
	NULL
};

Type object_get_type(void)
{
	return (Type) &__Object;
}

Type object_class_get_type(void)
{
	return (Type) &__ObjectClass;
}

/* }}} */

/* Selectors {{{ */


static Object* ctor(Object *self, va_list *ap)
{
	return_val_if_fail(IS_OBJECT(self), NULL);

	const ObjectClass *class = OBJECT_GET_CLASS(self);
	exit_if_fail(class->ctor != NULL);

	return class->ctor(self, ap);
}

static Object* dtor(Object *self, va_list *ap)
{
	return_val_if_fail(IS_OBJECT(self), NULL);

	const ObjectClass *class = OBJECT_GET_CLASS(self);
	exit_if_fail(class->dtor != NULL);

	return class->dtor(self, ap);
}

static Object* cpy(const Object *self, Object *object)
{
	return_val_if_fail(IS_OBJECT(self) && IS_OBJECT(object), NULL);

	const ObjectClass *class = OBJECT_GET_CLASS(self);
	exit_if_fail(class->cpy != NULL);

	return class->cpy(self, object);
}

Object* object_new(Type object_type, ...)
{
	return_val_if_fail(IS_OBJECT_CLASS(object_type), NULL);

	const ObjectClass *class = OBJECT_CLASS(object_type);
	exit_if_fail(oc_data(class)->size != 0);

	Object *object = (Object*)calloc(1, oc_data(class)->size);

	if (object == NULL)
	{
		msg_error("couldn't allocate memory for object of type '%s'!", oc_data(class)->name);
		return NULL;
	}

	o_data(object)->magic = MAGIC_NUM;
	o_data(object)->klass = class;

	va_list ap;
	va_start(ap, object_type);
	object = ctor(object, &ap);
	va_end(ap);

	if (object == NULL)
		msg_error("couldn't create object of type '%s'!", oc_data(class)->name);

	return object;
}

Object* object_new_stack(Type object_type, void *_object, ...)
{
	return_val_if_fail(IS_OBJECT_CLASS(object_type), NULL);
	return_val_if_fail(_object != NULL, NULL);

	const ObjectClass *class = OBJECT_CLASS(object_type);
	exit_if_fail(oc_data(class)->size != 0);

	Object *object = (Object*) _object;

	o_data(object)->magic = MAGIC_NUM;
	o_data(object)->klass = class;

	va_list ap;
	va_start(ap, _object);
	object = ctor(object, &ap);
	va_end(ap);

	if (object == NULL)
		msg_error("couldn't create object of type '%s'!", oc_data(class)->name);

	return object;
}

void object_delete(Object *self, ...)
{
	return_if_fail(IS_OBJECT(self));
	va_list ap;
	va_start(ap, self);
	free(dtor(self, &ap));
	va_end(ap);
}

Object* object_copy(const Object *self)
{
	return_val_if_fail(IS_OBJECT(self), NULL);

	const ObjectClass *class = OBJECT_GET_CLASS(self);
	exit_if_fail(oc_data(class)->size != 0);

	Object *object = (Object*)calloc(1, oc_data(class)->size);

	if (object == NULL)
	{
		msg_error("couldn't allocate memory for copy of object of type '%s'!",
				oc_data(class)->name);
		return NULL;
	}

	o_data(object)->magic = MAGIC_NUM;
	o_data(object)->klass = class;

	object = cpy(self, object);

	if (object == NULL)
		msg_error("couldn't create copy of object of type '%s'!",
				oc_data(class)->name);

	return object;
}

Object* object_set(Object *self, ...)
{
	return_val_if_fail(IS_OBJECT(self), NULL);

	const ObjectClass *class = OBJECT_GET_CLASS(self);
	return_val_if_fail(class->set != NULL, NULL);

	va_list ap;
	va_start(ap, self);
	Object *result = class->set(self, &ap);
	va_end(ap);

	return result;
}

void object_get(const Object *self, ...)
{
	return_if_fail(IS_OBJECT(self));

	const ObjectClass *class = OBJECT_GET_CLASS(self);
	return_if_fail(class->get != NULL);

	va_list ap;
	va_start(ap, self);
	class->get(self, &ap);
	va_end(ap);
}

/* }}} */

/* vim: set fdm=marker : */
