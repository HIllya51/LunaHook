#include<Windows.h>
#include<QApplication>
#include<QFont>
#include<QDir>
#include<thread>
#include<mutex>
#include<Shlwapi.h>
void GetCommandLineArguments(int& argc, char**& argv)
{
    LPWSTR* wideArgv = NULL;
    int wideArgc = 0;

    LPWSTR commandLine = GetCommandLine();

    wideArgv = CommandLineToArgvW(commandLine, &wideArgc);

    if (wideArgv != NULL)
    {
        argv = new char*[wideArgc];
        argc = wideArgc;

        for (int i = 0; i < wideArgc; i++)
        {
            int length = WideCharToMultiByte(CP_UTF8, 0, wideArgv[i], -1, NULL, 0, NULL, NULL);
            argv[i] = new char[length];
            WideCharToMultiByte(CP_UTF8, 0, wideArgv[i], -1, argv[i], length, NULL, NULL);
        }
        LocalFree(wideArgv);
    }
}
extern "C" __declspec(dllexport) HMODULE* QtLoadLibrary(LPCWSTR* dlls,int num){
    auto hdlls=new HMODULE[num];
    auto mutex=CreateSemaphoreW(0,0,1,0);
    std::thread([=](){
        int argc;
        char** argv;
        GetCommandLineArguments(argc, argv);
        QApplication app(argc, nullptr);
        app.setFont(QFont("MS Shell Dlg 2", 10));
        for(int i=0;i<num;i++)
            hdlls[i]=LoadLibraryW(dlls[i]);
        ReleaseSemaphore(mutex,1,0);
        app.exec();
        
    }).detach();
    WaitForSingleObject(mutex,INFINITE);
    return hdlls;
}