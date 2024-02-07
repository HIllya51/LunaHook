#include"Xbangbang.h"
 
bool Xbangbang::attach_function() {    
  //さわさわ絵にっき
  //さわさわ絵にっき２
  auto entry=Util::FindImportEntry(processStartAddress,(DWORD)GetTextExtentPoint32A);
  if(entry==0)return false;
  BYTE bytes[]={0xFF,0x15,XX4};
  memcpy(bytes+2,&entry,4);
  bool ok=false;
  for(auto addr:Util::SearchMemory(bytes, sizeof(bytes), PAGE_EXECUTE, processStartAddress, processStopAddress)){
    addr = MemDbg::findEnclosingAlignedFunction(addr);
    if (!addr) continue;
    HookParam hp;
    hp.address = addr;
    hp.offset=get_stack(2);  
    hp.type=USING_STRING;
    ok|=NewHook(hp, "Xbangbang");
  }
  return ok;
} 