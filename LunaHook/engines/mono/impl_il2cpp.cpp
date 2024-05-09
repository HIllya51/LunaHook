
il2cpp_string_new_utf16_t il2cpp_string_new_utf16;
il2cpp_string_new_t il2cpp_string_new;
il2cpp_domain_get_t il2cpp_domain_get;
il2cpp_domain_assembly_open_t il2cpp_domain_assembly_open;
il2cpp_assembly_get_image_t il2cpp_assembly_get_image;
il2cpp_class_from_name_t il2cpp_class_from_name;
il2cpp_class_get_methods_t il2cpp_class_get_methods;
il2cpp_class_get_method_from_name_t il2cpp_class_get_method_from_name;
il2cpp_method_get_param_t il2cpp_method_get_param;
il2cpp_object_new_t il2cpp_object_new;
il2cpp_resolve_icall_t il2cpp_resolve_icall;
il2cpp_array_new_t il2cpp_array_new;
il2cpp_thread_attach_t il2cpp_thread_attach;
il2cpp_thread_detach_t il2cpp_thread_detach;
il2cpp_class_get_field_from_name_t il2cpp_class_get_field_from_name;
il2cpp_class_is_assignable_from_t il2cpp_class_is_assignable_from;
il2cpp_class_for_each_t il2cpp_class_for_each;
il2cpp_class_get_nested_types_t il2cpp_class_get_nested_types;
il2cpp_class_get_type_t il2cpp_class_get_type;
il2cpp_type_get_object_t il2cpp_type_get_object;
il2cpp_gchandle_new_t il2cpp_gchandle_new;
il2cpp_gchandle_free_t il2cpp_gchandle_free;
il2cpp_gchandle_get_target_t il2cpp_gchandle_get_target;
il2cpp_class_from_type_t il2cpp_class_from_type;
il2cpp_runtime_class_init_t il2cpp_runtime_class_init;
il2cpp_runtime_invoke_t il2cpp_runtime_invoke;
il2cpp_class_get_name_t il2cpp_class_get_name;
il2cpp_class_get_namespace_t il2cpp_class_get_namespace;
il2cpp_domain_get_assemblies_t il2cpp_domain_get_assemblies;
il2cpp_class_get_image_t il2cpp_class_get_image;
il2cpp_image_get_name_t il2cpp_image_get_name;
namespace il2cpp_symbols
{
#define RESOLVE_IMPORT(name) name = reinterpret_cast<name##_t>(GetProcAddress(game_module, #name))

	void init(HMODULE game_module)
	{
		RESOLVE_IMPORT(il2cpp_image_get_name);
		RESOLVE_IMPORT(il2cpp_class_get_image);
		RESOLVE_IMPORT(il2cpp_string_new_utf16);
		RESOLVE_IMPORT(il2cpp_string_new);
		RESOLVE_IMPORT(il2cpp_domain_get);
		RESOLVE_IMPORT(il2cpp_domain_assembly_open);
		RESOLVE_IMPORT(il2cpp_assembly_get_image);
		RESOLVE_IMPORT(il2cpp_class_from_name);
		RESOLVE_IMPORT(il2cpp_class_get_methods);
		RESOLVE_IMPORT(il2cpp_class_get_method_from_name);
		RESOLVE_IMPORT(il2cpp_method_get_param);
		RESOLVE_IMPORT(il2cpp_object_new);
		RESOLVE_IMPORT(il2cpp_resolve_icall);
		RESOLVE_IMPORT(il2cpp_array_new);
		RESOLVE_IMPORT(il2cpp_thread_attach);
		RESOLVE_IMPORT(il2cpp_thread_detach);
		RESOLVE_IMPORT(il2cpp_class_get_field_from_name);
		RESOLVE_IMPORT(il2cpp_class_is_assignable_from);
		RESOLVE_IMPORT(il2cpp_class_for_each);
		RESOLVE_IMPORT(il2cpp_class_get_nested_types);
		RESOLVE_IMPORT(il2cpp_class_get_type);
		RESOLVE_IMPORT(il2cpp_type_get_object);
		RESOLVE_IMPORT(il2cpp_gchandle_new);
		RESOLVE_IMPORT(il2cpp_gchandle_free);
		RESOLVE_IMPORT(il2cpp_gchandle_get_target);
		RESOLVE_IMPORT(il2cpp_class_from_type);
		RESOLVE_IMPORT(il2cpp_runtime_class_init);
		RESOLVE_IMPORT(il2cpp_runtime_invoke);
		RESOLVE_IMPORT(il2cpp_class_get_name);
		RESOLVE_IMPORT(il2cpp_class_get_namespace);
		RESOLVE_IMPORT(il2cpp_domain_get_assemblies);
	}
	Il2CppClass* get_il2cppclass1(const char* assemblyName, const char* namespaze,
								 const char* klassName)
	{
		if(!(il2cpp_assembly_get_image&&il2cpp_class_from_name))return NULL;
		void* assembly=0;
		if(il2cpp_domain_assembly_open){
		do{
			assembly = il2cpp_domain_assembly_open(il2cpp_domain, assemblyName);
			if(!assembly)break; 
			auto image = il2cpp_assembly_get_image(assembly);
			if(!image)break;
			auto klass = il2cpp_class_from_name(image, namespaze, klassName);
			if(klass)return klass;
		}while(0);
		}
		if(il2cpp_domain_get_assemblies&&il2cpp_assembly_get_image)
		{
			int _ = 0;
			size_t sz = 0;
			auto assemblies = il2cpp_domain_get_assemblies(il2cpp_domain, &sz);
			for (auto i = 0; i < sz; i++, assemblies++) {
				auto image = il2cpp_assembly_get_image(*assemblies);
				auto cls = il2cpp_class_from_name(image, namespaze, klassName);
				if(cls)return cls;
			}
		}
		return NULL;
		 
	}
	void foreach_func(Il2CppClass* klass, void* userData){
		auto st=(std::vector<Il2CppClass*>*)userData;
		st->push_back(klass);
		
	}
	std::vector<Il2CppClass*> get_il2cppclass2(const char* namespaze,
								 const char* klassName)
	{
		if(!(il2cpp_class_for_each&&il2cpp_class_get_name))return {};
		std::vector<Il2CppClass*>maybes;
		std::vector<Il2CppClass*>klasses;
		il2cpp_class_for_each(foreach_func,&klasses);
		
		for(auto klass:klasses){
			auto classname = il2cpp_class_get_name(klass);
			if(strcmp(classname,klassName)!=0)continue;
			maybes.push_back(klass);
			if(il2cpp_class_get_namespace){
				auto namespacename=il2cpp_class_get_namespace(klass);
				if(strlen(namespaze)&&(strcmp(namespacename,namespaze)==0)){
					return {klass};
				}
			}
		}
		return maybes;
	}
	struct AutoThread{
		void*thread=NULL;
		AutoThread(){
			if(!(il2cpp_thread_attach&&il2cpp_domain_get))return;
			auto il2cpp_domain=il2cpp_domain_get();
			if (!il2cpp_domain) return;
			thread= il2cpp_thread_attach(il2cpp_domain);
		}
		~AutoThread(){
			if(!thread)return;
			if(!il2cpp_thread_detach)return;
			il2cpp_thread_detach(thread);
		}
	};
	void tryprintimage(Il2CppClass* klass){
		if(!(il2cpp_class_get_namespace&&il2cpp_class_get_image&&il2cpp_image_get_name))return;
		auto image=il2cpp_class_get_image(klass);
		if(!image)return;
		ConsoleOutput("%s:%s",il2cpp_image_get_name(image),il2cpp_class_get_namespace(klass));
	}
	uintptr_t getmethodofklass(Il2CppClass* klass,const char* name, int argsCount){
		if(!(il2cpp_class_get_method_from_name))return NULL;
		if(!klass)return NULL;
		auto ret = il2cpp_class_get_method_from_name(klass, name, argsCount);
		if(!ret)return NULL;
		tryprintimage(klass);
		return ret->methodPointer;
	}
	uintptr_t get_method_pointer(const char* assemblyName, const char* namespaze,
								 const char* klassName, const char* name, int argsCount)
	{
		auto thread=AutoThread();
		if(!thread.thread)return NULL;

		auto klass=get_il2cppclass1(assemblyName,namespaze,klassName);//正向查询，assemblyName可以为空
		if(klass)
			return getmethodofklass(klass,name,argsCount);
		auto klasses=get_il2cppclass2(namespaze,klassName);//反向查询，namespace可以为空
		for(auto klass:klasses){
			auto method= getmethodofklass(klass,name,argsCount);
			if(method)return method;
		}
		return NULL;
	}
}
