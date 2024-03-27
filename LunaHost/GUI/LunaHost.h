#include"window.h"
#include"controls.h"
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
    checkbox* readonlycheck;
    checkbox* autoattach;
    checkbox* autoattach_so;
    spinbox* spinmaxbuffsize;
    spinbox* spinmaxhistsize;
public:
Settingwindow(LunaHost*);
};

class processlistwindow:public mainwindow{
    textedit* g_hEdit;
    button* g_hButton;
    listview* g_hListBox;
    button* g_refreshbutton;  
    std::unordered_map<std::wstring,std::vector<int>> g_exe_pid; 
    void PopulateProcessList(listview*,std::unordered_map<std::wstring,std::vector<int>>&);
public:
    processlistwindow(mainwindow* parent=0);
    void on_size(int w,int h);
    void on_show();
};

class LunaHost:public mainwindow{
    Pluginwindow* pluginwindow=0;
    std::map<int64_t,std::vector<std::wstring>>savetext;
    std::set<int>attachedprocess;
    std::mutex settextmutex;
    textedit* g_hEdit_userhook;
    button* g_hButton_insert;
    button* btnplugin;
    listbox* g_hListBox_listtext;
    textedit* g_showtexts;
    button* g_selectprocessbutton;
    button* btndetachall;
    button* btnshowsettionwindow;
    //button* btnsavehook;
    void toclipboard(std::wstring& sentence);
    processlistwindow *_processlistwindow=0;
    Settingwindow *settingwindow=0;
    Pluginmanager* plugins;
    bool on_text_recv(TextThread& thread, std::wstring& sentence);
    void on_text_recv_checkissaved(TextThread& thread);
    void on_thread_create(TextThread& thread);
    void on_thread_delete(TextThread& thread);
    void on_proc_connect(DWORD pid);
public:
    confighelper* configs;
    int64_t currentselect=0;
    bool check_toclipboard; 
    bool autoattach;
    bool autoattach_savedonly;
    std::set<std::string>autoattachexes;
    std::unordered_map<std::string,nlohmann::json>savedhookcontext;
    std::set<int>userdetachedpids;
    std::set<int64_t>onceautoselectthread;
    void on_size(int w,int h);
    void on_close();
    LunaHost();
    friend class Settingwindow;
    friend class processlistwindow;
private:
    void loadsettings();
    void savesettings();

    void doautoattach();

};
