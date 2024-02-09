#include<Windows.h>
#include<QApplication>
#include<QFont>
#include<QDir>
#include<thread>
#include<mutex>
#include<Shlwapi.h>
#include<filesystem>

extern "C" __declspec(dllexport) HMODULE* QtLoadLibrary(LPCWSTR* dlls,int num){
    auto hdlls=new HMODULE[num];
    auto mutex=CreateSemaphoreW(0,0,1,0);
    std::thread([=](){
        int _=0;
        QApplication::addLibraryPath(QString::fromStdWString(std::filesystem::path(dlls[0]).parent_path()));
        QApplication app(_, nullptr);
        app.setFont(QFont("MS Shell Dlg 2", 10));
        for(int i=0;i<num;i++)
            hdlls[i]=LoadLibraryW(dlls[i]);
        ReleaseSemaphore(mutex,1,0);
        app.exec();
        
    }).detach();
    WaitForSingleObject(mutex,INFINITE);
    return hdlls;
}