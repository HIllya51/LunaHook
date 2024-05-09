
#include"monostringapis.h"
namespace {

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
void mscorlib_system_string_hook_fun(hook_stack* stack,  HookParam *hp, uintptr_t *data, uintptr_t *split, size_t*len)
{ 
    commonsolvemonostring(stack->ARG1,data,len);
}

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
            NewHook(hp, func.functionName); 
            }
        } 
        return true;
    }
}

namespace monocommon{
    
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
            NewHook(hp, func.functionName); 
            }
        } 
        return true;
    }
    struct functioninfo{
        const char* assemblyName;const char* namespaze;
						   const char* klassName; const char* name;int argsCount;
                           int argidx;void* text_fun;bool Embed;
        std::string hookname(){
            char tmp[1024];
            sprintf(tmp,"%s:%s",klassName,name);
            return tmp;
        }
        std::string info(){
            char tmp[1024];
            sprintf(tmp,"%s:%s:%s:%s:%d",assemblyName,namespaze,klassName,name,argsCount);
            return tmp;
        }
    }; 
    bool NewHook_check(uintptr_t addr,functioninfo&hook){
        
        HookParam hp;
        hp.address = addr;
        hp.argidx=hook.argidx;
        hp.type = USING_STRING | CODEC_UTF16|FULL_STRING;
        hp.text_fun =(decltype(hp.text_fun))hook.text_fun;
        hp.jittype=JITTYPE::UNITY;
        strcpy(hp.unityfunctioninfo,hook.info().c_str());
        if(hook.Embed)
            hp.type|=EMBED_ABLE|EMBED_BEFORE_SIMPLE;
        auto succ=NewHook(hp,hook.hookname().c_str());
        #ifdef _WIN64
        if(!succ){
            hp.type|=BREAK_POINT;
           succ|=NewHook(hp,hook.hookname().c_str());
        }
        #endif 
        return succ;
    } 
    std::vector<functioninfo>commonhooks{
        {"mscorlib","System","String","ToCharArray",0,1,nullptr,false},
        {"mscorlib","System","String","Replace",2,1,nullptr,false},
        {"mscorlib","System","String","ToString",0,1,nullptr,false},
        {"mscorlib","System","String","IndexOf",1,1,nullptr,false},
        {"mscorlib","System","String","Substring",2,1,nullptr,false},
        {"mscorlib","System","String","op_Inequality",2,1,0,false},
        {"mscorlib","System","String","InternalSubString",2,99999,mscorlib_system_string_InternalSubString_hook_fun,false},
        
        {"Unity.TextMeshPro","TMPro","TMP_Text","set_text",1,2,nullptr,true},
        {"UnityEngine.UI","UnityEngine.UI","Text","set_text",1,2,nullptr,true},
        {"UnityEngine.UIElementsModule","UnityEngine.UIElements","TextElement","set_text",1,2,nullptr,true},
        {"UnityEngine.UIElementsModule","UnityEngine.UIElements","TextField","set_value",1,2,nullptr,true},
        {"UnityEngine.TextRenderingModule","UnityEngine","GUIText","set_text",1,2,nullptr,true},    
        {"UnityEngine.TextRenderingModule","UnityEngine","TextMesh","set_text",1,2,nullptr,true},    
        {"UGUI","","UILabel","set_text",1,2,nullptr,true},
    };
    bool hook_mono_il2cpp(){
        for (const wchar_t* monoName : { L"mono.dll", L"mono-2.0-bdwgc.dll",L"GameAssembly.dll" }) 
            if (HMODULE module = GetModuleHandleW(monoName)) {
                bool b2=monodllhook(module);
                load_mono_functions_from_dll(module);
                il2cpp_symbols::init(module);
                bool succ=false;
                for(auto hook:commonhooks){
                    auto addr=tryfindmonoil2cpp(hook.assemblyName,hook.namespaze,hook.klassName,hook.name,hook.argsCount);
                    if(!addr)continue;
                    succ|=NewHook_check(addr,hook);
                }
                if(succ||b2)return true;
            }
        return false;
    }
}