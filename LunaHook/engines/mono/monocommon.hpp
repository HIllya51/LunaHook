
#include"il2cpp.hpp"
#include "main.h"
#include "monoobject.h"
#include"monofuncinfo.h"
namespace {
    
#define GetProcAddressXX(mm, func) auto func=(func##_t)GetProcAddress((mm), #func);
std::mutex mutex;
std::unordered_set<uint64_t>inserted_addr;
void NewHook_check(HookParam& hp,LPCSTR n){
    std::lock_guard _(mutex);
    if(inserted_addr.find(hp.address)==inserted_addr.end()){
        #ifdef _WIN64
        hp.type|=BREAK_POINT;
        #endif
        NewHook(hp,n);
        inserted_addr.insert(hp.address);
    }
} 
std::vector<std::vector<char*>> unity_ui_text={
        {"TMPro","TMP_Text"},
        {"UnityEngine.UI","Text"},
        {"","UILabel"},
        {"UnityEngine","GUIText"},
        {"UnityEngine","TextMesh"},
        {"UnityEngine.UIElements","TextElement"},
        {"UnityEngine.UIElements","TextField","value"}
    }; 
void commonsolvemonostring(uintptr_t offset,uintptr_t *data,  size_t*len){
    MonoString* string = (MonoString*)offset;
    if(string==0)return;
    *data = (uintptr_t)string->chars;
    if(wcslen((wchar_t*)string->chars)!=string->length)return;
    *len = string->length * 2;
} 
void mscorlib_system_string_hook_fun(hook_stack* stack,  HookParam *hp, uintptr_t *data, uintptr_t *split, size_t*len)
{ 
    commonsolvemonostring(stack->ARG1,data,len);
}
void mscorlib_system_string_InternalSubString_hook_fun(hook_stack* stack,  HookParam *hp, uintptr_t *data, uintptr_t *split, size_t*len)
{ 
    uintptr_t offset=stack->ARG1;
    uintptr_t startIndex=stack->ARG2;
    uintptr_t length=stack->ARG3;

    MonoString* string = (MonoString*)offset;
    if(string==0)return;
    *data = (uintptr_t)(startIndex+string->chars);
    if(wcslen((wchar_t*)*data)<length)return;
    *len = length * 2;
    
}
auto mscorlib_system_string_funcs=std::unordered_map<std::string,void*>{
                    {"ToCharArray",mscorlib_system_string_hook_fun},
                    {"Replace",mscorlib_system_string_hook_fun},
                    {"ToString",mscorlib_system_string_hook_fun},
                    {"IndexOf",mscorlib_system_string_hook_fun},
                    {"Substring",mscorlib_system_string_hook_fun},
                    {"op_Inequality",mscorlib_system_string_hook_fun},//[230901] [ILLGAMES] ハニカム
                   {"InternalSubString",mscorlib_system_string_InternalSubString_hook_fun} //[1000-REKA] 早咲きのくろゆり   不这样文本会很多会很卡
                   //https://learn.microsoft.com/zh-cn/dotnet/api/system.string.join?view=net-7.0
                   // "Join",  "IndexOfAnyUnchecked",  "LastIndexOfAny", "Split",  "IndexOfAny", "Compare", "Concat", "TrimEnd", "op_Inequality", "InternalSplitKeepEmptyEntries", "CreateString", "FormatHelper", "FastAllocateString", "EndsWith", "ReplaceInternal",  "CopyTo", "IndexOfUnchecked", "ConcatArray", "Trim", "ToLower", "MakeSeparatorList", ".ctor", "SplitInternal", "CharCopy", "LastIndexOfAnyUnchecked", "IsNullOrWhiteSpace", "CtorCharPtrStartLength", "FillStringChecked", "op_Equality", "StartsWith", "Contains", "ToLowerInvariant", "TrimHelper", "Equals", "wstrcpy", "CtorCharArrayStartLength", "ReplaceUnchecked", "InternalSplitOmitEmptyEntries", "LastIndexOf", "Format"
                    //String还有其他函数，但一般这几个就可以了
                };
void unity_ui_string_hook_fun(hook_stack* stack,  HookParam *hp,  uintptr_t *data, uintptr_t *split, size_t*len)
{ 
    commonsolvemonostring(stack->ARG2,data,len);
}
void unity_ui_string_hook_after(hook_stack* stack,void* data, size_t len)
{ 
    MonoString* string = (MonoString*)stack->ARG2;
    if(string==0)return;
    if(wcslen((wchar_t*)string->chars)!=string->length)return;
    
    //其实也可以直接覆写到原来的String上，但重新创建一个是更安全的操作。
    auto newstring=(MonoString*)malloc(sizeof(MonoString)+len+2);
    memcpy(newstring,string,sizeof(MonoString));
    wcscpy((wchar_t*)newstring->chars,(wchar_t*)data);
    newstring->length=len/2;
    stack->ARG2=(uintptr_t)newstring;
}

void MONO_IL2CPP_NEW_HOOK(void* text_fun,void* hook_after, uintptr_t addr,const char*name){
    if(addr==0)return;
    HookParam hp;
    hp.address = addr;
    hp.type = USING_STRING | CODEC_UTF16|FULL_STRING;
    hp.text_fun =(decltype(hp.text_fun))text_fun;
    
    if(hook_after)
    {
        hp.type|=EMBED_ABLE|EMBED_BEFORE_SIMPLE;
        hp.hook_after=(decltype(hp.hook_after))hook_after;
    }
    NewHook_check(hp, name);
}
auto unity_ui_string_hook=std::bind(MONO_IL2CPP_NEW_HOOK,unity_ui_string_hook_fun,unity_ui_string_hook_after,std::placeholders::_1,std::placeholders::_2);
auto mscorlib_system_string_hook=[](uintptr_t addr,std::pair<std::string,void*>func){
    MONO_IL2CPP_NEW_HOOK(func.second,0,addr,func.first.c_str());
};


/** jichi 12/26/2014 Mono
 *  Sample game: [141226] ハ�レ�めいと
 */
void SpecialHookMonoString(hook_stack* stack,  HookParam *hp, uintptr_t *data, uintptr_t *split, size_t*len)
{
  mscorlib_system_string_hook_fun(stack,hp,data,split,len);

  #ifndef _WIN64
  auto s = stack->ecx;
  for (int i = 0; i < 0x10; i++) // traverse pointers until a non-readable address is met
    if (s && !::IsBadReadPtr((LPCVOID)s, sizeof(DWORD)))
      s = *(DWORD *)s;
    else
      break;
  if (!s)
    s = hp->address;
  if (hp->type & USING_SPLIT) *split = s;
    #endif
   
}
}
namespace il2cpp_func{
    void insertsystemstringfunc(Il2CppClass*klass,HMODULE module){
        GetProcAddressXX(module,il2cpp_class_get_method_from_name); 
        if(il2cpp_class_get_method_from_name==0)return ;
        for(auto func:mscorlib_system_string_funcs){
            auto ToCharArray= il2cpp_class_get_method_from_name(klass,func.first.c_str(),-1);
            if(ToCharArray==0)continue;
            mscorlib_system_string_hook(ToCharArray->methodPointer,func); 
        }

    }
    bool withimage(HMODULE module){
        GetProcAddressXX(module,il2cpp_domain_get); 
        GetProcAddressXX(module,il2cpp_thread_attach); 
        GetProcAddressXX(module,il2cpp_domain_get_assemblies); 
        GetProcAddressXX(module,il2cpp_assembly_get_image); 
        GetProcAddressXX(module,il2cpp_class_from_name); 
        GetProcAddressXX(module,il2cpp_class_get_method_from_name);  

        if (!(il2cpp_domain_get && il2cpp_thread_attach && il2cpp_assembly_get_image && il2cpp_domain_get_assemblies && il2cpp_class_from_name && il2cpp_class_get_method_from_name))return false; 

        auto domain = il2cpp_domain_get();
        il2cpp_thread_attach(domain);
        int _ = 0;
        size_t sz = 0;
        auto assemblies = il2cpp_domain_get_assemblies(domain, &sz);
        for (auto i = 0; i < sz; i++, assemblies++) {
            auto image = il2cpp_assembly_get_image(*assemblies);
            
            do{
                auto cls = il2cpp_class_from_name(image, "System", "String");
                if (cls==0)break;
                il2cpp_func::insertsystemstringfunc(cls,module); 
            }while(0);

            for(auto _:unity_ui_text){ 
                auto cls = il2cpp_class_from_name(image, _[0], _[1]);
                if (cls == 0) continue;
                auto method = il2cpp_class_get_method_from_name(cls, "set_text", 1);
                if (method == 0) continue;
                unity_ui_string_hook(method->methodPointer,_[1]); 
            } 
        }
        return true;
        
    }
    void foreach_func(Il2CppClass* klass, void* userData){
        HMODULE module = GetModuleHandleW(L"GameAssembly.dll");
        
        GetProcAddressXX(module,il2cpp_class_get_name);  
        GetProcAddressXX(module,il2cpp_method_get_name);  
        GetProcAddressXX(module,il2cpp_class_get_methods);  
        GetProcAddressXX(module,il2cpp_class_get_namespace);  
        GetProcAddressXX(module,il2cpp_class_get_method_from_name);  
        
        if (!(il2cpp_class_get_name ))return;
        auto classname = il2cpp_class_get_name(klass);
        std::string cln = classname;
        
        do{
            if(!(il2cpp_class_get_namespace && il2cpp_class_get_method_from_name))break;
            std::string names=il2cpp_class_get_namespace(klass);
            if(cln=="String" && names=="System"){
                il2cpp_func::insertsystemstringfunc(klass,module); 
            }
        }while(0);
         

        do{
            if (!( il2cpp_class_get_methods && il2cpp_method_get_name))break;
            if (!(cln.size() >= 4 && (cln.substr(cln.size() - 4, 4) == "Text" || cln.substr(0, 4) == "Text"))) break;

            void* iter = 0;
            while (true) {
                auto methods = il2cpp_class_get_methods(klass, &iter);
                if (methods == 0)break;
                auto methodname = il2cpp_method_get_name(methods);
                if (std::string(methodname) == "set_text") {
                    unity_ui_string_hook(methods->methodPointer,classname);
                        
                }
            } 
        }while(0);
        
    }
    bool foreach(HMODULE module){
        
        GetProcAddressXX(module,il2cpp_class_for_each);   
        if (il2cpp_class_for_each==0)return false; 
        il2cpp_class_for_each(foreach_func, 0);
        return true;
    }
}
 

namespace{
HMODULE monodllhandle=0;
 
void MonoCallBack(uintptr_t assembly, void* userData) {
    GetProcAddressXX(monodllhandle,mono_get_root_domain); 
    GetProcAddressXX(monodllhandle,mono_thread_attach);  
    GetProcAddressXX(monodllhandle,mono_assembly_get_image); 
    GetProcAddressXX(monodllhandle,mono_class_get_property_from_name); 
    GetProcAddressXX(monodllhandle,mono_class_from_name);  
    GetProcAddressXX(monodllhandle,mono_property_get_set_method); 
    GetProcAddressXX(monodllhandle,mono_compile_method); 
    GetProcAddressXX(monodllhandle,mono_image_get_name); 
    GetProcAddressXX(monodllhandle,mono_image_get_table_info); 
    GetProcAddressXX(monodllhandle,mono_table_info_get_rows); 
    GetProcAddressXX(monodllhandle,mono_class_get); 
    GetProcAddressXX(monodllhandle,mono_class_get_name); 
    GetProcAddressXX(monodllhandle,mono_class_get_method_from_name); 
    if(mono_assembly_get_image==0)return ;
    uintptr_t image = mono_assembly_get_image(assembly); 
    
    if(!(mono_thread_attach&&mono_get_root_domain&&mono_compile_method))return ;

    if((mono_class_from_name&&mono_class_get_property_from_name&&mono_property_get_set_method))
        for(auto _:unity_ui_text){
            auto mono_class=mono_class_from_name(image,_[0],_[1]);
            if(mono_class==0)continue;
            uintptr_t mono_property=mono_class_get_property_from_name(mono_class,_.size()==3?_[2] :"text");
            if (mono_property == 0)continue;  
            auto mono_set_method = mono_property_get_set_method(mono_property); 
            mono_thread_attach(mono_get_root_domain());
            if(mono_set_method==0)continue;
            uint64_t* method_pointer = mono_compile_method(mono_set_method);
            if (method_pointer==0)continue; 
            unity_ui_string_hook((uintptr_t)method_pointer,_[1]);
        
        }
    if((mono_image_get_name&&mono_image_get_table_info&&mono_table_info_get_rows&&mono_class_get&&mono_class_get_name&&mono_class_get_method_from_name)){
        //遍历image中的类，根据类名判断。
        //x86下遍历mono_class_get会中途卡死，故放弃，仅保留预设。
        //这个仅用于hook String
        std::string mscorlib=mono_image_get_name(image);
        if(mscorlib=="mscorlib"){ 
            auto _1=mono_image_get_table_info((void*)image,MONO_TABLE_TYPEDEF);
            auto tdefcount=mono_table_info_get_rows(_1); 
            for (int i = 0; i < tdefcount; i++)
            {		
                void *klass = mono_class_get((void*)image, MONO_TOKEN_TYPE_DEF | i+1);
                char *name=mono_class_get_name(klass);
                std::string cln = name;
                if(cln=="String"){
                    for(auto func:mscorlib_system_string_funcs){
                        auto ToCharArray= mono_class_get_method_from_name(klass,func.first.c_str(),-1);
                        if(ToCharArray==0)continue;
                        auto ToCharArrayAddr=mono_compile_method((uintptr_t)ToCharArray);
                        mscorlib_system_string_hook((uint64_t)ToCharArrayAddr,func); 
                    }
                    
                }
                
            }
        }
    }
    
    
 
}

bool InsertMonoHooksByAssembly(HMODULE module) {
    monodllhandle=module; 
    GetProcAddressXX(monodllhandle,mono_assembly_foreach);  
    if(mono_assembly_foreach==0)return false;
    mono_assembly_foreach(MonoCallBack, NULL);
    return true;
}
} 
namespace {
    bool monodllhook(HMODULE module) {
        HookParam hp;
        const MonoFunction funcs[] = { MONO_FUNCTIONS_INITIALIZER };
        for (auto func : funcs) {
            if (FARPROC addr = GetProcAddress(module, func.functionName)) {
            hp.address =(uintptr_t) addr;
            hp.type =USING_STRING| func.hookType;
            hp.filter_fun =all_ascii_Filter;
            hp.offset = func.textIndex * 4;
            hp.length_offset = func.lengthIndex * 4;
            hp.text_fun = (decltype(hp.text_fun))func.text_fun;
            ConsoleOutput("Mono: INSERT");
            NewHook_check(hp, func.functionName); 
            }
        } 
        return true;
    }
}
 
namespace monocommon{
    bool il2cpp() {

        HMODULE module = GetModuleHandleW(L"GameAssembly.dll");
        if (module == 0)return false;
        bool _1=il2cpp_func::withimage(module); 
        bool _2=il2cpp_func::foreach(module); 
        return _1||_2;
    }

    bool hook_mono(){
        for (const wchar_t* monoName : { L"mono.dll", L"mono-2.0-bdwgc.dll",L"GameAssembly.dll" }) 
            if (HMODULE module = GetModuleHandleW(monoName)) {
                bool b1=InsertMonoHooksByAssembly(module);
                bool b2=monodllhook(module);
                if(b1||b2)return true;
            }
        return false;
    }
}