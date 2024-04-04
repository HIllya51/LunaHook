#include"GSX.h"


bool GSX::attach_function() { 
  //http://www.mirai-soft.com/products/persona/download.html
    ULONG addr = MemDbg::findCallerAddress((ULONG)::GetCharWidth32W, 0xec8b55, processStartAddress, processStopAddress);
    if(addr==0)return false;
    HookParam hp;
    hp.address=addr;
    hp.type=USING_CHAR|CODEC_UTF16|DATA_INDIRECT;
    hp.offset=get_stack(4);
    return NewHook(hp,"GSX");
} 