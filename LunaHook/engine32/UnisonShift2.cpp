#include"UnisonShift2.h"

bool InsertUnisonShift2Hook() {
  BYTE bytes[] = {
      //80 FB A0                      cmp     bl, 0A0h
    0x80,0xfb,0xa0
  };
  auto addr1 = MemDbg::findBytes(bytes, sizeof(bytes), processStartAddress, processStopAddress);
  if (addr1 == 0)return false;
  ConsoleOutput("UnisonShift2 %p", addr1);
  BYTE start[] = { 0x83 ,0xEC ,0x08 };
  addr1 = reverseFindBytes(start, sizeof(start), addr1 - 0x100, addr1);
  if (addr1 == 0)return false;
  HookParam hp;
  hp.address = addr1;
  hp.offset=get_reg(regs::eax);
  hp.type = DATA_INDIRECT;
  hp.index = 0;
  return NewHook(hp, "UnisonShift2");
}
bool InsertUnisonShift3Hook() {
  
  BYTE bytes2[] = {
      0x80,0xF9,XX
  };
  auto addrs=Util::SearchMemory(bytes2,sizeof(bytes2),PAGE_EXECUTE, processStartAddress, processStopAddress);
  BYTE moveaxoffset[] = { 0xb8 ,XX,XX,XX, 0x00 };
  auto succ=false;
  for (auto addr : addrs) {
    ConsoleOutput("UnisonShift3 %p", addr);
    addr = (DWORD)((BYTE*)addr -5);
    int x = -1;
    for (int i = 0; i < 0x20; i++) {
      if (*((BYTE*)addr-i) == 0xb8 && *((BYTE*)(addr)+4-i) == 0) {
        x = i; break;
      }
    }
    if (x == -1)continue;
    ConsoleOutput("UnisonShift3 found %p", addr-x);
    addr = (DWORD)((BYTE*)addr + 1-x);
    auto raddr = *(int*)addr;
    ConsoleOutput("UnisonShift3 raddr %p", raddr);
    HookParam hp;
    hp.address = raddr;
    hp.type = DIRECT_READ;
    succ|=NewHook(hp, "UnisonShift3");
  }
  

  return succ;
}
		
bool UnisonShift2::attach_function() {   
    bool b1=InsertUnisonShift2Hook();
		bool b2=InsertUnisonShift3Hook();
    return  b1||b2;
} 