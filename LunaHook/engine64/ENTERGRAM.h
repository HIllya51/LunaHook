#include"engine.h"

class ENTERGRAM:public ENGINE{
    public:
    ENTERGRAM(){
        
        check_by=CHECK_BY::CUSTOM; 
        is_engine_certain=false;
        check_by_target=[](){
            return GetProcAddress(GetModuleHandleA(0),"agsCheckDriverVersion");
        };
    }; 
    bool attach_function(); 
};
 