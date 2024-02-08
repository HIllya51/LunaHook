#include"pluginmanager.h"
#include<filesystem>
#include"Plugin/plugindef.h"
#include<fstream>
#include"Lang/Lang.h"
std::vector<char> readfile(const wchar_t* fname) {
    FILE* f;
    _wfopen_s(&f, fname, L"rb");
    if (f == 0)return {};
    fseek(f, 0, SEEK_END);
    auto len = ftell(f);
    fseek(f, 0, SEEK_SET);
    auto buff = std::vector<char>(len);
    fread(buff.data(), 1, len, f);
    fclose(f);
    return buff;
} 
std::vector<std::wstring>pluginmanager::readpluginfile(){
    if(!std::filesystem::exists(pluginfilename))
        return{};
    auto u16pl=StringToWideString(readfile(pluginfilename.c_str()).data());
    auto pls=strSplit(u16pl,L"\n");
    return pls;
}
void pluginmanager::writepluginfile(const std::wstring& plugf){
    auto u8s=WideStringToString(plugf);
    FILE* f;
    _wfopen_s(&f, pluginfilename.c_str(), L"a");
    fprintf(f,"\n%s",u8s.c_str());
    fclose(f);
}
pluginmanager::pluginmanager(){
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

bool pluginmanager::dispatch(const InfoForExtension* sentenceInfo, std::wstring& sentence){
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
    auto appendix=path.extension().wstring();
    stolower(appendix);
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
        MessageBoxW(0,InVaildPlugin,L"Error",0);
        return false;
    }
}