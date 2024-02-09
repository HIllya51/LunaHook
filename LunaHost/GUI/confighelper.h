#ifndef LUNA_CONFIG_HELPER
#define LUNA_CONFIG_HELPER
#include<nlohmann/json.hpp>
class confighelper{
    nlohmann::json configs;
    std::wstring configpath;
public:
    confighelper();
    ~confighelper();
    nlohmann::json pluginsget();
    void pluginsadd(const std::string&,bool);
    void pluginsremove(const std::string&);
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