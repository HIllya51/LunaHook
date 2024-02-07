#include"engine.h"

class V8:public ENGINE{
    public:
    V8(){
        
        check_by=CHECK_BY::CUSTOM;
        is_engine_certain=false;
        
  // Artikash 7/16/2018: Uses node/libuv: likely v8 - sample game https://vndb.org/v22975
  //if (GetProcAddress(GetModuleHandleW(nullptr), "uv_uptime") || GetModuleHandleW(L"node.dll"))
  //{
	 // InsertV8Hook();
	 // return true;
  //}
        check_by_target=[this](){ 
            for (HMODULE module : { (HMODULE)processStartAddress, GetModuleHandleW(L"node.dll"), GetModuleHandleW(L"nw.dll") })
                if (GetProcAddress(module, "?Write@String@v8@@QBEHPAGHHH@Z")){
                pmodule=module;
                return true;
                } 
            
            return false; 
  
        };
    };
     bool attach_function();
     private:
     HMODULE pmodule;
};

