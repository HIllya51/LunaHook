#include"Tomato.h"
bool Tomato::attach_function() {
  //姫武者
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
    hp.offset=get_reg(regs::edx);
    hp.type  = DATA_INDIRECT; 
			hp.index = 0;
    ok|=NewHook(hp, "Tomato");
  }
  return ok;
}
 