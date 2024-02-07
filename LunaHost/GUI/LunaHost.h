#include"window.h"
#include"controls.h"
#include"processlistwindow.h"
#include"textthread.h"
class LunaHost:public mainwindow{
        
    int64_t currentselect=0;
    std::map<int64_t,std::vector<std::wstring>>savetext;
    std::vector<int>attachedprocess;
    bool check_toclipboard=false; 
    std::map<int,TextThread*>savehooks;
    std::mutex settextmutex;
    textedit* g_hEdit_userhook;
    button* g_hButton_insert;
    listbox* g_hListBox_listtext;
    textedit* g_showtexts;
    button* g_selectprocessbutton;
    textedit* g_timeout;
    textedit* g_codepage;
    checkbox* g_check_clipboard; 
    void toclipboard(std::wstring& sentence);
    processlistwindow *_processlistwindow=0;
public:
    void on_size(int w,int h);
    void on_close();
    LunaHost();
};