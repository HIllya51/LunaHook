#include"engine.h"

class Silkys:public ENGINE{
    public:
    Silkys(){
        
        check_by=CHECK_BY::FILE_ALL;
        check_by_target=check_by_list{L"data.arc",L"effect.arc",L"Script.arc"};
        /// Almost the same as Silkys except mes.arc is replaced by Script.arc
    }; 
     bool attach_function(); 
};
class SilkysOld:public ENGINE{
    public:
    SilkysOld(){
        
        check_by=CHECK_BY::FILE_ALL;
        check_by_target=check_by_list{L"bgm.AWF",L"effect.AWF",L"gcc.ARC",L"mes.ARC",L"sequence.ARC"};
        /// Almost the same as Silkys except mes.arc is replaced by Script.arc
    }; 
     bool attach_function(); 
};
