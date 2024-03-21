#include"engine.h"

class yuzusuyu:public ENGINE{
    public:
    yuzusuyu(){
        
        is_engine_certain=false;
        check_by=CHECK_BY::CUSTOM;
        check_by_target=[](){
             
            return (wcscmp(processName_lower, L"suyu.exe")==0 || wcscmp(processName_lower, L"yuzu.exe")==0);
        };
    }; 
    bool attach_function(); 
};
 