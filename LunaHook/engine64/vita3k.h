#include"engine.h"

class vita3k:public ENGINE{
    public:
    vita3k(){
        
        check_by=CHECK_BY::FILE; 
        is_engine_certain=false;
        check_by_target=L"Vita3K.exe";
    }; 
    bool attach_function(); 
};
 