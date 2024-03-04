#ifndef LUNA_CONFIG_HELPER
#define LUNA_CONFIG_HELPER
#include<nlohmann/json.hpp>
struct pluginitem{
    std::string path;
    bool isQt;
    bool isref;
    pluginitem(const nlohmann::json&);
    pluginitem(const std::wstring&,bool);
    std::wstring wpath();
    nlohmann::json dump() const;
};
class confighelper{
    nlohmann::json configs;
    std::wstring configpath;
public:
    confighelper();
    ~confighelper();
    pluginitem pluginsget(int);
    int pluginsnum();
    void pluginsadd(const pluginitem&);
    void pluginsremove(const std::string&);
    void pluginrankswap(int,int);
    template<class T>
    T get(const std::string&key,T default1){
        if(configs.find(key)==configs.end())return default1;
        return configs[key];
    }
    template<class T>
    void set(const std::string&key,T v){
        configs[key]=v;
    }
};

#endif