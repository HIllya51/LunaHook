#include"engine.h"

class LightVN:public ENGINE{
    public:
    LightVN(){
        
        check_by=CHECK_BY::FILE_ANY; 
        is_engine_certain=false;
        check_by_target=check_by_list{L"Data/Scripts/title.txt",L"Data/data*.vndat"};
    }; 
    bool attach_function(); 
};
 