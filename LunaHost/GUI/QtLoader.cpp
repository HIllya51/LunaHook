#include<Windows.h>
#include<thread>
#include<mutex>
#include<Shlwapi.h>
#include<filesystem>
 

#ifndef _WIN64
#define THISCALL __thiscall
#define _CDECL __cdecl
#define fnQString_fromStdWString "?fromStdWString@QString@@SA?AV1@ABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z"
#define fnQCoreApplication_addLibraryPath "?addLibraryPath@QCoreApplication@@SAXABVQString@@@Z"
#define fnQString_dtor "??1QString@@QAE@XZ"
#define fnQApplication_ctor "??0QApplication@@QAE@AAHPAPADH@Z"
#define fnQFont_ctor "??0QFont@@QAE@ABVQString@@HH_N@Z"
#define fnQApplication_setFont "?setFont@QApplication@@SAXABVQFont@@PBD@Z"
#define fnQFont_dtor "??1QFont@@QAE@XZ"
#define fnQApplication_exec "?exec@QApplication@@SAHXZ"
#define fnQApplication_dtor "??1QApplication@@UAE@XZ"
#else
#define THISCALL __fastcall
#define _CDECL __fastcall
#define fnQString_fromStdWString "?fromStdWString@QString@@SA?AV1@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z"
#define fnQCoreApplication_addLibraryPath "?addLibraryPath@QCoreApplication@@SAXAEBVQString@@@Z"
#define fnQString_dtor "??1QString@@QEAA@XZ"
#define fnQApplication_ctor "??0QApplication@@QEAA@AEAHPEAPEADH@Z"
#define fnQFont_ctor "??0QFont@@QEAA@AEBVQString@@HH_N@Z"
#define fnQApplication_setFont "?setFont@QApplication@@SAXAEBVQFont@@PEBD@Z"
#define fnQFont_dtor "??1QFont@@QEAA@XZ"
#define fnQApplication_exec "?exec@QApplication@@SAHXZ"
#define fnQApplication_dtor "??1QApplication@@UEAA@XZ"
#endif

extern "C" __declspec(dllexport) std::vector<HMODULE>* QtLoadLibrary(std::vector<std::wstring>* dlls){
 
    auto Qt5Widgets=LoadLibrary(L"Qt5Widgets.dll");
    auto Qt5Gui=LoadLibrary(L"Qt5Gui.dll");
    auto Qt5Core=LoadLibrary(L"Qt5Core.dll");
    if(Qt5Core==0||Qt5Gui==0||Qt5Widgets==0)return new std::vector<HMODULE>{};
    
    auto QString_fromStdWString=GetProcAddress(Qt5Core,fnQString_fromStdWString);
    auto QCoreApplication_addLibraryPath=GetProcAddress(Qt5Core,fnQCoreApplication_addLibraryPath);
    auto QString_dtor=GetProcAddress(Qt5Core,fnQString_dtor);
    auto QApplication_ctor=GetProcAddress(Qt5Widgets,fnQApplication_ctor);
    auto QFont_ctor=GetProcAddress(Qt5Gui,fnQFont_ctor);
    auto QFont_dtor=GetProcAddress(Qt5Gui,fnQFont_dtor);
    auto QApplication_setFont=GetProcAddress(Qt5Widgets,fnQApplication_setFont);
    auto QApplication_exec=GetProcAddress(Qt5Widgets,fnQApplication_exec);
    auto QApplication_dtor=GetProcAddress(Qt5Widgets,fnQApplication_dtor);
    
    if(QString_fromStdWString==0||QCoreApplication_addLibraryPath==0||QString_dtor==0||QApplication_ctor==0||QFont_ctor==0||QFont_dtor==0||QApplication_setFont==0||QApplication_exec==0||QApplication_dtor==0)return {};
    auto hdlls=new std::vector<HMODULE>;
    auto mutex=CreateSemaphoreW(0,0,1,0);
    std::thread([=](){
        static void* qapp;  //必须static
        void* qstring;
        void* qfont;
        for(int i=0;i<dlls->size();i++){
            auto dirname=std::filesystem::path(dlls->at(i)).parent_path().wstring();
            ((void* (_CDECL*)(void*,void*))QString_fromStdWString)(&qstring,&dirname);
            ((void(_CDECL *)(void*))QCoreApplication_addLibraryPath)(&qstring);
            ((void(THISCALL*)(void*))QString_dtor)(&qstring);
            //QCoreApplication_addLibraryPath(QString_fromStdWString(std::filesystem::path(collectQtplugs[i]).parent_path()));
        }
        
        int _=0;
        ((void*(THISCALL*)(void*,int*,char**,int))QApplication_ctor)(&qapp,&_,0,331266);
        
        std::wstring font=L"MS Shell Dlg 2";
        ((void* (_CDECL*)(void*,void*))QString_fromStdWString)(&qstring,&font);
        ((void*(THISCALL*)(void*,void*,int,int,bool))QFont_ctor)(&qfont,&qstring,10,-1,0);
        ((void(_CDECL*)(void*,void*))QApplication_setFont)(&qfont,0);
        ((void(THISCALL*)(void*))QFont_dtor)(&qfont);
        ((void(THISCALL*)(void*))QString_dtor)(&qstring);
        for(int i=0;i<dlls->size();i++){
            hdlls->push_back(LoadLibrary(dlls->at(i).c_str()));
        }
        ReleaseSemaphore(mutex,1,0);
        
        ((void(*)())QApplication_exec)();
        
        ((void(THISCALL*)(void*))QApplication_dtor)(&qapp);
        
        
    }).detach();
    WaitForSingleObject(mutex,INFINITE);
    
    return hdlls;
}