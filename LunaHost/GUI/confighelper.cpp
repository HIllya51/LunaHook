#include"confighelper.h"
#include"stringutils.h"
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
void writefile(const wchar_t* fname,const std::string& s){
    FILE* f;
    _wfopen_s(&f, fname, L"w");
    fprintf(f,"%s",s.c_str());
    fclose(f);
}

confighelper::confighelper(){
    configpath=std::filesystem::current_path()/(x64?"config64.json":"config32.json");
    try{
        configs=nlohmann::json::parse(readfile(configpath.c_str()));
    }
    catch(std::exception &){
        configs={};
    }

    if(configs.find("plugins")==configs.end()){
        configs["plugins"]={};
    }
    
}
confighelper::~confighelper(){

    writefile(configpath.c_str(), configs.dump(4));
}
void confighelper::pluginsremove(const std::string&s){
    auto &plgs=configs["plugins"];
    auto it=std::remove_if(plgs.begin(),plgs.end(),[&](auto&t){
        std::string p=t["path"];
        return p==s;
    });
    plgs.erase(it, plgs.end());
}
void confighelper::pluginrankswap(int a,int b){
    auto &plgs=configs["plugins"];
    auto _b=plgs[b];
    plgs[b]=plgs[a];
    plgs[a]=_b;
}
void confighelper::pluginsadd(const pluginitem& item){
    configs["plugins"].push_back(item.dump());
}
int confighelper::pluginsnum(){
    return configs["plugins"].size();
}
pluginitem confighelper::pluginsget(int i){
    return pluginitem{configs["plugins"][i]};
}
template<typename T>
T safequeryjson(const nlohmann::json& js,const std::string& key,const T &defaultv){
    if(js.find(key)==js.end()){
        return defaultv;
    }
    return js[key];
}
pluginitem::pluginitem(const nlohmann::json& js){
    path=js["path"];
    isQt=safequeryjson(js,"isQt",false);
    isref=safequeryjson(js,"isref",false);
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
}
nlohmann::json pluginitem::dump() const{
    return {
        {"path",path},
        {"isQt",isQt},
        {"isref",isref}
    };
}