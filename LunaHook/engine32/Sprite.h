#include"engine.h"

class Sprite:public ENGINE{public:
    Sprite(){
        is_engine_certain=false;
        check_by=CHECK_BY::CUSTOM;
        check_by_target=[](){
            return Util::CheckFile(L"*.cct");
        };
    };
     bool attach_function();
};