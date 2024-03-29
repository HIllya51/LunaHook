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

typedef  std::vector<HMODULE>*(* QtLoadLibrary_t)(std::vector<std::wstring>* dlls);

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

    auto QtLoadLibrary = (QtLoadLibrary_t)GetProcAddress(GetModuleHandle(0), "QtLoadLibrary"); 
    
    auto modules=QtLoadLibrary(&collectQtplugs);
    
    if(modules->empty())return;
    
    for(int i=0;i<collectQtplugs.size();i++){
        OnNewSentenceS[collectQtplugs[i]]=GetProcAddress(modules->at(i),"OnNewSentence"); 
    }

    delete modules;
}
Pluginmanager::Pluginmanager(LunaHost* _host):host(_host),configs(_host->configs){
    try {
        std::scoped_lock lock(OnNewSentenceSLock);
        
        std::vector<std::wstring>collectQtplugs;
        for (auto i=0;i<count();i++) {
            auto plg=get(i);
            bool isqt=plg.isQt;
            auto path=plg.wpath();
            OnNewSentenceS[path]=0;
            if(isqt){
                if(plg.enable==false)continue;
                collectQtplugs.push_back((path));
            }
            else
                OnNewSentenceS[path]=GetProcAddress(LoadLibraryW(path.c_str()),"OnNewSentence");
        }
        loadqtdlls(collectQtplugs);
        
        OnNewSentenceS[L"InternalClipBoard"]=GetProcAddress(GetModuleHandle(0),"OnNewSentence");//内部链接的剪贴板插件

        
    } catch (const std::exception& ex) {
        std::wcerr << "Error: " << ex.what() << std::endl;
    }
}

bool Pluginmanager::dispatch(TextThread& thread, std::wstring& sentence){
    auto sentenceInfo=GetSentenceInfo(thread).data();
    wchar_t* sentenceBuffer = (wchar_t*)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, (sentence.size() + 1) * sizeof(wchar_t));
	wcscpy_s(sentenceBuffer, sentence.size() + 1, sentence.c_str());
    concurrency::reader_writer_lock::scoped_lock_read readLock(OnNewSentenceSLock);

    for(int i=0;i<count()+1;i++){
        std::wstring path;
        if(i==count())path=L"InternalClipBoard";
        else {
            if(getenable(i)==false)continue;
            path=getname(i);
        }

        auto funptr=OnNewSentenceS[path];
        if(funptr==0)continue;
        if (!*(sentenceBuffer = ((OnNewSentence_t)funptr)(sentenceBuffer, sentenceInfo)))
            break;
    }
    
	sentence = sentenceBuffer;
	HeapFree(GetProcessHeap(), 0, sentenceBuffer);
	return !sentence.empty();
}

void Pluginmanager::remove(const std::string&s){
    
    auto &plgs=configs->configs["plugins"];
    auto it=std::remove_if(plgs.begin(),plgs.end(),[&](auto&t){
        std::string p=t["path"];
        return p==s;
    });
    plgs.erase(it, plgs.end());
}
void Pluginmanager::add(const pluginitem& item){
    configs->configs["plugins"].push_back(item.dump());
}
int Pluginmanager::count(){
    return configs->configs["plugins"].size();
}
pluginitem Pluginmanager::get(int i){
    return pluginitem{configs->configs["plugins"][i]};
}
void Pluginmanager::set(int i,const pluginitem& item){
    configs->configs["plugins"][i]=item.dump();
}

pluginitem::pluginitem(const nlohmann::json& js){
    path=js["path"];
    isQt=safequeryjson(js,"isQt",false);
    isref=safequeryjson(js,"isref",false);
    enable=safequeryjson(js,"enable",true);
}
std::wstring pluginitem::wpath(){
    auto wp=StringToWideString(path);
    if(isref)return std::filesystem::current_path()/wp;
    else return wp;
}

std::pair<std::wstring,bool> castabs2ref(const std::wstring&p){
    auto curr=std::filesystem::current_path().wstring();
    if(startWith(p,curr)){
        return {p.substr(curr.size()+1),true};
    }
    return {p,false};
}
pluginitem::pluginitem(const std::wstring& pabs,bool _isQt){
    isQt=_isQt;
    auto [p,_isref]=castabs2ref(pabs);
    isref=_isref;
    path=WideStringToString(p);
    enable=true;
}
nlohmann::json pluginitem::dump() const{
    return {
        {"path",path},
        {"isQt",isQt},
        {"isref",isref},
        {"enable",enable}
    };
} 
bool Pluginmanager::getenable(int idx){
    return get(idx).enable;
}
void Pluginmanager::setenable(int idx,bool en){
    auto item=get(idx);
    item.enable=en;
    set(idx,item);
}
std::wstring Pluginmanager::getname(int idx){
    return get(idx).wpath();
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
bool Pluginmanager::checkisdump(const std::wstring& dll){
    for(auto& p:OnNewSentenceS){
        if(p.first==dll)return true;
    }
    return false;
}
void Pluginmanager::remove(const std::wstring& ss){
    remove(WideStringToString(ss));
}
std::optional<std::wstring>Pluginmanager::selectpluginfile(){
    return SelectFile(0,L"Plugin Files\0*.dll;*.xdll\0");
}
void Pluginmanager::swaprank(int a,int b){
    auto &plgs=configs->configs["plugins"];
    auto _b=plgs[b];
    plgs[b]=plgs[a];
    plgs[a]=_b;
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
addpluginresult Pluginmanager::addplugin(const std::wstring& p){
    auto importtable=getimporttable(p);
    if(importtable.empty())return addpluginresult::invaliddll;
    auto isQt=qtchecker(importtable);
    LPVOID plugin;
    if(isQt){
        plugin=0;
    }
    else{
        auto base=LoadLibraryW(p.c_str());
        if(base==0)return addpluginresult::invaliddll;
        plugin=GetProcAddress(base,"OnNewSentence");
        if(!plugin)return addpluginresult::isnotaplugins;
    } 
    
    std::scoped_lock lock(OnNewSentenceSLock);
    if(checkisdump(p))return addpluginresult::dumplicate;

    OnNewSentenceS[p]=plugin;
    add({p,isQt});

    return addpluginresult::success;
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