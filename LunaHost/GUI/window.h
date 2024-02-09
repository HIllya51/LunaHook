#ifndef LUNA_BASE_WINDOW_H
#define LUNA_BASE_WINDOW_H
class control;
class basewindow{
public:
    HFONT hfont=0;
    HWND winId;
    void setgeo(int,int,int,int);
    RECT getgeo();
    std::wstring text();
    void settext(const std::wstring&);
    void setfont(int,LPCWSTR fn=0);
    operator HWND(){return winId;}
};
class mainwindow:public basewindow{
public:
    std::vector<control*>controls;
    mainwindow* parent;
    HWND lastcontexthwnd;
    virtual void on_show();
    virtual void on_close();
    virtual void on_size(int w,int h);
    mainwindow(mainwindow* _parent=0);
    LRESULT wndproc(UINT message, WPARAM wParam, LPARAM lParam);
    static void run();
    void show();
    void close();
    void setcentral(int,int);
    std::pair<int,int>calculateXY(int w,int h);
};
#endif