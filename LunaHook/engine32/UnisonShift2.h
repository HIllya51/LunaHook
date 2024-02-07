#include"engine.h"

class UnisonShift2:public ENGINE{
    public:
    UnisonShift2(){
        
        check_by=CHECK_BY::FILE_ALL;
        check_by_target=check_by_list{L"PIC.*",L"TP.*",L"GR.*",L"BGM.*"}; 
    };
     bool attach_function();
};