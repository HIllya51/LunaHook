#ifndef LUNA_PLUGINMANAGER_H
#define LUNA_PLUGINMANAGER_H
#include"Plugin/plugindef.h"
#include"textthread.h"
class LunaHost;
class pluginmanager{
    std::vector<std::pair<std::wstring,LPVOID>>OnNewSentenceS;
    std::optional<std::pair<std::wstring,LPVOID>> checkisvalidplugin(const std::wstring&);
    std::vector<std::wstring>readpluginfile();
    void writepluginfile(const std::wstring&);
    std::wstring pluginfilename;
    concurrency::reader_writer_lock OnNewSentenceSLock;
    bool checkisdump(LPVOID);
    LunaHost* host;
    std::array<InfoForExtension, 20> GetSentenceInfo(TextThread& thread);
public:
    pluginmanager(LunaHost*);
    bool dispatch(TextThread&, std::wstring& sentence);
    bool addplugin(const std::wstring&);
    std::optional<std::wstring>selectpluginfile();
};

#endif