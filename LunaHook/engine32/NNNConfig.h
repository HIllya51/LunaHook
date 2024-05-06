

class NNNConfig:public ENGINE{
    public:
    NNNConfig(){
        
        check_by=CHECK_BY::FILE;
        check_by_target=L"nnnConfig2.exe"; 
        is_engine_certain=false;
    };
     bool attach_function();
};