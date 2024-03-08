#include"python.h"
#include"embed_util.h"
#include"main.h"
#include"stackoffset.hpp"
#include"types.h"
#include"defs.h"
const wchar_t* internal_renpy_call_host(const wchar_t* text,int split){
    return text;
}
void hookrenpy(HMODULE module){
    LoadPyRun(module);
    patch_fun=patchfontfunction;
    hook_internal_renpy_call_host();
}
void patchfontfunction(){
    //由于不知道怎么从字体名映射到ttc/ttf文件名，所以暂时写死arial/msyh
    if(wcslen(embedsharedmem->fontFamily)==0)return;
    PyRunScript(Renpy_hook_font);
}
void hook_internal_renpy_call_host(){
    HookParam hp_internal;
    hp_internal.address=(uintptr_t)GetProcAddress(GetModuleHandleW(LUNA_HOOK_DLL),"internal_renpy_call_host");
    #ifndef _WIN64
    hp_internal.offset=get_stack(1);
    hp_internal.split=get_stack(2);
    #else
    hp_internal.offset=get_reg(regs::rcx);
    hp_internal.split=get_reg(regs::rdx);
    #endif
    hp_internal.type=USING_SPLIT|USING_STRING|CODEC_UTF16|EMBED_ABLE|EMBED_BEFORE_SIMPLE|EMBED_AFTER_NEW;
    NewHook(hp_internal, "internal_renpy_call_host");
    PyRunScript(Renpy_hook_text);
}
const char* Renpy_hook_font=R"(
try:
    import os
    import renpy
    def hook_renpy_text_font_get_font_init(original):
        def new_init(*args, **kwargs):
            #ctypes.windll.user32.MessageBoxW(None, str(kwargs), str(args), 0)
            if os.path.exists(r'C:\Windows\Fonts\msyh.ttc'):
                font='msyh.ttc'
            elif os.path.exists(r'C:\Windows\Fonts\arial.ttf'):
                font='arial.ttf'
            else:
                font=None
            if font:
                args=(font,)+args[1:]
                if 'fn' in kwargs:
                    kwargs['fn']=font
            return original(*args, **kwargs)

        return new_init
    if "original_renpy_text_font_get_font" not in globals():
        original_renpy_text_font_get_font = renpy.text.font.get_font
    renpy.text.font.get_font = hook_renpy_text_font_get_font_init(original_renpy_text_font_get_font)

except:
    pass
)";

const char* Renpy_hook_text=R"(
def callLunaHost(text,split):
    import ctypes
    try:
        internal_renpy_call_host=ctypes.CDLL('LunaHook64').internal_renpy_call_host
    except:
        internal_renpy_call_host=ctypes.CDLL('LunaHook32').internal_renpy_call_host
    internal_renpy_call_host.argstype=ctypes.c_wchar_p,ctypes.c_int
    internal_renpy_call_host.restype=ctypes.c_wchar_p
    if isinstance(text,str):
        try:
            _text=text.decode('utf8')
        except:
            _text=text
        text=internal_renpy_call_host(_text,split)
    return text
try:
    #6.1.0
    import renpy
    def hook_initT0(original_init):

        def new_init(self, *args, **kwargs):
            
            if isinstance(args[0], list):
                trs = callLunaHost((args[0][0]),1)
            else:
                trs = callLunaHost((args[0]),1)
            
            nargs = (trs,) + args[1:]
            if 'text' in kwargs:
                kwargs['text'] = nargs[0]
            self.mtoolHooked = True

            original_init(self, *nargs, **kwargs)

        return new_init

    if "original_Text_init_hook" not in globals():
        original_Text_init_hook = renpy.text.text.Text.__init__

    renpy.text.text.Text.__init__ = hook_initT0(original_Text_init_hook)

    def hook_init_renderT0(original):
        def new_init(self, *args, **kwargs):
            if not hasattr(self, "LunaHooked"):
                if isinstance(self.text, list):
                    trs = callLunaHost(str(self.text[0]),2)
                else:
                    trs = callLunaHost(str(self.text),2)
                self.set_text(trs)
                self.LunaHooked = True
            return original(self, *args, **kwargs)

        return new_init


    if "original_hook_init_renderT0" not in globals():
        original_hook_init_renderT0 = renpy.text.text.Text.render

    renpy.text.text.Text.render = hook_init_renderT0(original_hook_init_renderT0)
except:
    pass
try:
    #4.0
    import renpy
    def hook_initT3(original_init):
        def new_init(self, *args, **kwargs):
            trs = callLunaHost(str(args[0]),3)
            
            nargs = (trs,) + args[1:]
            original_init(self, *nargs, **kwargs)

        return new_init


    if "original_Text_init_hookT3" not in globals():
        original_Text_init_hookT3 = renpy.exports.Text.__init__

    renpy.exports.Text.__init__ = hook_initT3(original_Text_init_hookT3)
except:
    pass
)";

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