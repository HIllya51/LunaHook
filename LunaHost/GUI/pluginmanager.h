#include"plugin.h"
#include"textthread.h"
class pluginmanager{
    std::vector<std::pair<std::wstring,LPVOID>>OnNewSentenceS;
    std::optional<std::pair<std::wstring,LPVOID>> checkisvalidplugin(const std::wstring&);
    std::vector<std::wstring>readpluginfile();
    void writepluginfile(const std::wstring&);
    std::wstring pluginfilename;
    concurrency::reader_writer_lock OnNewSentenceSLock;
    bool checkisdump(LPVOID);
public:
    pluginmanager();
    bool dispatch(const InfoForExtension*, std::wstring& sentence);
    bool addplugin(const std::wstring&);
};

