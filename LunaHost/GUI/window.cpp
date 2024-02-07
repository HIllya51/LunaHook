#include"window.h"
#include"controls.h"
#include"Lang/Lang.h"
void SetDefaultFont(HWND hwnd)
{    
    EnumChildWindows(hwnd, [](HWND hwndChild, LPARAM lParam)
    {
        static auto fnt=CreateFont(28, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            ANSI_CHARSET, OUT_DEFAULT_PRECIS, 
                            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                            DEFAULT_PITCH | FF_DONTCARE, DefaultFont);
        SendMessage(hwndChild, WM_SETFONT, (WPARAM)fnt, TRUE);
        return TRUE;
    }, 0);
}

std::wstring basewindow::text(){
    int textLength = GetWindowTextLength(winId);
    std::vector<wchar_t> buffer(textLength + 1);
    GetWindowText(winId, buffer.data(), buffer.size());
    return buffer.data();
}
void basewindow::settext(const std::wstring& text){
    SetWindowText(winId,text.c_str());
}

void basewindow::setgeo(int x,int y,int w,int h){
    MoveWindow(winId,x,y,w,h,TRUE);
}
RECT basewindow::getgeo(){
    RECT rect;
    GetWindowRect(winId,&rect);
    return rect;
}

LRESULT mainwindow::wndproc(UINT message, WPARAM wParam, LPARAM lParam){
    switch (message)
    {
        case WM_SHOWWINDOW:
        {
            on_show();
            SetDefaultFont(winId);
            break;
        }
        case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            on_size(width,height);
            break;
        } 
        case WM_COMMAND:
        {
            if(lParam==0){
                for(auto ctl:controls){
                    if(lastcontexthwnd==ctl->winId){
                        ctl->oncontextmenucallback(wParam);break;
                    }
                }
            }
            else
            for(auto ctl:controls){
                if((HWND)lParam==ctl->winId){
                    ctl->dispatch(wParam);break;
                }
            }
            break;
        }
        case WM_CONTEXTMENU:
        {
            bool succ=false;lastcontexthwnd=0;
            for(auto ctl:controls){
                if((HWND)wParam==ctl->winId){
                    auto hm=ctl->oncontextmenu();
                    if(hm){
                        int xPos = LOWORD(lParam);
                        int yPos = HIWORD(lParam);
                        TrackPopupMenu(hm, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
                        xPos, yPos, 0, winId, NULL);
                        lastcontexthwnd=ctl->winId;
                        succ=true;
                    }
                    break;
                }
            }
            if(succ==false)return DefWindowProc(winId, message, wParam, lParam);
            break;
        }
        case WM_CLOSE:
        {
            on_close();
            if(parent==0)PostQuitMessage(0);
            else ShowWindow(winId,SW_HIDE);
            break;
        }
        default:
            return DefWindowProc(winId, message, wParam, lParam);
    }
    
    return 0;
}

mainwindow::mainwindow(mainwindow* _parent){
    const wchar_t CLASS_NAME[] = L"LunaHostWindow"; 
     
    WNDCLASS wc = {};
    wc.lpfnWndProc = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        mainwindow* _window = reinterpret_cast<mainwindow *>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if ((!_window)||(_window->winId!=hWnd))  return DefWindowProc(hWnd, message, wParam, lParam);
        return _window->wndproc(message,wParam,lParam);
    };
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW );
    wc.hIcon=LoadIconW(GetModuleHandle(0),L"IDI_ICON1");
    static auto _=RegisterClass(&wc); 
    HWND hWnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,CLASS_NAME,CLASS_NAME,WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        _parent?_parent->winId:NULL,NULL,GetModuleHandle(0),this
    ); 
    winId = hWnd;
    parent=_parent;
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)this);
}
void mainwindow::show(){
    ShowWindow(winId, SW_SHOW);
    SetForegroundWindow(winId);
}
void mainwindow::close(){
    ShowWindow(winId, SW_HIDE);
}
void mainwindow::run(){
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void mainwindow::on_close(){}
void mainwindow::on_show(){}
void mainwindow::on_size(int w,int h){}