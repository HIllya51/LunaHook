#ifndef LUNA_PLUGINMANAGER_H
#define LUNA_PLUGINMANAGER_H
#include"Plugin/plugindef.h"
#include"textthread.h"
class LunaHost;
class Pluginmanager{
    std::unordered_map<std::wstring,LPVOID>OnNewSentenceS;
    std::optional<LPVOID> checkisvalidplugin(const std::wstring&);
    concurrency::reader_writer_lock OnNewSentenceSLock;
    bool checkisdump(LPVOID);
    LunaHost* host;
    std::array<InfoForExtension, 20> GetSentenceInfo(TextThread& thread);
    void loadqtdlls(std::vector<std::wstring>&collectQtplugs);
public:
    std::vector<std::wstring>PluginRank;
    Pluginmanager(LunaHost*);
    bool dispatch(TextThread&, std::wstring& sentence);
    bool addplugin(const std::wstring&,bool isQt=false);
    void remove(const std::wstring&);
    std::optional<std::wstring>selectpluginfile();
};

#endif