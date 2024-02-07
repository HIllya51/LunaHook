#include"Tarte.h"
 
bool Tarte::attach_function() {   
  //ひなたぼっこ
  //ひなたると～ひなたぼっこファンディスク～
  //スクールぱにっく！ 
  //こいじばし  https://vndb.org/v4247
  auto entry=Util::FindImportEntry(processStartAddress,(DWORD)GetGlyphOutlineA);
  if(entry==0)return false;
  BYTE bytes[]={0xFF,0x15,XX4};
  memcpy(bytes+2,&entry,4); 
  for(auto addr:Util::SearchMemory(bytes, sizeof(bytes), PAGE_EXECUTE, processStartAddress, processStopAddress)){ 
    addr = MemDbg::findEnclosingAlignedFunction(addr);
    if (!addr) continue; 
    auto xrefs=findxref_reverse_checkcallop(addr,addr-0x1000,addr+0x1000,0xe8);
    for(auto addrx:xrefs){
      auto addrx1 = MemDbg::findEnclosingAlignedFunction(addrx);
      if (!addrx1) continue; 
      BYTE __[]={0x3C,0x81};
      auto _ = MemDbg::findBytes(__, 2, addrx1, addrx);
      if(_==0)continue;
      HookParam hp;
      hp.address = addrx1;
      hp.offset=get_stack(2);   
      hp.type = CODEC_ANSI_BE;
      auto succ=NewHook(hp, "Tarte");

      auto xrefs1=findxref_reverse_checkcallop(addrx1,addrx1-0x1000,addrx1+0x1000,0xe8);
      for(auto addrx11:xrefs1){
        auto addrx12 = MemDbg::findEnclosingAlignedFunction(addrx11);
        if(addrx11-addrx12<0x30){
          HookParam hp;
          hp.address = addrx12;
          hp.offset=get_stack(5);   
          hp.type = CODEC_ANSI_BE;
          succ|=NewHook(hp, "Tarte"); 
        }
      }
      return succ;
    } 
  }
  return false;
} 