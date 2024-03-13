#include"python.h"
#include"embed_util.h"
#include"main.h"
#include"stackoffset.hpp"
#include"types.h"
#include"defs.h"
#include <windows.h>
#include <shlobj.h>

extern "C" __declspec(dllexport) const wchar_t* internal_renpy_call_host(const wchar_t* text,int split){
    return text;
}
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

typedef BOOL(WINAPI* PGFRI)(LPCWSTR, LPDWORD, LPVOID, DWORD);
#define QFR_LOGFONT (2)
#define LOADFONTTHREADNUM 4
std::unordered_map<std::wstring, std::wstring> loadfontfiles() {

    PWSTR localAppDataPath;
    HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &localAppDataPath);
    std::unordered_map<std::wstring, std::wstring> fnts;
    
    std::vector< std::wstring>collectfile;
    for (auto fontdir : { std::wstring(LR"(C:\Windows\Fonts)"),std::wstring(localAppDataPath) + LR"(\Microsoft\Windows\Fonts)" }) {

        for (auto entry : std::filesystem::directory_iterator(fontdir)) {
            collectfile.emplace_back(entry.path());
        }
    }
    std::vector<std::thread>ts;
    std::vector<decltype(fnts)> fntss(LOADFONTTHREADNUM);
    auto singletask = [&](int i) {
        HINSTANCE hGdi32 = GetModuleHandleA("gdi32.dll");
        if (hGdi32 == 0)return;
        PGFRI GetFontResourceInfo = (PGFRI)GetProcAddress(hGdi32, "GetFontResourceInfoW");
        for (auto j = i; j < collectfile.size(); j += LOADFONTTHREADNUM) {
            auto fontfile = collectfile[j];
            DWORD dwFontsLoaded = AddFontResourceExW(fontfile.c_str(), FR_PRIVATE, 0);
            if (dwFontsLoaded == 0) {
                continue;
            }

            auto lpLogfonts = std::make_unique<LOGFONTW[]>(dwFontsLoaded);
            DWORD cbBuffer = dwFontsLoaded * sizeof(LOGFONTW);
            if (!GetFontResourceInfo(fontfile.c_str(), &cbBuffer, lpLogfonts.get(), QFR_LOGFONT)) {
                RemoveFontResourceExW(fontfile.c_str(), FR_PRIVATE, 0);
                continue;
            }
            for (int k = 0; k < dwFontsLoaded; k++)
                fntss[i].insert(std::make_pair(lpLogfonts[k].lfFaceName, fontfile));
            RemoveFontResourceExW(fontfile.c_str(), FR_PRIVATE, 0);

        }
        };
    for (int i = 0; i < LOADFONTTHREADNUM; i++) {
        ts.emplace_back(std::thread(singletask,i));
    }
    for (int i = 0; i < LOADFONTTHREADNUM; i++)
        ts[i].join();
    for (int i = 0; i < LOADFONTTHREADNUM; i++) {
        for (auto p : fntss[i])
            fnts.insert(std::move(p));
    }
    return fnts;
}
}
extern "C" __declspec(dllexport) const wchar_t* internal_renpy_get_font(){
    if(wcslen(embedsharedmem->fontFamily)==0)return NULL;
    static auto fontname2fontfile=std::move(loadfontfiles());
    if(fontname2fontfile.find(embedsharedmem->fontFamily)==fontname2fontfile.end())return NULL;
    else return fontname2fontfile.at(embedsharedmem->fontFamily).c_str();

}
void hookrenpy(HMODULE module){
    LoadPyRun(module);
    patch_fun=[](){
        PyRunScript(LoadResData(L"renpy_hook_font",L"PYSOURCE").c_str());
    };
    hook_internal_renpy_call_host();
}