#ifndef INTERFACE_H_DTU16BXB
#define INTERFACE_H_DTU16BXB

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#include "Definitions.h"

typedef struct _Interface Interface;

struct _Interface
{
	void *private[5];
};

#define USE_INTERFACE(type, init) (type), (init)

#define DEFINE_INTERFACE(TN, t_n) 																	\
	static Type __##TN##Interface; 																	\
	Type t_n##_interface_get_type(void) 															\
	{ 																								\
		if (!__##TN##Interface) 																	\
		{ 																							\
			__##TN##Interface = interface_type_new(TOSTR(TN##Interface), sizeof(TN##Interface), 0); \
		} 																							\
																									\
		return __##TN##Interface; 																	\
	}

#define DEFINE_INTERFACE_WITH_IFACES(TN, t_n, IFC, IFS...) 													\
	static Type __##TN##Interface; 																			\
	Type t_n##_interface_get_type(void) 																	\
	{ 																										\
		if (!__##TN##Interface) 																			\
		{ 																									\
			__##TN##Interface = interface_type_new(TOSTR(TN##Interface), sizeof(TN##Interface), IFC, IFS); 	\
		} 																									\
																											\
		return __##TN##Interface; 																			\
	}

#define DECLARE_INTERFACE(ModuleObjName, module_obj_name, MODULE_OBJ_NAME)												\
	typedef struct _##ModuleObjName##Interface ModuleObjName##Interface;												\
	typedef struct { Object* parent; } ModuleObjName;																	\
	Type module_obj_name##_interface_get_type(void);																	\
	inline ModuleObjName##Interface* MODULE_OBJ_NAME##_INTERFACE(const ModuleObjName *self) {							\
		return (ModuleObjName##Interface*)interface_cast(module_obj_name##_interface_get_type(), (const void*) self); } \
	inline bool IS_##MODULE_OBJ_NAME(const ModuleObjName *self) { 														\
		return hasInterface(module_obj_name##_interface_get_type(), (const void*) self); }
	
Type isInterfaceType(Type itype);
const Interface* isInterface(const void *_iface);
bool hasInterface(Type itype, const void *self);
Interface* interface_find(Type itype, Interface **ifaces, size_t ifaces_count);
void* interface_cast(Type itype, const void *self);
void interface_init_all(Interface **ifaces, size_t ifaces_count);
Interface* interface_copy(Interface *iface);
Type interface_type_new(char *name, size_t size, size_t itypes_count, ...);
Interface* interface_new(Type interface_type, void (*init)(Interface *iface));

#endif /* end of include guard: INTERFACE_H_DTU16BXB */
