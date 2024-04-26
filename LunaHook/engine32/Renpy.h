#include"engine.h"

class Renpy:public ENGINE{
    public:
    Renpy(){
        //使用lunatranslator启动游戏，会把cwd修改成exe所在目录，其中没有.py
        check_by=CHECK_BY::ALL_TRUE;
        // check_by=CHECK_BY::CUSTOM; 
        // check_by_target=[](){
        //     //Renpy - sample game https://vndb.org/v19843
        //     return Util::CheckFile(L"*.py")|| GetModuleHandleW(L"librenpython.dll");
        // };
    }; 
    bool attach_function(); 
};
