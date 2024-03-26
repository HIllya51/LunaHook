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
bool Pluginmanager::addplugin(const std::wstring& p,bool isQt){
    if(isQt){
        host->configs->pluginsadd({p,isQt});
        return true;
    }
    auto plugin=GetProcAddress(LoadLibraryW(p.c_str()),"OnNewSentence");
    if(plugin){ 
        std::scoped_lock lock(OnNewSentenceSLock);
        if(!checkisdump(plugin)){
            PluginRank.push_back(p);
            //std::swap(PluginRank.end()-2,PluginRank.end()-1);
            OnNewSentenceS[p]=plugin;
            host->configs->pluginsadd({p,isQt});
        }
        return true;
    }
    else{
        MessageBoxW(host->winId,InVaildPlugin,L"Error",0);
        return false;
    }
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