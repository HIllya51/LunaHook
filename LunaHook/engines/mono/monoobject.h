#ifndef MONOOBJECT_H
#define MONOOBJECT_H

// monoobject.h
// 12/26/2014 jichi
// https://github.com/mono/mono/blob/master/mono/metadata/object.h
// https://github.com/mono/mono/blob/master/mono/metadata/object-internals.h
// https://github.com/mono/mono/blob/master/mono/util/mono-publib.h

#include <cstdint>

#define MONO_ZERO_LEN_ARRAY 1

// mono/io-layer/uglify.h
//typedef int8_t gint8;
//typedef int32_t gint32;
//typedef wchar_t gunichar2; // either char or wchar_t, depending on how mono is compiled

typedef int32_t		mono_bool;
typedef uint8_t		mono_byte;
typedef uint16_t	mono_unichar2;
typedef uint32_t	mono_unichar4;

// mono/metadata/object.h

typedef mono_bool MonoBoolean;

struct MonoArray;
struct MonoDelegate;
struct MonoDomain;
struct MonoException;
struct MonoString;
struct MonoThreadsSync;
struct MonoThread;
struct MonoVTable;

struct MonoObject {
  MonoVTable *vtable;
  MonoThreadsSync *synchronisation;
};

struct MonoString {
  MonoObject object;
  int32_t length;
  mono_unichar2 chars[MONO_ZERO_LEN_ARRAY];
};
#define MONO_TOKEN_TYPE_DEF 0x02000000
#define MONO_TABLE_TYPEDEF 0x2
typedef void (*mono_assembly_foreach_callback_t)(uintptr_t, void*);
typedef void (*mono_assembly_foreach_t)(mono_assembly_foreach_callback_t, uintptr_t);
typedef uintptr_t(*mono_assembly_get_image_t)(uintptr_t);
typedef char* (*mono_image_get_name_t)(uintptr_t);
typedef uintptr_t(*mono_class_from_name_t)(uintptr_t, char*, char*);
typedef uintptr_t(*mono_class_get_property_from_name_t)(uintptr_t, char*);
typedef uintptr_t(*mono_property_get_set_method_t)(uintptr_t);
typedef uint64_t* (*mono_compile_method_t)(uintptr_t);
typedef MonoDomain*(*mono_get_root_domain_t)();
typedef void (*mono_thread_attach_t)(MonoDomain*);
typedef void* (*mono_class_get_t)(void* image, uint32_t type_token);
typedef int (*mono_table_info_get_rows_t)(void*);
typedef void*(*mono_image_get_table_info_t)(void*,uint32_t);
typedef char*(*mono_class_get_name_t)(void*);
typedef void* (*mono_class_get_method_from_name_t)(void *klass, const char *name, int param_count);
#endif // MONOOBJECT_H
