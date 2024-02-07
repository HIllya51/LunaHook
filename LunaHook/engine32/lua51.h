#include"engine.h"

class lua51:public ENGINE{
    public:
    lua51(){
        
        check_by=CHECK_BY::FILE;
        check_by_target=L"lua5.1.dll";
        is_engine_certain=false;
        dontstop=true;
    };
     bool attach_function();
};