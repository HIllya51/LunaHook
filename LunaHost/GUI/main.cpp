#include"LunaHost.h"
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
//int main()
{
    SetProcessDPIAware();
    LunaHost _lunahost;
    _lunahost.show();
    mainwindow::run();
}
int main(){
    SetProcessDPIAware();
    LunaHost _lunahost;
    _lunahost.show();
    mainwindow::run();
}