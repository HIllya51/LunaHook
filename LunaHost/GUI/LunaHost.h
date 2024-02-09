#include"window.h"
#include"controls.h"
#include"processlistwindow.h"
#include"textthread.h"
#include"pluginmanager.h"
#include"confighelper.h"
class LunaHost;
class Pluginwindow:public mainwindow{
    listbox* listplugins;
    Pluginmanager* pluginmanager;
public:
    Pluginwindow(mainwindow*,Pluginmanager*);
    void on_size(int w,int h);
};
class Settingwindow:public mainwindow{
    checkbox* ckbfilterrepeat;
    spinbox* g_timeout;
    spinbox* g_codepage;
    checkbox* g_check_clipboard; 
public:
Settingwindow(LunaHost*);
};
class LunaHost:public mainwindow{
    Pluginwindow* pluginwindow=0;
    std::map<int64_t,std::vector<std::wstring>>savetext;
    std::vector<int>attachedprocess;
    std::mutex settextmutex;
    textedit* g_hEdit_userhook;
    button* g_hButton_insert;
    button* btnplugin;
    listbox* g_hListBox_listtext;
    textedit* g_showtexts;
    button* g_selectprocessbutton;
    button* btnshowsettionwindow;
    void toclipboard(std::wstring& sentence);
    processlistwindow *_processlistwindow=0;
    Settingwindow *settingwindow=0;
    Pluginmanager* plugins;
    bool on_text_recv(TextThread& thread, std::wstring& sentence);
    void on_thread_create(TextThread& thread);
    void on_thread_delete(TextThread& thread);
public:
    confighelper* configs;
    int64_t currentselect=0;
    bool check_toclipboard=false; 
    void on_size(int w,int h);
    void on_close();
    LunaHost();
};
