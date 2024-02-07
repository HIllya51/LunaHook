#include"Sprite.h"
 
bool Sprite::attach_function() {
  //恋と選挙とチョコレート
  auto m=GetModuleHandle(L"dirapi.dll");
  auto [minAddress, maxAddress] = Util::QueryModuleLimits(m);
  const BYTE bytes[] = {
    0x83,0xF8,0x40,
    0x74,XX,
    0x83,0xF8,0x43,
    0x74,XX,
    0x83,XX,0xFF,
    0xEB,XX,
    0x8D,0x45,0xF8,
    XX,
    XX,
    XX,
    //+20
    0xE8,XX4,
    0x89,0x45,0xF0,
    0x8D,0x45,0xF4,
    0x50,
    XX,
    0xE8,XX4
	};
  auto addr = MemDbg::findBytes(bytes, sizeof(bytes), minAddress, maxAddress);
  if(addr==0)return false; 
  if(((*(int*)(addr+22))+addr+22)!=((*(int*)(addr+35))+addr+35))return false;
  HookParam hp;
  hp.address = addr+sizeof(bytes);
  hp.offset=get_reg(regs::eax);
  hp.type = USING_STRING;  
  return NewHook(hp, "Sprite");
} 