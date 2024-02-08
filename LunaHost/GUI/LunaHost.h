#include"window.h"
#include"controls.h"
#include"processlistwindow.h"
#include"textthread.h"
#include"pluginmanager.h"
#include"Plugin/plugindef.h"
class LunaHost:public mainwindow{
        
    int64_t currentselect=0;
    std::map<int64_t,std::vector<std::wstring>>savetext;
    std::vector<int>attachedprocess;
    bool check_toclipboard=false; 
    std::mutex settextmutex;
    textedit* g_hEdit_userhook;
    button* g_hButton_insert;
    button* btnaddplugin;
    listbox* g_hListBox_listtext;
    textedit* g_showtexts;
    button* g_selectprocessbutton;
    textedit* g_timeout;
    textedit* g_codepage;
    checkbox* g_check_clipboard; 
    void toclipboard(std::wstring& sentence);
    processlistwindow *_processlistwindow=0;
    pluginmanager* plugins;
    bool on_text_recv(TextThread& thread, std::wstring& sentence);
    void on_thread_create(TextThread& thread);
    void on_thread_delete(TextThread& thread);
    std::array<InfoForExtension, 20> GetSentenceInfo(TextThread& thread);
public:
    void on_size(int w,int h);
    void on_close();
    LunaHost();
};