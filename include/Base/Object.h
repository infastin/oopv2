#ifndef OBJECT_H_IEJNPLAH
#define OBJECT_H_IEJNPLAH

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#include "Interface.h"
#include "Definitions.h"
#include "Macros.h"

#define MAGIC_NUM 0xAb0bA

#define DEFINE_TYPE(TN, t_n, t_p)                                        \
	static Type __##TN##Class;                                           \
	Type t_n##_class_get_type(void)                                      \
	{                                                                    \
		if (!__##TN##Class)                                              \
		{                                                                \
			__##TN##Class = (Type) object_new(t_p##_class_get_type(),    \
					TOSTR(TN##Class),                                    \
					t_p##_class_get_type(),                              \
					sizeof(TN##Class),                                   \
					NULL,                                                \
					0);                                                  \
		}                                                                \
																		 \
		return __##TN##Class;                                            \
	}                                                                    \
	static void t_n##_class_init(TN##Class *klass);                      \
	static Type __##TN;                                                  \
	Type t_n##_get_type(void)                                            \
	{                                                                    \
		if (!__##TN)                                                     \
		{                                                                \
			__##TN = (Type) object_new((t_n##_class_get_type()),         \
					#TN,                                                 \
					t_p##_get_type(),                                    \
					sizeof(TN),                                          \
					t_n##_class_init,                                    \
					0);                                                  \
		}                                                                \
																		 \
		return __##TN;                                                   \
	}

#define DEFINE_TYPE_WITH_IFACES(TN, t_n, t_p, IFC, IFS...)               \
	static Type __##TN##Class;                                           \
	Type t_n##_class_get_type(void)                                      \
	{                                                                    \
		if (!__##TN##Class)                                              \
		{                                                                \
			__##TN##Class = (Type) object_new(t_p##_class_get_type(),    \
					TOSTR(TN##Class),                                    \
					t_p##_class_get_type(),                              \
					sizeof(TN##Class),                                   \
					NULL,                                                \
					0);                                                  \
		}                                                                \
																		 \
		return __##TN##Class;                                            \
	}                                                                    \
	static void t_n##_class_init(TN##Class *klass);                      \
	static Type __##TN;                                                  \
	Type t_n##_get_type(void)                                            \
	{                                                                    \
		if (!__##TN)                                                     \
		{                                                                \
			__##TN = (Type) object_new((t_n##_class_get_type()),         \
					#TN,                                                 \
					t_p##_get_type(),                                    \
					sizeof(TN),                                          \
					t_n##_class_init,                                    \
					IFC, IFS,                                            \
					0);                                                  \
		}                                                                \
																		 \
		return __##TN;                                                   \
	}


#define DECLARE_TYPE_BODY(ModuleObjName, module_obj_name, MODULE_OBJ_NAME)                 \
	Type module_obj_name##_get_type(void);                                                 \
	Type module_obj_name##_class_get_type(void);                                           \
	GNUC_UNUSED static inline ModuleObjName* MODULE_OBJ_NAME(const Object *self) {                            \
		return (ModuleObjName*)cast(module_obj_name##_get_type(), self); }                 \
	GNUC_UNUSED static inline ModuleObjName##Class* MODULE_OBJ_NAME##_CLASS(const ObjectClass *klass) {       \
		return (ModuleObjName##Class*)cast((module_obj_name##_class_get_type()), klass); } \
	GNUC_UNUSED static inline ModuleObjName##Class* MODULE_OBJ_NAME##_GET_CLASS(const ModuleObjName *self) {  \
		return (ModuleObjName##Class*)classOf(self); }                                     \
	GNUC_UNUSED static inline bool IS_##MODULE_OBJ_NAME(const ModuleObjName *self) {                          \
		return isOf(self, module_obj_name##_get_type()); }                                 \
	GNUC_UNUSED static inline bool IS_##MODULE_OBJ_NAME##_CLASS(const ModuleObjName##Class *klass) {          \
		return isOf(klass, module_obj_name##_class_get_type()); }

#define DECLARE_TYPE(ModuleObjName, module_obj_name, MODULE_OBJ_NAME, ParentName) \
	typedef struct _##ModuleObjName ModuleObjName;                                \
	typedef struct { ParentName##Class parent; } ModuleObjName##Class;            \
	DECLARE_TYPE_BODY(ModuleObjName, module_obj_name, MODULE_OBJ_NAME)

#define DECLARE_DERIVABLE_TYPE(ModuleObjName, module_obj_name, MODULE_OBJ_NAME, ParentName) \
	typedef struct _##ModuleObjName ModuleObjName; 											\
	typedef struct _##ModuleObjName##Class ModuleObjName##Class; 							\
	DECLARE_TYPE_BODY(ModuleObjName, module_obj_name, MODULE_OBJ_NAME)

typedef struct _Object Object;
typedef struct _ObjectClass ObjectClass;

struct _Object
{
	unsigned int magic;
	const ObjectClass *klass;
};

struct _ObjectClass
{
	const Object _;
	const char *name;
	const ObjectClass *super;
	size_t size;
	size_t ifaces_count;
	Interface **ifaces;

	Object* (*ctor)(Object *self, va_list *ap);
	Object* (*dtor)(Object *self, va_list *ap);
	Object* (*cpy)(const Object *self, Object *object);

	Object* (*set)(Object *self, va_list *ap);
	void  	(*get)(const Object *self, va_list *ap);
};

#define OBJECT_TYPE (object_get_type())
#define OBJECT_CLASS_TYPE (object_class_get_type())

#define OBJECT(self) ((Object*)(cast(object_get_type(), (const void*) self)))
#define OBJECT_CLASS(klass) ((ObjectClass*)(cast(object_class_get_type(), (const void*) klass)))

#define IS_OBJECT(self) (isOf((const void*) self, object_get_type()))
#define IS_OBJECT_CLASS(klass) (isOf((const void*) klass, object_class_get_type()))

#define OBJECT_GET_CLASS(self) ((ObjectClass*)(classOf((const void*) self)))
#define OBJECT_SIZE(self) (sizeOf((const void*) self))

Type object_get_type(void);
Type object_class_get_type(void);

const void* isObject(const void *self);
const void* classOf(const void *self);

void*  cast(Type object_type, const void *self);
size_t sizeOf(const void *self);
bool   isOf(const void *self, Type object_type);

Object* object_new(Type object_type, ...);
Object* object_new_stack(Type object_type, void *object, ...);
void    object_delete(Object *self, ...);
Object* object_copy(const Object *self);

Object* object_set(Object *self, ...);
void object_get(const Object *self, ...);

const ObjectClass* object_super(const ObjectClass *self);
	  Object* 	   object_super_ctor(Type object_type, Object *self, va_list *ap);
	  Object* 	   object_super_dtor(Type object_type, Object *self, va_list *ap);
	  Object* 	   object_super_cpy(Type object_type, const Object *self, Object *object);

#endif /* end of include guard: OBJECT_H_IEJNPLAH */
