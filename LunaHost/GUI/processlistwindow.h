#ifndef LUNA_PROCLIST_WIN_H
#define LUNA_PROCLIST_WIN_H
#include"window.h"
#include"controls.h"
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

#endif