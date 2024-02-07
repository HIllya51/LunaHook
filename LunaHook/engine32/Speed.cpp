#include"Speed.h"
 
bool Speed::attach_function() {
  //	藍色ノ狂詩曲～Deep Blue Rhapsody～
  auto entry=Util::FindImportEntry(processStartAddress,(DWORD)DrawTextA);
  if(entry==0)return false;
  BYTE bytes2[]={0x8b,0x35,XX4}; //mov     esi, ds:DrawTextA
  memcpy(bytes2+2,&entry,4);    
  auto addr = MemDbg::findBytes(bytes2, sizeof(bytes2), processStartAddress, processStopAddress);
  if (addr == 0)return false;
  BYTE sig1[]={ 0x68,0x00,0x04,0x00,0x00  };
  BYTE sig2[]={ 0xFF,0xD6 };
  BYTE sig3[]={ 0x68,0x00,0x01,0x00,0x00  };
  BYTE sig4[]={ 0xFF,0xD6  };
  for(auto p:std::vector<std::pair<BYTE*,int>>{{sig1,sizeof(sig1)},{sig2,sizeof(sig2)},{sig3,sizeof(sig3)},{sig4,sizeof(sig4)}}){
    addr=MemDbg::findBytes(p.first, p.second, addr, addr+0x40);
    if(addr==0)return false;
  }
  addr = MemDbg::findEnclosingAlignedFunction(addr);
  if (addr == 0)return false;
  HookParam hp;
    hp.address = addr;
    hp.offset=get_stack(1); 
    hp.type = CODEC_ANSI_BE ;
    return NewHook(hp, "Speed"); 
} 