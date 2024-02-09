#include"controls.h"
#include"window.h"
#include <CommCtrl.h>

control::control(mainwindow*_parent){
    parent=_parent;
    parent->controls.push_back(this);
}
void control::dispatch(WPARAM){}

button::button(mainwindow* parent,LPCWSTR text,int x,int y,int w,int h,DWORD style):control(parent){
    winId=CreateWindowEx(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE |style ,
        x, y, w, h, parent->winId , NULL, NULL, NULL);
}
void button::dispatch(WPARAM wparam){
    if(wparam==BN_CLICKED){
        onclick();
    }
}
bool checkbox::ischecked(){
    int state = SendMessage(winId, BM_GETCHECK, 0, 0);
    return (state == BST_CHECKED);
}
checkbox::checkbox(mainwindow* parent,LPCWSTR text,int x,int y,int w,int h):button(parent,text,x,y,w,h,BS_AUTOCHECKBOX|BS_RIGHTBUTTON ){
}
void checkbox::setcheck(bool b){
    SendMessage(winId, BM_SETCHECK, (WPARAM)BST_CHECKED*b, 0);
}
spinbox::spinbox(mainwindow* parent,LPCWSTR text,int x,int y,int w,int h,DWORD stype):control(parent){
    winId=CreateWindowEx(0, L"EDIT", text, WS_CHILD | WS_VISIBLE  | WS_BORDER|ES_NUMBER ,
        x, y, w, h, parent->winId, NULL, NULL, NULL);

    hUpDown = CreateWindowEx(0, UPDOWN_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS ,
        0, 0, 0, 0,
        parent->winId, NULL, NULL, NULL);
    SendMessage(hUpDown, UDM_SETBUDDY, (WPARAM)winId, 0);
    std::tie(minv,maxv)= getminmax();
}
void spinbox::setcurr(int cur){
    SendMessage(hUpDown, UDM_SETPOS32, 0, cur);
}
void spinbox::dispatch(WPARAM wparam){
    if(HIWORD(wparam)==EN_CHANGE){
        bool ok=false;int value;
        try{
            value=std::stoi(text());
            ok=true;
        }
        catch(std::exception&){}
        if(ok){
            if(value>maxv){
                setcurr(maxv);
                value=maxv;
            }
            else if(value<minv){
                setcurr(minv);
                value=minv;
            }
            else{
                onvaluechange(value);
            }
        }
    }
}
std::pair<int,int>spinbox::getminmax(){
    int minValue, maxValue;
    SendMessage(hUpDown, UDM_GETRANGE32, (WPARAM)&minValue, (LPARAM)&maxValue);
    return {minValue,maxValue};
}
void spinbox::setminmax(int min,int max){
    SendMessage(hUpDown, UDM_SETRANGE32,min, max);
    std::tie(minv,maxv)= getminmax();
}
textedit::textedit(mainwindow* parent,LPCWSTR text,int x,int y,int w,int h,DWORD stype):control(parent){
    winId=CreateWindowEx(0, L"EDIT", text, WS_CHILD | WS_VISIBLE  | WS_BORDER|stype ,
        x, y, w, h, parent->winId, NULL, NULL, NULL);
}
void textedit::scrolltoend(){
    int textLength = GetWindowTextLength(winId);
    SendMessage(winId, EM_SETSEL, (WPARAM)textLength, (LPARAM)textLength);
    SendMessage(winId, EM_SCROLLCARET, 0, 0);
}
void textedit::appendtext(const std::wstring& text){
    auto _=std::wstring(L"\r\n")+text;
    SendMessage(winId, EM_REPLACESEL, 0, (LPARAM)_.c_str()); 
}

void textedit::dispatch(WPARAM wparam){
    if(HIWORD(wparam)==EN_CHANGE){
        ontextchange(text());
    }
}
label::label(mainwindow* parent,LPCWSTR text,int x,int y,int w,int h):control(parent){
    winId=CreateWindowEx(0, L"STATIC", text, WS_CHILD | WS_VISIBLE,
        x, y, w, h, parent->winId , NULL, NULL, NULL);
}

listbox::listbox(mainwindow* parent,int x,int y,int w,int h):control(parent){
    
    winId=CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY|LBS_NOINTEGRALHEIGHT,
        x, y, w, h, parent->winId , NULL, NULL, NULL);
}
void listbox::dispatch(WPARAM wparam){
    if(HIWORD(wparam) == LBN_SELCHANGE){
        auto idx=currentidx();
        if(idx!=-1)
            oncurrentchange(idx);
    }
}

int listbox::currentidx(){
    return SendMessage(winId, LB_GETCURSEL, 0, 0);
}
std::wstring listbox::text(int idx){
    int textLength = SendMessage(winId, LB_GETTEXTLEN, idx,0);
    std::vector<wchar_t> buffer(textLength + 1);
    SendMessage(winId, LB_GETTEXT, idx, (LPARAM)buffer.data());
    return buffer.data();
}
void listbox::clear(){
    SendMessage(winId, LB_RESETCONTENT, 0, 0);
}
int listbox::additem(LPCWSTR text){
    return SendMessage(winId, LB_ADDSTRING, 0, (LPARAM)text);
}
void listbox::deleteitem(int i){
    SendMessage(winId, LB_DELETESTRING, (WPARAM)i, (LPARAM)i);
}
void listbox::setdata(int idx,LONG_PTR data){
    SendMessage(winId, LB_SETITEMDATA, idx, (LPARAM)data); 
}
LONG_PTR listbox::getdata(int idx){
    return SendMessage(winId, LB_GETITEMDATA, idx, 0);
}
int listbox::count(){
    return SendMessage(winId, LB_GETCOUNT, 0, 0); 
}
int listbox::insertitem(int i,LPCWSTR t){
    return SendMessage(winId, LB_INSERTSTRING, i, (LPARAM)t);
}