#include"Fizz.h"
 
bool Fizz::attach_function() {
  //char __thiscall sub_59AA90(char *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8, char a9)
  //HB8@59AA90
  //https://vndb.org/v1380
  //さくらテイル

    const BYTE bytes[] = {
    0x55,0x8b,0xec,
    0x6a,0xff,
    0x68,XX4,
    0x64,0xa1,0,0,0,0,
    0x50,
    0x81,0xec,XX2,0,0,
    0xa1,XX4,
    0x33,0xc5,
    0x89,0x45,0xf0,
    0x50,
    0x8d,0x45,0xf4,
    0x64,0xa3,0,0,0,0,
    0x89,0x4d,XX,
    0xc7,0x45,XX,0,0,0,0,
    0xc7,0x45,XX,0,0,0,0,
    0x8d,0x4d,XX,
    0xe8,XX4,

  }; 
  ULONG addr = MemDbg::findBytes(bytes, sizeof(bytes), processStartAddress, processStopAddress);
  if (!addr)  return false; 
 
  HookParam hp;
  hp.address = addr;
  hp.offset=get_stack(2);
  hp.type = USING_CHAR; 
  return NewHook(hp, "Fizz");
} 