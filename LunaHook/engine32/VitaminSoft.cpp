#include"VitaminSoft.h"
 
namespace{
  bool _1(){
    //どうして？いじってプリンセスFinalRoad～もう！またこんなところで3～
    auto entry=Util::FindImportEntry(processStartAddress,(DWORD)ExtTextOutA);
    if(entry==0)return false;
    BYTE bytes[]={0xFF,0x15,XX4};
    memcpy(bytes+2,&entry,4);
    bool ok=false;
    for(auto addr:Util::SearchMemory(bytes, sizeof(bytes), PAGE_EXECUTE, processStartAddress, processStopAddress)){
      addr = MemDbg::findEnclosingAlignedFunction(addr);
      if (!addr) continue;
      HookParam hp;
      hp.address = addr;
      hp.offset=get_stack(3);  
      hp.type = DATA_INDIRECT;
			hp.index = 0;
      ok|=NewHook(hp, "VitaminSoft");
    }
    return ok;
  }
  bool _2(){
    //ねとって女神
    //ねとって女神 NEO
    auto entry=Util::FindImportEntry(processStartAddress,(DWORD)TextOutA);
    if(entry==0)return false;
    BYTE bytes[]={0xFF,0x15,XX4};
    memcpy(bytes+2,&entry,4);
    bool ok=false;
    for(auto addr:Util::SearchMemory(bytes, sizeof(bytes), PAGE_EXECUTE, processStartAddress, processStopAddress)){
      addr = MemDbg::findEnclosingAlignedFunction(addr);
      if (!addr) continue;
      HookParam hp;
      hp.address = addr;
      hp.offset=get_stack(1);  
      hp.type = USING_STRING; 
      ok|=NewHook(hp, "VitaminSoft");
    }
    return ok;
  }
} 

bool VitaminSoft::attach_function(){
   
  return _2()||_1();
}