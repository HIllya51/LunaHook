#include"UnisonShift.h"

bool InsertUnisonShiftHook() {
  BYTE bytes[] = {
    0x83,0xec,0x14,
    0x8b,0x44,0x24,0x10,
    0x53,
    0x55,
    0x8b,0x6c,0x24,0x20

  }; 
  auto addr1 = MemDbg::findBytes(bytes, sizeof(bytes), processStartAddress, processStopAddress); 
  if (addr1 == 0) return false;
  ConsoleOutput("UnisonShift %p", addr1);
  HookParam hp;
  hp.address = addr1;
  hp.offset=get_stack(3);
  return NewHook(hp, "UnisonShift");
}
bool UnisonShift::attach_function() {   
    return InsertUnisonShiftHook();
} 