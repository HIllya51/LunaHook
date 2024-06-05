

class Jellyfish:public ENGINE{
    public:
    Jellyfish(){
        
        is_engine_certain=false;
        check_by=CHECK_BY::FILE_ALL;
        check_by_target=check_by_list{L"ism.dll"};//,L"data.isa"};
    };
     bool attach_function();
};
 