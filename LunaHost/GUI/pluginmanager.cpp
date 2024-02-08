#include"pluginmanager.h"
#include<filesystem>
#include"Plugin/plugindef.h"
#include<fstream>
#include <commdlg.h>
#include"LunaHost.h"
#include"Lang/Lang.h"
#include"host.h"
std::string readfile(const wchar_t* fname) {
    FILE* f;
    _wfopen_s(&f, fname, L"rb");
    if (f == 0)return {};
    fseek(f, 0, SEEK_END);
    auto len = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string buff;
    buff.resize(len);
    fread(buff.data(), 1, len, f);
    fclose(f);
    return buff;
} 
void appendfile(const wchar_t* fname,const std::wstring& s){
    auto u8s=WideStringToString(s);
    FILE* f;
    _wfopen_s(&f, fname, L"a");
    fprintf(f,"\n%s",u8s.c_str());
    fclose(f);
}
std::optional<std::wstring>SelectFile(HWND hwnd,LPCWSTR lpstrFilter){
    OPENFILENAME ofn;
    wchar_t szFileName[MAX_PATH] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = lpstrFilter;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = sizeof(szFileName);
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileName(&ofn))
    {
        return szFileName;
    }
    else return {};
}

std::vector<std::wstring>pluginmanager::readpluginfile(){
    if(!std::filesystem::exists(pluginfilename))
        return{};
    auto u16pl=StringToWideString(readfile(pluginfilename.c_str()).data());
    auto pls=strSplit(u16pl,L"\n");
    return pls;
}
void pluginmanager::writepluginfile(const std::wstring& plugf){
    appendfile(pluginfilename.c_str(),plugf);
}
pluginmanager::pluginmanager(LunaHost* _host):host(_host){
    try {
        std::scoped_lock lock(OnNewSentenceSLock);
        pluginfilename=std::filesystem::current_path()/(x64?"plugin64.txt":"plugin32.txt");
        OnNewSentenceS.push_back({L"Internal ClipBoard",GetProcAddress(GetModuleHandle(0),"OnNewSentence")});//内部链接的剪贴板插件
        
        for (auto &pl:readpluginfile()) {
            
            auto ret=checkisvalidplugin(pl);
            if(ret.has_value()){
                auto v=ret.value();
                if(!checkisdump(v.second)){
                    OnNewSentenceS.push_back(v);
                }
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
}

bool pluginmanager::dispatch(TextThread& thread, std::wstring& sentence){
    auto sentenceInfo=GetSentenceInfo(thread).data();
    wchar_t* sentenceBuffer = (wchar_t*)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, (sentence.size() + 1) * sizeof(wchar_t));
	wcscpy_s(sentenceBuffer, sentence.size() + 1, sentence.c_str());
    concurrency::reader_writer_lock::scoped_lock_read readLock(OnNewSentenceSLock);
    for (const auto& extension : OnNewSentenceS){
		if (!*(sentenceBuffer = ((OnNewSentence_t)extension.second)(sentenceBuffer, sentenceInfo))) break;
    }
	sentence = sentenceBuffer;
	HeapFree(GetProcessHeap(), 0, sentenceBuffer);
	return !sentence.empty();
}

std::optional<std::pair<std::wstring,LPVOID>> pluginmanager::checkisvalidplugin(const std::wstring& pl){
    auto path=std::filesystem::path(pl);
    if (!std::filesystem::exists(path))return{};
    if (!std::filesystem::is_regular_file(path))return{};
    auto appendix=stolower(path.extension().wstring());
    if((appendix!=std::wstring(L".dll"))&&(appendix!=std::wstring(L".xdll")))return {};
    auto dll=LoadLibraryW(pl.c_str());
    if(!dll)return {};
    auto OnNewSentence=GetProcAddress(dll,"OnNewSentence");
    if(!OnNewSentence){
        FreeLibrary(dll);
        return {};
    }
    return std::make_pair(path.stem(), OnNewSentence);
}
bool pluginmanager::checkisdump(LPVOID ptr){
    for(auto& p:OnNewSentenceS){
        if(p.second==ptr)return true;
    }
    return false;
}

std::optional<std::wstring>pluginmanager::selectpluginfile(){
    return SelectFile(host->winId,L"Plugin Files\0*.dll;*.xdll\0");
}
bool pluginmanager::addplugin(const std::wstring& p){
    auto plugin=checkisvalidplugin(p);
    if(plugin.has_value()){
        auto v=plugin.value();
        std::scoped_lock lock(OnNewSentenceSLock);
        if(!checkisdump(v.second)){
            OnNewSentenceS.push_back(v);
            writepluginfile(p);
        }
        return true;
    }
    else{
        MessageBoxW(host->winId,InVaildPlugin,L"Error",0);
        return false;
    }
}


std::array<InfoForExtension, 20> pluginmanager::GetSentenceInfo(TextThread& thread)
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