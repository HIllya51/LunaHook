#include"python.h"
#include"embed_util.h"
#include"main.h"
#include"stackoffset.hpp"
#include"types.h"
#include"defs.h"
namespace{

typedef enum {PyGILState_LOCKED, PyGILState_UNLOCKED}
    PyGILState_STATE;
typedef PyGILState_STATE (*PyGILState_Ensure_t)(void);
typedef void (*PyGILState_Release_t)(PyGILState_STATE);
typedef int (*PyRun_SimpleString_t)(const char *command);

PyRun_SimpleString_t PyRun_SimpleString;
PyGILState_Release_t PyGILState_Release;
PyGILState_Ensure_t PyGILState_Ensure;
    
void LoadPyRun(HMODULE module)
{
    PyGILState_Ensure=(PyGILState_Ensure_t)GetProcAddress(module, "PyGILState_Ensure");
    PyGILState_Release=(PyGILState_Release_t)GetProcAddress(module, "PyGILState_Release");
    PyRun_SimpleString=(PyRun_SimpleString_t)GetProcAddress(module, "PyRun_SimpleString");
}

void PyRunScript(const char* script)
{
    if(!(PyGILState_Ensure&&PyGILState_Release&&PyRun_SimpleString))return;
    
    auto state=PyGILState_Ensure();
    PyRun_SimpleString(script);
    PyGILState_Release(state);
}
    
void hook_internal_renpy_call_host(){
    HookParam hp_internal;
    hp_internal.address=(uintptr_t)internal_renpy_call_host;
    #ifndef _WIN64
    hp_internal.offset=get_stack(1);
    hp_internal.split=get_stack(2);
    #else
    hp_internal.offset=get_reg(regs::rcx);
    hp_internal.split=get_reg(regs::rdx);
    #endif
    hp_internal.type=USING_SPLIT|USING_STRING|CODEC_UTF16|EMBED_ABLE|EMBED_BEFORE_SIMPLE|EMBED_AFTER_NEW;
    NewHook(hp_internal, "internal_renpy_call_host");
    PyRunScript(LoadResData(L"renpy_hook_text",L"PYSOURCE").c_str());
}
    
void patchfontfunction(){
    //由于不知道怎么从字体名映射到ttc/ttf文件名，所以暂时写死arial/msyh
    if(wcslen(embedsharedmem->fontFamily)==0)return;
    PyRunScript(LoadResData(L"renpy_hook_font",L"PYSOURCE").c_str());
}
}
const wchar_t* internal_renpy_call_host(const wchar_t* text,int split){
    return text;
}
void hookrenpy(HMODULE module){
    LoadPyRun(module);
    patch_fun=patchfontfunction;
    hook_internal_renpy_call_host();
}