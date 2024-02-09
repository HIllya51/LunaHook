#ifndef LUNA_BASE_CONTROLS_H
#define LUNA_BASE_CONTROLS_H
#include"window.h"
class control:public basewindow{
    public:
    mainwindow* parent;
    control(mainwindow*);
    virtual void dispatch(WPARAM);
    
    std::function<HMENU()>oncontextmenu=[](){return (HMENU)nullptr;};
    std::function<void(WPARAM)>oncontextmenucallback=[](WPARAM){};
};

class button:public control{
public:
    button(mainwindow*,LPCWSTR,int,int,int,int,DWORD=BS_PUSHBUTTON);
    void dispatch(WPARAM);
    std::function<void()> onclick=[](){};
};
class checkbox:public button{
public:
    checkbox(mainwindow*,LPCWSTR,int,int,int,int);
    bool ischecked();
    void setcheck(bool);
};
class textedit:public control{
public:
    textedit(mainwindow*,LPCWSTR,int,int,int,int,DWORD stype=0);
    void dispatch(WPARAM);
    std::function<void(const std::wstring&)> ontextchange=[&](const std::wstring &text){};
    void appendtext(const std::wstring&);
    void scrolltoend();
};
class spinbox:public control{
    HWND hUpDown;
    int minv,maxv;
    public:
    void dispatch(WPARAM);
    spinbox(mainwindow*,LPCWSTR,int,int,int,int,DWORD stype=0);
    void setminmax(int,int);
    std::pair<int,int>getminmax();
    void setcurr(int);
    std::function<void(int)> onvaluechange=[&](int){};
};
class label:public control{
public:
    label(mainwindow*,LPCWSTR,int,int,int,int);
};

class listbox:public control{
public:
    listbox(mainwindow*,int,int,int,int);
    void dispatch(WPARAM);
    int currentidx();
    std::wstring text(int);
    std::function<void(int)> oncurrentchange=[](int){};
    void clear();
    int additem(LPCWSTR);
    void deleteitem(int);
    void setdata(int,LONG_PTR);
    int insertitem(int,LPCWSTR);
    LONG_PTR getdata(int);
    int count();
};
#endif