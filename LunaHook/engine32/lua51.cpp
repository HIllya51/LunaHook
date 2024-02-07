#include"lua51.h"
 
bool lua51::attach_function() {  
     //[180330][TOUCHABLE] 想聖天使クロスエモーション外伝5 (認証回避済)
    auto hlua51=GetModuleHandleW(L"lua5.1.dll");
    if(hlua51==0)return false;
    auto lua_pushstring=GetProcAddress(hlua51,"lua_pushstring");
    if(lua_pushstring==0)return false;
    HookParam hp;
		hp.address =(uintptr_t) lua_pushstring;
		hp.offset=get_stack(2);
		hp.type = CODEC_UTF8 | USING_STRING; 
    
    hp.filter_fun=all_ascii_Filter;
    return NewHook(hp,"lua51");
} 