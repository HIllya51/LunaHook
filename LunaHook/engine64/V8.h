#include"engine.h"

class V8:public ENGINE{
    public:
    V8(){
        
        check_by=CHECK_BY::ALL_TRUE; 
    };
     bool attach_function(); 
};

