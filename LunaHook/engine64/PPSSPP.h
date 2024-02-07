#include"engine.h"

class PPSSPP:public ENGINE{
    public:
    PPSSPP(){
        
        check_by=CHECK_BY::FILE; 
        is_engine_certain=false;
        check_by_target=L"PPSSPP*.exe";
    }; 
    bool attach_function(); 
};
