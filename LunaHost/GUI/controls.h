#ifndef LUNA_BASE_CONTROLS_H
#define LUNA_BASE_CONTROLS_H
#include"window.h"
#include <CommCtrl.h>
class control:public basewindow{
    public:
    mainwindow* parent;
    control(mainwindow*);
    virtual void dispatch(WPARAM);
    virtual void dispatch_2(WPARAM wParam, LPARAM lParam);
    std::function<HMENU()>oncontextmenu=[](){return (HMENU)nullptr;};
    std::function<void(WPARAM)>oncontextmenucallback=[](WPARAM){};
};

class button:public control{
public:
    button(mainwindow*,const std::wstring&,int,int,int,int,DWORD=BS_PUSHBUTTON);
    void dispatch(WPARAM);
    std::function<void()> onclick=[](){};
};
class checkbox:public button{
public:
    checkbox(mainwindow*,const std::wstring&,int,int,int,int);
    bool ischecked();
    void setcheck(bool);
};
class textedit:public control{
public:
    textedit(mainwindow*,const std::wstring&,int,int,int,int,DWORD stype=0);
    void dispatch(WPARAM);
    std::function<void(const std::wstring&)> ontextchange=[&](const std::wstring &text){};
    void appendtext(const std::wstring&);
    void scrolltoend();
    void setreadonly(bool);
};
class spinbox:public control{
    HWND hUpDown;
    int minv,maxv;
    public:
    void dispatch(WPARAM);
    spinbox(mainwindow*,const std::wstring&,int,int,int,int,DWORD stype=0);
    void setminmax(int,int);
    std::pair<int,int>getminmax();
    void setcurr(int);
    std::function<void(int)> onvaluechange=[&](int){};
};
class label:public control{
public:
    label(mainwindow*,const std::wstring&,int,int,int,int);
};

class listbox:public control{
public:
    listbox(mainwindow*,int,int,int,int);
    void dispatch(WPARAM);
    int currentidx();
    std::wstring text(int);
    std::function<void(int)> oncurrentchange=[](int){};
    void clear();
    int additem(const std::wstring&);
    void deleteitem(int);
    void setdata(int,LONG_PTR);
    int insertitem(int,const std::wstring&);
    LONG_PTR getdata(int);
    int count();
};
class listview:public control{
    int headernum=1;
    HIMAGELIST hImageList;
public:
    listview(mainwindow*,int,int,int,int);
    int insertitem(int,const std::wstring&,HICON hicon);
    int insertcol(int,const std::wstring& );
    void clear();
    int count();
    std::function<void(int)> oncurrentchange=[](int){};
    std::wstring text(int,int=0);
    void setheader(const std::vector<std::wstring>&);
    void autosize();
    int additem(const std::wstring&,HICON hicon);
    void dispatch_2(WPARAM wParam, LPARAM lParam);
};
#endif