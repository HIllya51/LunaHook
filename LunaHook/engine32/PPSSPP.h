#include"engine.h"

class PPSSPP:public ENGINE{
    public:
    PPSSPP(){
        
        check_by=CHECK_BY::FILE;
        check_by_target=L"PPSSPP*.exe";
        is_engine_certain=false;
    };
    bool attach_function();
     
};