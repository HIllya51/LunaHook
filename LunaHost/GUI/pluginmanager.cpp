#include"pluginmanager.h"
#include<filesystem>
#include"Plugin/plugindef.h"
#include<fstream>
#include <commdlg.h>
#include"LunaHost.h"
#include"Lang/Lang.h"
#include"host.h"

std::optional<std::wstring>SelectFile(HWND hwnd,LPCWSTR lpstrFilter){
    OPENFILENAME ofn;
    wchar_t szFileName[MAX_PATH] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = lpstrFilter;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = sizeof(szFileName);
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

    if (GetOpenFileName(&ofn))
    {
        return szFileName;
    }
    else return {};
}

#ifndef _WIN64
#define THISCALL __thiscall
#define _CDECL __cdecl
#define fnQString_fromStdWString "?fromStdWString@QString@@SA?AV1@ABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z"
#define fnQCoreApplication_addLibraryPath "?addLibraryPath@QCoreApplication@@SAXABVQString@@@Z"
#define fnQString_dtor "??1QString@@QAE@XZ"
#define fnQApplication_ctor "??0QApplication@@QAE@AAHPAPADH@Z"
#define fnQFont_ctor "??0QFont@@QAE@ABVQString@@HH_N@Z"
#define fnQApplication_setFont "?setFont@QApplication@@SAXABVQFont@@PBD@Z"
#define fnQFont_dtor "??1QFont@@QAE@XZ"
#define fnQApplication_exec "?exec@QApplication@@SAHXZ"
#define fnQApplication_dtor "??1QApplication@@UAE@XZ"
#else
#define THISCALL __fastcall
#define _CDECL __fastcall
#define fnQString_fromStdWString "?fromStdWString@QString@@SA?AV1@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z"
#define fnQCoreApplication_addLibraryPath "?addLibraryPath@QCoreApplication@@SAXAEBVQString@@@Z"
#define fnQString_dtor "??1QString@@QEAA@XZ"
#define fnQApplication_ctor "??0QApplication@@QEAA@AEAHPEAPEADH@Z"
#define fnQFont_ctor "??0QFont@@QEAA@AEBVQString@@HH_N@Z"
#define fnQApplication_setFont "?setFont@QApplication@@SAXAEBVQFont@@PEBD@Z"
#define fnQFont_dtor "??1QFont@@QEAA@XZ"
#define fnQApplication_exec "?exec@QApplication@@SAHXZ"
#define fnQApplication_dtor "??1QApplication@@UEAA@XZ"
#endif
std::vector<HMODULE> QtLoadLibrarys(std::vector<std::wstring>&collectQtplugs)
{
    std::vector<HMODULE> modules;
    auto Qt5Widgets=LoadLibrary(L"Qt5Widgets.dll");
    auto Qt5Gui=LoadLibrary(L"Qt5Gui.dll");
    auto Qt5Core=LoadLibrary(L"Qt5Core.dll");
    if(Qt5Core==0||Qt5Gui==0||Qt5Widgets==0)return {};
    
    auto QString_fromStdWString=GetProcAddress(Qt5Core,fnQString_fromStdWString);
    auto QCoreApplication_addLibraryPath=GetProcAddress(Qt5Core,fnQCoreApplication_addLibraryPath);
    auto QString_dtor=GetProcAddress(Qt5Core,fnQString_dtor);
    auto QApplication_ctor=GetProcAddress(Qt5Widgets,fnQApplication_ctor);
    auto QFont_ctor=GetProcAddress(Qt5Gui,fnQFont_ctor);
    auto QFont_dtor=GetProcAddress(Qt5Gui,fnQFont_dtor);
    auto QApplication_setFont=GetProcAddress(Qt5Widgets,fnQApplication_setFont);
    auto QApplication_exec=GetProcAddress(Qt5Widgets,fnQApplication_exec);
    auto QApplication_dtor=GetProcAddress(Qt5Widgets,fnQApplication_dtor);
    
    if(QString_fromStdWString==0||QCoreApplication_addLibraryPath==0||QString_dtor==0||QApplication_ctor==0||QFont_ctor==0||QFont_dtor==0||QApplication_setFont==0||QApplication_exec==0||QApplication_dtor==0)return {};
    
    auto mutex=CreateSemaphoreW(0,0,1,0);
    std::thread([=,&modules](){
        static void* qapp;  //必须static
        void* qstring;
        void* qfont;
        for(int i=0;i<collectQtplugs.size();i++){
            auto dirname=std::filesystem::path(collectQtplugs[i]).parent_path().wstring();
            ((void* (_CDECL*)(void*,void*))QString_fromStdWString)(&qstring,&dirname);
            ((void(_CDECL *)(void*))QCoreApplication_addLibraryPath)(&qstring);
            ((void(THISCALL*)(void*))QString_dtor)(&qstring);
            //QCoreApplication_addLibraryPath(QString_fromStdWString(std::filesystem::path(collectQtplugs[i]).parent_path()));
        }
        
        int _=0;
        ((void*(THISCALL*)(void*,int*,char**,int))QApplication_ctor)(&qapp,&_,0,331266);
        
        std::wstring font=L"MS Shell Dlg 2";
        ((void* (_CDECL*)(void*,void*))QString_fromStdWString)(&qstring,&font);
        ((void*(THISCALL*)(void*,void*,int,int,bool))QFont_ctor)(&qfont,&qstring,10,-1,0);
        ((void(_CDECL*)(void*,void*))QApplication_setFont)(&qfont,0);
        ((void(THISCALL*)(void*))QFont_dtor)(&qfont);
        ((void(THISCALL*)(void*))QString_dtor)(&qstring);
        for(int i=0;i<collectQtplugs.size();i++){
            modules.push_back(LoadLibrary(collectQtplugs[i].c_str()));
        }
        ReleaseSemaphore(mutex,1,0);
        
        ((void(*)())QApplication_exec)();
        
        ((void(THISCALL*)(void*))QApplication_dtor)(&qapp);
        
        
    }).detach();
    WaitForSingleObject(mutex,INFINITE);
    
    return modules;
}
typedef  HMODULE*(*QtLoadLibrary_t)(LPWSTR*,int);
QtLoadLibrary_t loadqtloader(const std::filesystem::path&pluginpath){
    auto QtLoaderPath=pluginpath/"QtLoader.dll";
    auto helper=LoadLibrary(QtLoaderPath.c_str());
    if(helper==0)return 0;
    auto QtLoadLibrary = (QtLoadLibrary_t)GetProcAddress(helper, "QtLoadLibrary"); 
    return QtLoadLibrary;
}
void Pluginmanager::loadqtdlls(std::vector<std::wstring>&collectQtplugs){
    if(collectQtplugs.size()==0)return;
    auto pluginpath=std::filesystem::current_path()/(x64?"plugin64":"plugin32");
    wchar_t env[65535];
    GetEnvironmentVariableW(L"PATH",env,65535);
    auto envs=std::wstring(env);
    envs+=L";";envs+=pluginpath;
    for(auto&p:collectQtplugs){
        envs+=L";";envs+=std::filesystem::path(p).parent_path();
    }
    SetEnvironmentVariableW(L"PATH",envs.c_str());
    
    // auto modules=QtLoadLibrarys(collectQtplugs);
    // if(modules.empty())return;
    

    auto QtLoadLibrary = loadqtloader(pluginpath); 
    if(!QtLoadLibrary){
        MessageBoxW(host->winId,CantLoadQtLoader,L"Error",0);
        return ;
    }
    std::vector<wchar_t*>saves;
    for(auto&p:collectQtplugs){
        auto str=new wchar_t[p.size()+1];
        wcscpy(str,p.c_str());
        saves.emplace_back(str);
    }
    auto modules=QtLoadLibrary(saves.data(),collectQtplugs.size());
    for(auto str:saves)delete str;

    for(int i=0;i<collectQtplugs.size();i++){
        OnNewSentenceS[collectQtplugs[i]]=GetProcAddress(modules[i],"OnNewSentence"); 
    }
}
Pluginmanager::Pluginmanager(LunaHost* _host):host(_host){
    try {
        std::scoped_lock lock(OnNewSentenceSLock);
        
        std::vector<std::wstring>collectQtplugs;
        for (auto i=0;i<host->configs->pluginsnum();i++) {
            auto plg=host->configs->pluginsget(i);
            bool isqt=plg.isQt;
            auto path=plg.wpath();
            PluginRank.push_back(path);
            OnNewSentenceS[path]=0;
            if(isqt){
                collectQtplugs.push_back((path));
                continue;
            }
            OnNewSentenceS[path]=GetProcAddress(LoadLibraryW(path.c_str()),"OnNewSentence");
        }
        loadqtdlls(collectQtplugs);
        
        OnNewSentenceS[L"InternalClipBoard"]=GetProcAddress(GetModuleHandle(0),"OnNewSentence");//内部链接的剪贴板插件

        
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
}

bool Pluginmanager::dispatch(TextThread& thread, std::wstring& sentence){
    auto sentenceInfo=GetSentenceInfo(thread).data();
    wchar_t* sentenceBuffer = (wchar_t*)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, (sentence.size() + 1) * sizeof(wchar_t));
	wcscpy_s(sentenceBuffer, sentence.size() + 1, sentence.c_str());
    concurrency::reader_writer_lock::scoped_lock_read readLock(OnNewSentenceSLock);

    bool interupt=false;
    for (const auto& pathl :{std::vector<std::wstring>{L"InternalClipBoard"}, PluginRank}){
        for(const auto&path:pathl){
            auto funptr=OnNewSentenceS[path];
            if(funptr==0)continue;
            if (!*(sentenceBuffer = ((OnNewSentence_t)funptr)(sentenceBuffer, sentenceInfo))){interupt=true;break;};
        }
        if(interupt)break;
    }
    
	sentence = sentenceBuffer;
	HeapFree(GetProcessHeap(), 0, sentenceBuffer);
	return !sentence.empty();
}

std::optional<LPVOID> Pluginmanager::checkisvalidplugin(const std::wstring& pl){
    auto path=std::filesystem::path(pl);
    if (!std::filesystem::exists(path))return{};
    if (!std::filesystem::is_regular_file(path))return{};
    auto appendix=stolower(path.extension().wstring());
    if((appendix!=std::wstring(L".dll"))&&(appendix!=std::wstring(L".xdll")))return {};
    auto dll=LoadLibraryW(pl.c_str());
    if(!dll)return {};
    auto OnNewSentence=GetProcAddress(LoadLibraryW(pl.c_str()),"OnNewSentence");
    if(!OnNewSentence){
        FreeLibrary(dll);
        return {};
    }
    return OnNewSentence ;
}
bool Pluginmanager::checkisdump(LPVOID ptr){
    for(auto& p:OnNewSentenceS){
        if(p.second==ptr)return true;
    }
    return false;
}
void Pluginmanager::remove(const std::wstring& ss){
    std::scoped_lock lock(OnNewSentenceSLock);
    auto it = std::find(PluginRank.begin(), PluginRank.end(), ss);
    if (it != PluginRank.end()) { 
        PluginRank.erase(it);
    }
    host->configs->pluginsremove(WideStringToString(ss));
}
std::optional<std::wstring>Pluginmanager::selectpluginfile(){
    return SelectFile(host->winId,L"Plugin Files\0*.dll;*.xdll\0");
}
void Pluginmanager::swaprank(int a,int b){
    std::scoped_lock lock(OnNewSentenceSLock);
    auto _b=PluginRank[b];
    PluginRank[b]=PluginRank[a];
    PluginRank[a]=_b;
    host->configs->pluginrankswap(a,b);
}
DWORD Rva2Offset(DWORD rva, PIMAGE_SECTION_HEADER psh, PIMAGE_NT_HEADERS pnt)
{
    size_t i = 0;
    PIMAGE_SECTION_HEADER pSeh;
    if (rva == 0)
    {
        return (rva);
    }
    pSeh = psh;
    for (i = 0; i < pnt->FileHeader.NumberOfSections; i++)
    {
        if (rva >= pSeh->VirtualAddress && rva < pSeh->VirtualAddress +
            pSeh->Misc.VirtualSize)
        {
            break;
        }
        pSeh++;
    }
    return (rva - pSeh->VirtualAddress + pSeh->PointerToRawData);
} 
std::set<std::string> getimporttable(const std::wstring&pe){
    AutoHandle handle = CreateFile(pe.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(!handle)return {};
    DWORD byteread, size = GetFileSize(handle, NULL);
    PVOID virtualpointer = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
    if(!virtualpointer) return {};
    ReadFile(handle, virtualpointer, size, &byteread, NULL);
    
    struct __{
        PVOID _ptr;DWORD size;
        __(PVOID ptr,DWORD sz):_ptr(ptr),size(sz){}
        ~__(){
            VirtualFree(_ptr, size, MEM_DECOMMIT);
        }
    }_(virtualpointer,size);
    

    if(PIMAGE_DOS_HEADER(virtualpointer)->e_magic!=0x5a4d) 
        return {}; 

    PIMAGE_NT_HEADERS           ntheaders = (PIMAGE_NT_HEADERS)(PCHAR(virtualpointer) + PIMAGE_DOS_HEADER(virtualpointer)->e_lfanew);

    auto magic=ntheaders->OptionalHeader.Magic;
    if(x64 && (magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC))return {};
    if((!x64)&&(magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC))return {};

    PIMAGE_SECTION_HEADER       pSech = IMAGE_FIRST_SECTION(ntheaders);//Pointer to first section header
    PIMAGE_IMPORT_DESCRIPTOR    pImportDescriptor; //Pointer to import descriptor 

    
    if (ntheaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size == 0)/*if size of the table is 0 - Import Table does not exist */
        return {};

    std::set<std::string> ret;
    pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD_PTR)virtualpointer + \
        Rva2Offset(ntheaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress, pSech, ntheaders));
        
    while (pImportDescriptor->Name != NULL)
    {
        //Get the name of each DLL
        ret.insert((PCHAR)((DWORD_PTR)virtualpointer + Rva2Offset(pImportDescriptor->Name, pSech, ntheaders)));
        
        pImportDescriptor++; //advance to next IMAGE_IMPORT_DESCRIPTOR
    }
    return ret;

}
bool qtchecker(const std::set<std::string>& dll)
{
    for(auto qt5:{"Qt5Widgets.dll","Qt5Gui.dll","Qt5Core.dll"})
        if(dll.find(qt5)!=dll.end())
            return true;
    return false;
}
bool Pluginmanager::addplugin(const std::wstring& p){
    auto importtable=getimporttable(p);
    if(importtable.empty())return false;
    auto isQt=qtchecker(importtable);
    if(isQt){
        host->configs->pluginsadd({p,isQt});
        return true;
    }
    auto plugin=GetProcAddress(LoadLibraryW(p.c_str()),"OnNewSentence");
    if(!plugin)return false;
    
    std::scoped_lock lock(OnNewSentenceSLock);
    if(!checkisdump(plugin)){
        PluginRank.push_back(p);
        //std::swap(PluginRank.end()-2,PluginRank.end()-1);
        OnNewSentenceS[p]=plugin;
        host->configs->pluginsadd({p,isQt});
    }
    return true;
}


std::array<InfoForExtension, 20> Pluginmanager::GetSentenceInfo(TextThread& thread)
{
    void (*AddText)(int64_t, const wchar_t*) = [](int64_t number, const wchar_t* text)
    {
        if (TextThread* thread = Host::GetThread(number)) thread->Push(text);
    };
    void (*AddSentence)(int64_t, const wchar_t*) = [](int64_t number, const wchar_t* sentence)
    {
        if (TextThread* thread = Host::GetThread(number)) thread->AddSentence(sentence);;
    };
    static DWORD SelectedProcessId;
    auto currthread=Host::GetThread(host->currentselect);
    SelectedProcessId=(currthread!=0)?currthread->tp.processId:0;
    DWORD (*GetSelectedProcessId)() = [] { return SelectedProcessId; };

    return
    { {
    { "HostHWND",(int64_t)host->winId },
    { "toclipboard", host->check_toclipboard },
    { "current select", &thread == currthread },
    { "text number", thread.handle },
    { "process id", thread.tp.processId },
    { "hook address", (int64_t)thread.tp.addr },
    { "text handle", thread.handle },
    { "text name", (int64_t)thread.name.c_str() },
    { "add sentence", (int64_t)AddSentence },
    { "add text", (int64_t)AddText },
    { "get selected process id", (int64_t)GetSelectedProcessId },
    { "void (*AddSentence)(int64_t number, const wchar_t* sentence)", (int64_t)AddSentence },
    { "void (*AddText)(int64_t number, const wchar_t* text)", (int64_t)AddText },
    { "DWORD (*GetSelectedProcessId)()", (int64_t)GetSelectedProcessId },
    { nullptr, 0 } // nullptr marks end of info array
    } };
}