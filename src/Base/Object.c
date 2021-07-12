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

/* Type checking and sizeOf {{{1 */

const void* isObject(const void *_self)
{
	if (_self == NULL)
	{
		msg_warn("object is NULL!");
		return NULL;
	}

	const Object* self = _self;

	if (self->magic != MAGIC_NUM)
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
		const ObjectClass *p = self->klass;
		const ObjectClass *class = (const ObjectClass*) object_type;

		while (p != class) 
		{
			if ((Type) p == OBJECT_TYPE)
			{
				msg_critical("object can't be casted to '%s'!", class->name);
				exit(EXIT_FAILURE);
			}

			p = p->super;
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
			const ObjectClass *p = self->klass;
			const ObjectClass *class = (const ObjectClass*) object_type;

			while (p != class) 
			{
				if ((Type) p == OBJECT_TYPE)
					return 0;

				p = p->super;
			}
		}

		return 1;
	}

	return 0;
}

const void* classOf(const void *self)
{
	return OBJECT(self)->klass;
}

size_t sizeOf(const void *self)
{
	return OBJECT_GET_CLASS(self)->size;
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

	self->name = va_arg(*ap, char*);
	self->super = va_arg(*ap, ObjectClass*);

	exit_if_fail(self->super != NULL);
	exit_if_fail(IS_OBJECT_CLASS(self->super));

	self->size = va_arg(*ap, size_t);

	const size_t offset = offsetof(ObjectClass, ctor);
	memcpy((char*) self + offset, 
			(char*) self->super + offset, 
			OBJECT_SIZE(self->super) - offset);

	typedef void (*init_class)(ObjectClass* klass);

	init_class ic = va_arg(*ap, init_class);

	if (ic != NULL)
		ic(self);

	size_t ifaces_count = va_arg(*ap, size_t);
	self->ifaces_count = self->super->ifaces_count + ifaces_count;
	self->ifaces = NULL;

	if (self->ifaces_count != 0)
	{
		typedef void (*init_interface)(Interface* iface);

		self->ifaces = (Interface**)calloc(sizeof(Interface*), self->ifaces_count);

		if (self->ifaces == NULL)
		{
			msg_critical("couldn't allocate memory for interfaces!");
			exit(EXIT_FAILURE);
		}

		if (self->super->ifaces != NULL && self->super->ifaces_count != 0)
		{
			for (int i = 0; i < self->super->ifaces_count; ++i) 
			{
				self->ifaces[i] = interface_copy(self->super->ifaces[i]);
			}
		}

		Type itype = va_arg(*ap, Type);

		for (int j = self->super->ifaces_count; itype != 0; itype = va_arg(*ap, Type)) 
		{
			init_interface ii = va_arg(*ap, init_interface);
			Interface *find;

			if ((find = interface_find(itype, self->ifaces, j)) != NULL)
				find->init = ii;
			else
				self->ifaces[j++] = interface_new(itype, ii);
		}

		interface_init_all(self->ifaces, self->ifaces_count);
	}

	return _self;
}

static Object* object_class_dtor(Object *_self, va_list *ap)
{
	ObjectClass *self = OBJECT_CLASS(_self);
	msg_info("can't destroy class '%s'!", self->name);
	
	return _self;
}

static Object* object_class_cpy(const Object *_self, Object *object)
{
	const ObjectClass *self = OBJECT_CLASS(_self);
	msg_info("can't copy class '%s'!", self->name);
	
	return object;
}

/* }}} */

/* Static Initialization {{{ */

static const ObjectClass __Object;
static const ObjectClass __ObjectClass;

static const ObjectClass __Object = {
	{ MAGIC_NUM, 1, &__ObjectClass },
	"Object", &__Object, sizeof(Object), 0, NULL,
	object_ctor,
	object_dtor,
	object_cpy,
	NULL,
	NULL
};

static const ObjectClass __ObjectClass = {
	{ MAGIC_NUM, 1, &__ObjectClass },
	"ObjectClass", &__Object, sizeof(ObjectClass), 0, NULL,
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
	exit_if_fail(class->size != 0);

	Object *object = (Object*)calloc(1, class->size);

	if (object == NULL)
	{
		msg_error("couldn't allocate memory for object of type '%s'!", class->name);
		return NULL;
	}

	object->magic = MAGIC_NUM;
	object->klass = class;
	object->ref_count = 1;

	va_list ap;
	va_start(ap, object_type);
	object = ctor(object, &ap);
	va_end(ap);

	if (object == NULL)
		msg_error("couldn't create object of type '%s'!", class->name);

	return object;
}

Object* object_new_stack(Type object_type, void *_object, ...)
{
	return_val_if_fail(IS_OBJECT_CLASS(object_type), NULL);
	return_val_if_fail(_object != NULL, NULL);

	const ObjectClass *class = OBJECT_CLASS(object_type);
	exit_if_fail(class->size != 0);

	Object *object = (Object*) _object;

	object->magic = MAGIC_NUM;
	object->klass = class;
	object->ref_count = 1;

	va_list ap;
	va_start(ap, _object);
	object = ctor(object, &ap);
	va_end(ap);

	if (object == NULL)
		msg_error("couldn't create object of type '%s'!", class->name);

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
	exit_if_fail(class->size != 0);

	Object *object = (Object*)calloc(1, class->size);

	if (object == NULL)
	{
		msg_error("couldn't allocate memory for copy of object of type '%s'!",
				class->name);
		return NULL;
	}

	object->magic = MAGIC_NUM;
	object->klass = class;
	object->ref_count = self->ref_count;

	object = cpy(self, object);

	if (object == NULL)
		msg_error("couldn't create copy of object of type '%s'!",
				class->name);

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

const ObjectClass* object_super(const ObjectClass *self)
{
	return_val_if_fail(IS_OBJECT_CLASS(self), NULL);
	exit_if_fail(self->super != NULL);

	return self->super;
}

Object* object_super_ctor(Type object_type, Object *self, va_list *ap)
{
	const ObjectClass *superclass = object_super((const ObjectClass*) object_type);
	
	return_val_if_fail(superclass != NULL, NULL);
	exit_if_fail(superclass->ctor != NULL);

	return superclass->ctor(self, ap);
}

Object* object_super_dtor(Type object_type, Object *self, va_list *ap)
{
	const ObjectClass *superclass = object_super((const ObjectClass*) object_type);
	
	return_val_if_fail(superclass != NULL, NULL);
	exit_if_fail(superclass->dtor != NULL);

	return superclass->dtor(self, ap);
}

Object* object_super_cpy(Type object_type, const Object *self, Object *object)
{
	const ObjectClass *superclass = object_super((const ObjectClass*) object_type);
	
	return_val_if_fail(superclass != NULL, NULL);
	exit_if_fail(superclass->cpy != NULL);

	return superclass->cpy(self, object);
}

/* }}} */

/* vim: set fdm=marker : */
