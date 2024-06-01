

class Fizz:public ENGINE{
    public:
    Fizz(){
        
        check_by=CHECK_BY::FILE_ALL;
        check_by_target=check_by_list{L"data.gsp",L"Image*.gsp",L"se.gsp",L"bgm*.gsp",L"voice/*.gsp"};
    };
     bool attach_function();
};