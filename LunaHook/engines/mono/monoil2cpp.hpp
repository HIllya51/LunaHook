#include"def_mono.hpp"
#include"def_il2cpp.hpp"

inline void commonsolvemonostring(uintptr_t offset,uintptr_t *data,  size_t*len){
    MonoString* string = (MonoString*)offset;
    if(string==0)return;
    *data = (uintptr_t)string->chars;
    if(wcslen((wchar_t*)string->chars)!=string->length)return;
    *len = string->length * 2;
} 


inline void unity_ui_string_hook_after(uintptr_t *offset,void* data, size_t len)
{ 
    MonoString* string = (MonoString*)*offset;
    if(string==0)return;
    if(wcslen((wchar_t*)string->chars)!=string->length)return;
    
    //其实也可以直接覆写到原来的String上，但重新创建一个是更安全的操作。
    auto newstring=(MonoString*)malloc(sizeof(MonoString)+len+2);
    memcpy(newstring,string,sizeof(MonoString));
    wcscpy((wchar_t*)newstring->chars,(wchar_t*)data);
    newstring->length=len/2;
    *offset=(uintptr_t)newstring;
}
namespace il2cpp_symbols
{
	void init(HMODULE game_module);
	uintptr_t get_method_pointer(const char* assemblyName, const char* namespaze,
								 const char* klassName, const char* name, int argsCount);

}
void load_mono_functions_from_dll(HMODULE dll);

uintptr_t getmonofunctionptr(const char *_dll, const char *_namespace, const char *_class, const char *_method, int paramCount);


inline uintptr_t tryfindmonoil2cpp(const char *_dll, const char *_namespace, const char *_class, const char *_method, int paramCoun){
    auto addr=il2cpp_symbols::get_method_pointer(_dll,_namespace,_class,_method,paramCoun);
    if(addr)return addr;
    return getmonofunctionptr(_dll,_namespace,_class,_method,paramCoun);
}