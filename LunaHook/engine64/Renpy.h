#include"engine.h"

class Renpy:public ENGINE{
    public:
    Renpy(){
        
        check_by=CHECK_BY::CUSTOM; 
        check_by_target=[](){
            //Renpy - sample game https://vndb.org/v19843
            return Util::CheckFile(L"*.py")|| GetModuleHandleW(L"librenpython.dll");
        };
    }; 
    bool attach_function(); 
};
