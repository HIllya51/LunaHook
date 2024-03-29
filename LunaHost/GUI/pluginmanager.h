#ifndef LUNA_PLUGINMANAGER_H
#define LUNA_PLUGINMANAGER_H
#include"Plugin/plugindef.h"
#include"textthread.h"
class LunaHost;
enum class addpluginresult{
    success,
    invaliddll,
    isnotaplugins,
    dumplicate
};
class Pluginmanager{
    std::unordered_map<std::wstring,LPVOID>OnNewSentenceS;
    std::optional<LPVOID> checkisvalidplugin(const std::wstring&);
    concurrency::reader_writer_lock OnNewSentenceSLock;
    bool checkisdump(const std::wstring&);
    LunaHost* host;
    std::array<InfoForExtension, 20> GetSentenceInfo(TextThread& thread);
    void loadqtdlls(std::vector<std::wstring>&collectQtplugs);
public:
    std::vector<std::wstring>PluginRank;
    Pluginmanager(LunaHost*);
    bool dispatch(TextThread&, std::wstring& sentence);
    addpluginresult addplugin(const std::wstring&);
    void swaprank(int,int);
    void remove(const std::wstring&);
    std::optional<std::wstring>selectpluginfile();
};

#endif