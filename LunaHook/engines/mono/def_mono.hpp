#pragma once

typedef signed long SInt32;
typedef unsigned long UInt32;
typedef signed short SInt16;
typedef unsigned short UInt16;
typedef unsigned char UInt8;
typedef signed char SInt8;
typedef unsigned long long UInt64;
typedef signed long long SInt64;
#define MONO_ZERO_LEN_ARRAY 1
#define MONO_TOKEN_TYPE_DEF 0x02000000
#define MONO_TABLE_TYPEDEF 0x2
struct MonoException;
struct MonoAssembly;
struct MonoClassField;
struct MonoClass;
struct MonoDomain;
struct MonoImage;
struct MonoType;
struct MonoMethodSignature;
struct MonoArray;
struct MonoThread;
struct MonoThreadsSync;
struct MonoVTable;
struct MonoProperty;
struct MonoReflectionAssembly;
struct MonoReflectionMethod;
struct MonoAppDomain;
struct MonoCustomAttrInfo;

struct MonoReflectionType { UInt32 offset[2]; MonoType* type;};

typedef void* gconstpointer;
typedef void* gpointer;
typedef int gboolean;
typedef unsigned int guint32;
typedef int gint32;
typedef long gint64;
typedef unsigned char   guchar;
typedef UInt16 gunichar2;

struct MonoObject {
  MonoVTable *vtable;
  MonoThreadsSync *synchronisation;
};

typedef MonoObject* MonoStruct;
typedef MonoObject** MonoStruct_out;

struct MonoString {
  MonoObject object;
  gint32 length;
  gunichar2 chars[0];
};

struct MonoMethod {
	UInt16 flags;
	UInt16 iflags;
};

typedef enum
{
	MONO_VERIFIER_MODE_OFF,
	MONO_VERIFIER_MODE_VALID,
	MONO_VERIFIER_MODE_VERIFIABLE,
	MONO_VERIFIER_MODE_STRICT
} MiniVerifierMode;

typedef enum {
	MONO_SECURITY_MODE_NONE,
	MONO_SECURITY_MODE_CORE_CLR,
	MONO_SECURITY_MODE_CAS,
	MONO_SECURITY_MODE_SMCS_HACK
} MonoSecurityMode;

typedef void GFuncRef (void*, void*);
typedef GFuncRef* GFunc;

typedef enum {
	MONO_UNHANDLED_POLICY_LEGACY,
	MONO_UNHANDLED_POLICY_CURRENT
} MonoRuntimeUnhandledExceptionPolicy;

struct MonoMethodDesc {
	char *namespace2;
	char *klass;
	char *name;
	char *args1;
	UInt32 num_args;
	gboolean include_namespace, klass_glob, name_glob;
};


struct MonoJitInfo;
struct MonoAssemblyName;
struct MonoDebugSourceLocation;
struct MonoLoaderError;
struct MonoDlFallbackHandler;
struct LivenessState;

struct MonoBreakPolicy;

typedef bool (*MonoCoreClrPlatformCB) (const char *image_name);

typedef unsigned int guint;
typedef void (*register_object_callback)(gpointer* arr, int size, void* callback_userdata);
typedef gboolean (*MonoStackWalk)     (MonoMethod *method, gint32 native_offset, gint32 il_offset, gboolean managed, gpointer data);
typedef MonoBreakPolicy (*MonoBreakPolicyFunc) (MonoMethod *method);
typedef void* (*MonoDlFallbackLoad) (const char *name, int flags, char **err, void *user_data);
typedef void* (*MonoDlFallbackSymbol) (void *handle, const char *name, char **err, void *user_data);
typedef void* (*MonoDlFallbackClose) (void *handle, void *user_data);

typedef enum {
	MONO_TYPE_NAME_FORMAT_IL,
	MONO_TYPE_NAME_FORMAT_REFLECTION,
	MONO_TYPE_NAME_FORMAT_FULL_NAME,
	MONO_TYPE_NAME_FORMAT_ASSEMBLY_QUALIFIED
} MonoTypeNameFormat;

//typedef void (*vprintf_func)(const char* msg, va_list args);

struct MonoProfiler;
typedef void (*MonoProfileFunc) (MonoProfiler *prof);

typedef enum {
	MONO_PROFILE_NONE = 0,
	MONO_PROFILE_APPDOMAIN_EVENTS = 1 << 0,
	MONO_PROFILE_ASSEMBLY_EVENTS  = 1 << 1,
	MONO_PROFILE_MODULE_EVENTS    = 1 << 2,
	MONO_PROFILE_CLASS_EVENTS     = 1 << 3,
	MONO_PROFILE_JIT_COMPILATION  = 1 << 4,
	MONO_PROFILE_INLINING         = 1 << 5,
	MONO_PROFILE_EXCEPTIONS       = 1 << 6,
	MONO_PROFILE_ALLOCATIONS      = 1 << 7,
	MONO_PROFILE_GC               = 1 << 8,
	MONO_PROFILE_THREADS          = 1 << 9,
	MONO_PROFILE_REMOTING         = 1 << 10,
	MONO_PROFILE_TRANSITIONS      = 1 << 11,
	MONO_PROFILE_ENTER_LEAVE      = 1 << 12,
	MONO_PROFILE_COVERAGE         = 1 << 13,
	MONO_PROFILE_INS_COVERAGE     = 1 << 14,
	MONO_PROFILE_STATISTICAL      = 1 << 15,
	MONO_PROFILE_METHOD_EVENTS    = 1 << 16,
	MONO_PROFILE_MONITOR_EVENTS   = 1 << 17,
	MONO_PROFILE_IOMAP_EVENTS = 1 << 18, /* this should likely be removed, too */
	MONO_PROFILE_GC_MOVES = 1 << 19
} MonoProfileFlags;

typedef enum {
	MONO_GC_EVENT_START,
	MONO_GC_EVENT_MARK_START,
	MONO_GC_EVENT_MARK_END,
	MONO_GC_EVENT_RECLAIM_START,
	MONO_GC_EVENT_RECLAIM_END,
	MONO_GC_EVENT_END,
	MONO_GC_EVENT_PRE_STOP_WORLD,
	MONO_GC_EVENT_POST_STOP_WORLD,
	MONO_GC_EVENT_PRE_START_WORLD,
	MONO_GC_EVENT_POST_START_WORLD
} MonoGCEvent;

typedef void (*MonoProfileMethodFunc)   (MonoProfiler *prof, MonoMethod   *method);
typedef void (*MonoProfileGCFunc)         (MonoProfiler *prof, MonoGCEvent event, int generation);
typedef void (*MonoProfileGCMoveFunc) (MonoProfiler *prof, void **objects, int num);
typedef void (*MonoProfileGCResizeFunc)   (MonoProfiler *prof, gint64 new_size);
typedef void (*MonoProfileAllocFunc)      (MonoProfiler *prof, MonoObject *obj, MonoClass *klass);
typedef void (*MonoProfileJitResult)      (MonoProfiler *prof, MonoMethod   *method,   MonoJitInfo* jinfo,   int result);
typedef void (*MonoProfileExceptionFunc) (MonoProfiler *prof, MonoObject *object);
typedef void (*MonoProfileExceptionClauseFunc) (MonoProfiler *prof, MonoMethod *method, int clause_type, int clause_num);
typedef void (*MonoProfileThreadFunc)     (MonoProfiler *prof, uint32_t tid);


typedef void (*mono_thread_suspend_all_other_threads_t)();
typedef void (*mono_thread_pool_cleanup_t)();
typedef void (*mono_threads_set_shutting_down_t)();
typedef void (*mono_runtime_set_shutting_down_t)();
typedef gboolean (*mono_domain_finalize_t)(MonoDomain *domain, int timeout);
typedef void (*mono_runtime_cleanup_t)(MonoDomain *domain);
typedef MonoMethodDesc *(*mono_method_desc_new_t)(const char *name, gboolean include_namespace);
typedef MonoMethod *(*mono_method_desc_search_in_image_t)(MonoMethodDesc *desc, MonoImage *image);
typedef void (*mono_verifier_set_mode_t)(MiniVerifierMode m);
typedef void (*mono_security_set_mode_t)(MonoSecurityMode m);
typedef void (*mono_add_internal_call_t)(const char *name, gconstpointer method);
typedef void (*mono_jit_cleanup_t)(MonoDomain *domain);
typedef MonoDomain *(*mono_jit_init_t)(const char *file);
typedef MonoDomain *(*mono_jit_init_version_t)(const char *file, const char *runtime_version);
typedef int (*mono_jit_exec_t)(MonoDomain *domain, MonoAssembly *assembly, int argc, char *argv[]);
typedef MonoClass *(*mono_class_from_name_t)(MonoImage *image, const char *name_space, const char *name);
typedef MonoAssembly *(*mono_domain_assembly_open_t)(MonoDomain *domain, const char *name);
typedef MonoDomain *(*mono_domain_create_appdomain_t)(const char *domainname, const char *configfile);
typedef void (*mono_domain_unload_t)(MonoDomain *domain);
typedef MonoObject *(*mono_object_new_t)(MonoDomain *domain, MonoClass *klass);
typedef void (*mono_runtime_object_init_t)(MonoObject *this_obj);
typedef MonoObject *(*mono_runtime_invoke_t)(MonoMethod *method, void *obj, void **params, MonoObject **exc);
typedef void (*mono_field_set_value_t)(MonoObject *obj, MonoClassField *field, void *value);
typedef void (*mono_field_get_value_t)(MonoObject *obj, MonoClassField *field, void *value);
typedef int (*mono_field_get_offset_t)(MonoClassField *field);
typedef MonoClassField *(*mono_class_get_fields_t)(MonoClass *klass, gpointer *iter);
typedef MonoMethod *(*mono_class_get_methods_t)(MonoClass *klass, gpointer *iter);
typedef MonoDomain *(*mono_domain_get_t)();
typedef MonoDomain *(*mono_get_root_domain_t)();
typedef gint32 (*mono_domain_get_id_t)(MonoDomain *domain);
typedef void (*mono_assembly_foreach_t)(GFunc func, gpointer user_data);
typedef void (*mono_image_close_t)(MonoImage *image);
typedef void (*mono_unity_socket_security_enabled_set_t)(gboolean b);
typedef const char *(*mono_image_get_name_t)(MonoImage *image);
typedef MonoClass *(*mono_get_object_class_t)();
typedef void (*mono_set_commandline_arguments_t)(int i, const char *argv[], const char *s);
typedef const char *(*mono_field_get_name_t)(MonoClassField *field);
typedef MonoType *(*mono_field_get_type_t)(MonoClassField *field);
typedef int (*mono_type_get_type_t)(MonoType *type);
typedef const char *(*mono_method_get_name_t)(MonoMethod *method);
typedef MonoImage *(*mono_assembly_get_image_t)(MonoAssembly *assembly);
typedef MonoClass *(*mono_method_get_class_t)(MonoMethod *method);
typedef MonoClass *(*mono_object_get_class_t)(MonoObject *obj);
typedef gboolean (*mono_class_is_valuetype_t)(MonoClass *klass);
typedef guint32 (*mono_signature_get_param_count_t)(MonoMethodSignature *sig);
typedef char *(*mono_string_to_utf8_t)(MonoString *string_obj);
typedef MonoString *(*mono_string_new_wrapper_t)(const char *text);
typedef MonoClass *(*mono_class_get_parent_t)(MonoClass *klass);
typedef const char *(*mono_class_get_namespace_t)(MonoClass *klass);
typedef gboolean (*mono_class_is_subclass_of_t)(MonoClass *klass, MonoClass *klassc, gboolean check_interfaces);
typedef const char *(*mono_class_get_name_t)(MonoClass *klass);
typedef char *(*mono_type_get_name_t)(MonoType *type);
typedef MonoClass *(*mono_type_get_class_t)(MonoType *type);
typedef MonoException *(*mono_exception_from_name_msg_t)(MonoImage *image, const char *name_space, const char *name, const char *msg);
typedef void (*mono_raise_exception_t)(MonoException *ex);
typedef MonoClass *(*mono_get_exception_class_t)();
typedef MonoClass *(*mono_get_array_class_t)();
typedef MonoClass *(*mono_get_string_class_t)();
typedef MonoClass *(*mono_get_int32_class_t)();
typedef MonoArray *(*mono_array_new_t)(MonoDomain *domain, MonoClass *eclass, guint32 n);
typedef MonoArray *(*mono_array_new_full_t)(MonoDomain *domain, MonoClass *array_class, guint32 *lengths, guint32 *lower_bounds);
typedef MonoClass *(*mono_array_class_get_t)(MonoClass *eclass, guint32 rank);
typedef gint32 (*mono_class_array_element_size_t)(MonoClass *ac);
typedef MonoObject *(*mono_type_get_object_t)(MonoDomain *domain, MonoType *type);
typedef MonoThread *(*mono_thread_attach_t)(MonoDomain *domain);
typedef void (*mono_thread_detach_t)(MonoThread *thread);
typedef MonoThread *(*mono_thread_exit_t)();
typedef MonoThread *(*mono_thread_current_t)();
typedef void (*mono_thread_set_main_t)(MonoThread *thread);
typedef void (*mono_set_find_plugin_callback_t)(gconstpointer method);
typedef void (*mono_security_enable_core_clr_t)();
typedef bool (*mono_security_set_core_clr_platform_callback_t)(MonoCoreClrPlatformCB a);
typedef MonoRuntimeUnhandledExceptionPolicy (*mono_runtime_unhandled_exception_policy_get_t)();
typedef void (*mono_runtime_unhandled_exception_policy_set_t)(MonoRuntimeUnhandledExceptionPolicy policy);
typedef MonoClass *(*mono_class_get_nesting_type_t)(MonoClass *klass);
typedef MonoVTable *(*mono_class_vtable_t)(MonoDomain *domain, MonoClass *klass);
typedef MonoReflectionMethod *(*mono_method_get_object_t)(MonoDomain *domain, MonoMethod *method, MonoClass *refclass);
typedef MonoMethodSignature *(*mono_method_signature_t)(MonoMethod *method);
typedef MonoType *(*mono_signature_get_params_t)(MonoMethodSignature *sig, gpointer *iter);
typedef MonoType *(*mono_signature_get_return_type_t)(MonoMethodSignature *sig);
typedef MonoType *(*mono_class_get_type_t)(MonoClass *klass);
typedef void (*mono_set_ignore_version_and_key_when_finding_assemblies_already_loaded_t)(gboolean value);
typedef void (*mono_debug_init_t)(int format);
typedef void (*mono_debug_open_image_from_memory_t)(MonoImage *image, const char *raw_contents, int size);
typedef guint32 (*mono_field_get_flags_t)(MonoClassField *field);
typedef MonoImage *(*mono_image_open_from_data_full_t)(const void *data, guint32 data_len, gboolean need_copy, int *status, gboolean ref_only);
typedef MonoImage *(*mono_image_open_from_data_with_name_t)(char *data, guint32 data_len, gboolean need_copy, int *status, gboolean refonly, const char *name);
typedef MonoAssembly *(*mono_assembly_load_from_t)(MonoImage *image, const char *fname, int *status);
typedef MonoObject *(*mono_value_box_t)(MonoDomain *domain, MonoClass *klass, gpointer val);
typedef MonoImage *(*mono_class_get_image_t)(MonoClass *klass);
typedef char (*mono_signature_is_instance_t)(MonoMethodSignature *signature);
typedef MonoMethod *(*mono_method_get_last_managed_t)();
typedef MonoClass *(*mono_get_enum_class_t)();
typedef MonoType *(*mono_class_get_byref_type_t)(MonoClass *klass);
typedef void (*mono_field_static_get_value_t)(MonoVTable *vt, MonoClassField *field, void *value);
typedef void (*mono_unity_set_embeddinghostname_t)(const char *name);
typedef void (*mono_set_assemblies_path_t)(const char *name);
typedef guint32 (*mono_gchandle_new_t)(MonoObject *obj, gboolean pinned);
typedef MonoObject *(*mono_gchandle_get_target_t)(guint32 gchandle);
typedef guint32 (*mono_gchandle_new_weakref_t)(MonoObject *obj, gboolean track_resurrection);
typedef MonoObject *(*mono_assembly_get_object_t)(MonoDomain *domain, MonoAssembly *assembly);
typedef void (*mono_gchandle_free_t)(guint32 gchandle);
typedef MonoProperty *(*mono_class_get_properties_t)(MonoClass *klass, gpointer *iter);
typedef MonoMethod *(*mono_property_get_get_method_t)(MonoProperty *prop);
typedef MonoObject *(*mono_object_new_alloc_specific_t)(MonoVTable *vtable);
typedef MonoObject *(*mono_object_new_specific_t)(MonoVTable *vtable);
typedef void (*mono_gc_collect_t)(int generation);
typedef int (*mono_gc_max_generation_t)();
typedef MonoAssembly *(*mono_image_get_assembly_t)(MonoImage *image);
typedef MonoAssembly *(*mono_assembly_open_t)(const char *filename, int *status);
typedef gboolean (*mono_class_is_enum_t)(MonoClass *klass);
typedef gint32 (*mono_class_instance_size_t)(MonoClass *klass);
typedef guint32 (*mono_object_get_size_t)(MonoObject *obj);
typedef const char *(*mono_image_get_filename_t)(MonoImage *image);
typedef MonoAssembly *(*mono_assembly_load_from_full_t)(MonoImage *image, const char *fname, int *status, gboolean refonly);
typedef MonoClass *(*mono_class_get_interfaces_t)(MonoClass *klass, gpointer *iter);
typedef void (*mono_assembly_close_t)(MonoAssembly *assembly);
typedef MonoProperty *(*mono_class_get_property_from_name_t)(MonoClass *klass, const char *name);
typedef MonoMethod *(*mono_class_get_method_from_name_t)(MonoClass *klass, const char *name, int param_count);
typedef MonoClass *(*mono_class_from_mono_type_t)(MonoType *image);
typedef gboolean (*mono_domain_set_t)(MonoDomain *domain, gboolean force);
typedef void (*mono_thread_push_appdomain_ref_t)(MonoDomain *domain);
typedef void (*mono_thread_pop_appdomain_ref_t)();
typedef int (*mono_runtime_exec_main_t)(MonoMethod *method, MonoArray *args, MonoObject **exc);
typedef MonoImage *(*mono_get_corlib_t)();
typedef MonoClassField *(*mono_class_get_field_from_name_t)(MonoClass *klass, const char *name);
typedef guint32 (*mono_class_get_flags_t)(MonoClass *klass);
typedef int (*mono_parse_default_optimizations_t)(const char *p);
typedef void (*mono_set_defaults_t)(int verbose_level, guint32 opts);
typedef void (*mono_set_dirs_t)(const char *assembly_dir, const char *config_dir);
typedef void (*mono_jit_parse_options_t)(int argc, char *argv[]);
typedef gpointer (*mono_object_unbox_t)(MonoObject *o);
typedef MonoObject *(*mono_custom_attrs_get_attr_t)(MonoCustomAttrInfo *ainfo, MonoClass *attr_klass);
typedef gboolean (*mono_custom_attrs_has_attr_t)(MonoCustomAttrInfo *ainfo, MonoClass *attr_klass);
typedef MonoCustomAttrInfo *(*mono_custom_attrs_from_field_t)(MonoClass *klass, MonoClassField *field);
typedef MonoCustomAttrInfo *(*mono_custom_attrs_from_method_t)(MonoMethod *method);
typedef MonoCustomAttrInfo *(*mono_custom_attrs_from_class_t)(MonoClass *klass);
typedef void (*mono_custom_attrs_free_t)(MonoCustomAttrInfo *attr);
typedef void (*g_free_t)(void *p);
typedef gboolean (*mono_runtime_is_shutting_down_t)();
typedef MonoMethod *(*mono_object_get_virtual_method_t)(MonoObject *obj, MonoMethod *method);
typedef gpointer (*mono_jit_info_get_code_start_t)(MonoJitInfo *ji);
typedef int (*mono_jit_info_get_code_size_t)(MonoJitInfo *ji);
typedef MonoClass *(*mono_class_from_name_case_t)(MonoImage *image, const char *name_space, const char *name);
typedef MonoClass *(*mono_class_get_nested_types_t)(MonoClass *klass, gpointer *iter);
typedef int (*mono_class_get_userdata_offset_t)();
typedef void *(*mono_class_get_userdata_t)(MonoClass *klass);
typedef void (*mono_class_set_userdata_t)(MonoClass *klass, void *userdata);
typedef void (*mono_set_signal_chaining_t)(gboolean chain_signals);
// typedef LONG( *mono_unity_seh_handler_t)(EXCEPTION_POINTERS* ep);
typedef void (*mono_unity_set_unhandled_exception_handler_t)(void *handler);
typedef MonoObject *(*mono_runtime_invoke_array_t)(MonoMethod *method, void *obj, MonoArray *params, MonoObject **exc);
typedef char *(*mono_array_addr_with_size_t)(MonoArray *array, int size, uintptr_t idx);
typedef gunichar2 *(*mono_string_to_utf16_t)(MonoString *string_obj);
typedef MonoClass *(*mono_field_get_parent_t)(MonoClassField *field);
typedef char *(*mono_method_full_name_t)(MonoMethod *method, gboolean signature);
typedef MonoObject *(*mono_object_isinst_t)(MonoObject *obj, MonoClass *klass);
typedef MonoString *(*mono_string_new_len_t)(MonoDomain *domain, const char *text, guint length);
typedef MonoString *(*mono_string_from_utf16_t)(gunichar2 *data);
typedef MonoException *(*mono_get_exception_argument_null_t)(const char *arg);
typedef MonoClass *(*mono_get_boolean_class_t)();
typedef MonoClass *(*mono_get_byte_class_t)();
typedef MonoClass *(*mono_get_char_class_t)();
typedef MonoClass *(*mono_get_int16_class_t)();
typedef MonoClass *(*mono_get_int64_class_t)();
typedef MonoClass *(*mono_get_single_class_t)();
typedef MonoClass *(*mono_get_double_class_t)();
typedef gboolean (*mono_class_is_generic_t)(MonoClass *klass);
typedef gboolean (*mono_class_is_inflated_t)(MonoClass *klass);
typedef gboolean (*unity_mono_method_is_generic_t)(MonoMethod *method);
typedef gboolean (*unity_mono_method_is_inflated_t)(MonoMethod *method);
typedef gboolean (*mono_is_debugger_attached_t)();
typedef gboolean (*mono_assembly_fill_assembly_name_t)(MonoImage *image, MonoAssemblyName *aname);
typedef char *(*mono_stringify_assembly_name_t)(MonoAssemblyName *aname);
typedef gboolean (*mono_assembly_name_parse_t)(const char *name, MonoAssemblyName *aname);
typedef MonoAssembly *(*mono_assembly_loaded_t)(MonoAssemblyName *aname);
typedef int (*mono_image_get_table_rows_t)(MonoImage *image, int table_id);
typedef MonoClass *(*mono_class_get_t)(MonoImage *image, guint32 type_token);
typedef gboolean (*mono_metadata_signature_equal_t)(MonoMethodSignature *sig1, MonoMethodSignature *sig2);
typedef gboolean (*mono_gchandle_is_in_domain_t)(guint32 gchandle, MonoDomain *domain);
typedef void (*mono_stack_walk_t)(MonoStackWalk func, gpointer user_data);
typedef char *(*mono_pmip_t)(void *ip);
typedef MonoObject *(*mono_runtime_delegate_invoke_t)(MonoObject *delegate, void **params, MonoObject **exc);
typedef MonoJitInfo *(*mono_jit_info_table_find_t)(MonoDomain *domain, char *addr);
typedef MonoDebugSourceLocation *(*mono_debug_lookup_source_location_t)(MonoMethod *method, guint32 address, MonoDomain *domain);
typedef void (*mono_debug_free_source_location_t)(MonoDebugSourceLocation *location);
typedef void (*mono_gc_wbarrier_generic_store_t)(gpointer ptr, MonoObject *value);
typedef MonoType *(*mono_class_enum_basetype_t)(MonoClass *klass);
typedef guint32 (*mono_class_get_type_token_t)(MonoClass *klass);
typedef int (*mono_class_get_rank_t)(MonoClass *klass);
typedef MonoClass *(*mono_class_get_element_class_t)(MonoClass *klass);
typedef gboolean (*mono_unity_class_is_interface_t)(MonoClass *klass);
typedef gboolean (*mono_unity_class_is_abstract_t)(MonoClass *klass);
typedef gint32 (*mono_array_element_size_t)(MonoClass *ac);
typedef void (*mono_config_parse_t)(const char *filename);
typedef void (*mono_set_break_policy_t)(MonoBreakPolicyFunc policy_callback);
typedef MonoArray *(*mono_custom_attrs_construct_t)(MonoCustomAttrInfo *cinfo);
typedef MonoCustomAttrInfo *(*mono_custom_attrs_from_assembly_t)(MonoAssembly *assembly);
typedef MonoArray *(*mono_reflection_get_custom_attrs_by_type_t)(MonoObject *obj, MonoClass *attr_klass);
typedef MonoLoaderError *(*mono_loader_get_last_error_t)();
typedef MonoException *(*mono_loader_error_prepare_exception_t)(MonoLoaderError *error);
typedef MonoDlFallbackHandler *(*mono_dl_fallback_register_t)(MonoDlFallbackLoad load_func, MonoDlFallbackSymbol symbol_func, MonoDlFallbackClose close_func, void *user_data);
typedef void (*mono_dl_fallback_unregister_t)(MonoDlFallbackHandler *handler);
typedef LivenessState *(*mono_unity_liveness_allocate_struct_t)(MonoClass *filter, guint max_count, register_object_callback callback, void *callback_userdata);
typedef void (*mono_unity_liveness_stop_gc_world_t)();
typedef void (*mono_unity_liveness_finalize_t)(LivenessState *state);
typedef void (*mono_unity_liveness_start_gc_world_t)();
typedef void (*mono_unity_liveness_free_struct_t)(LivenessState *state);
typedef LivenessState *(*mono_unity_liveness_calculation_begin_t)(MonoClass *filter, guint max_count, register_object_callback callback, void *callback_userdata);
typedef void (*mono_unity_liveness_calculation_end_t)(LivenessState *state);
typedef void (*mono_unity_liveness_calculation_from_root_t)(MonoObject *root, LivenessState *state);
typedef void (*mono_unity_liveness_calculation_from_statics_t)(LivenessState *state);
typedef void (*mono_trace_set_level_string_t)(const char *value);
typedef void (*mono_trace_set_mask_string_t)(char *value);
typedef gint64 (*mono_gc_get_used_size_t)();
typedef gint64 (*mono_gc_get_heap_size_t)();
typedef MonoMethod *(*mono_method_desc_search_in_class_t)(MonoMethodDesc *desc, MonoClass *klass);
typedef void (*mono_method_desc_free_t)(MonoMethodDesc *desc);
typedef char *(*mono_type_get_name_full_t)(MonoType *type, MonoTypeNameFormat format);
typedef void (*mono_unity_thread_clear_domain_fields_t)();
// typedef void( *mono_unity_set_vprintf_func_t)(vprintf_func func);
typedef void (*mono_profiler_install_t)(MonoProfiler *prof, MonoProfileFunc shutdown_callback);
typedef void (*mono_profiler_set_events_t)(MonoProfileFlags events);
typedef void (*mono_profiler_install_enter_leave_t)(MonoProfileMethodFunc enter, MonoProfileMethodFunc fleave);
typedef void (*mono_profiler_install_gc_t)(MonoProfileGCFunc callback, MonoProfileGCResizeFunc heap_resize_callback);
typedef void (*mono_profiler_install_allocation_t)(MonoProfileAllocFunc callback);
typedef void (*mono_profiler_install_jit_end_t)(MonoProfileJitResult end);
typedef void (*mono_profiler_install_exception_t)(MonoProfileExceptionFunc throw_callback, MonoProfileMethodFunc exc_method_leave, MonoProfileExceptionClauseFunc clause_callback);
typedef void (*mono_profiler_install_thread_t)(MonoProfileThreadFunc start, MonoProfileThreadFunc end);
typedef uint64_t* (*mono_compile_method_t)(uintptr_t);

typedef void*(*mono_image_get_table_info_t)(void*,uint32_t);
typedef int (*mono_table_info_get_rows_t)(void*);
