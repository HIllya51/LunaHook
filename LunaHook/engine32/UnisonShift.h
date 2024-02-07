#include"engine.h"

class UnisonShift:public ENGINE{
    public:
    UnisonShift(){
        
        check_by=CHECK_BY::FILE;
        check_by_target=L"*.dat"; 
         is_engine_certain=false;
    };
     bool attach_function();
};