#include"solfasys.h"

bool solfasys::attach_function() {    
  auto addr=MemDbg::findCallerAddressAfterInt3((DWORD)GetGlyphOutlineA,processStartAddress,processStopAddress);
  ConsoleOutput("%p",addr);
  if(!addr)return false;
  addr=MemDbg::findShortJumpAddress(addr,processStartAddress,processStopAddress);
  ConsoleOutput("%p",addr);
  if(!addr)return false;
  addr=MemDbg::findEnclosingAlignedFunction(addr,0x10);//actually only 2
  ConsoleOutput("%p",addr);
  if(!addr)return false;
  auto addrs=findxref_reverse_checkcallop(addr,processStartAddress,processStopAddress,0xe8);
  if(addrs.size()!=2)return false;
  addr=addrs[0];
  ConsoleOutput("%p",addr);
  addr=MemDbg::findEnclosingAlignedFunction(addr);
  ConsoleOutput("%p",addr);
  if(!addr)return false;
  HookParam hp;
  hp.address=addr;
  hp.type=CODEC_ANSI_BE|USING_CHAR;
  hp.offset=get_stack(1);
  return NewHook(hp,"solfasys");
} 