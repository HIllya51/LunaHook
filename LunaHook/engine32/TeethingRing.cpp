#include "TeethingRing.h"

bool TeethingRing::attach_function()
{
  // https://vndb.org/v5635
  // キミとボクとエデンの林檎
  // HSF932#-C@85FB0:EDEN.exe
  BYTE bytes[] = {
    0x8B,0x0A,0x8B,0xC1,0x83,0xF8,0x20 ,
    0x0F,0x8F,XX4, 
    0x0F,0x84,XX4,
    0x48 ,
    0xBE,0x0F,0x00,0x00,0x00,0x3B,0xC6 ,
    0x77,XX,
  };
  auto addr=MemDbg::findBytes(bytes, sizeof(bytes), processStartAddress, processStopAddress);
  if(addr==0)return false;
  BYTE sehstart[]={
    0x6a,0xff,
    0x68,XX4,
    0x64,0xa1,0,0,0,0
  };
  addr=reverseFindBytes(sehstart,sizeof(sehstart),addr-0x100,addr);
  if(addr==0)return false;
  HookParam hp;
  hp.address = addr;//0x84C70+(DWORD)GetModuleHandle(0);
  hp.type=USING_STRING|NO_CONTEXT|FULL_STRING;
  hp.text_fun = [](hook_stack *stack, HookParam *hp, uintptr_t *data, uintptr_t *split, size_t *len)
  {
    auto _this = (void *)stack->THISCALLTHIS;
    auto a2 = (DWORD *)stack->ARG1;
    
    auto v2 = *a2;
    if ((int)*a2 <= 32)
    {
      if (*a2 != 32)
      {
        switch (v2)
        {

        case 16:
          auto v4 = (char *)(*(int(__thiscall **)(void *, DWORD))(*(DWORD *)_this + 60))(_this, a2[1]);
          *data = (uintptr_t)v4;
          *len = strlen(v4);
        }
      }
    }
  };
  hp.filter_fun=[](void* data, size_t* len, HookParam* hp){
    //#F【琉星】#F
    if(all_ascii((char*)data,*len))return false;
    auto str=std::string((char*)data,*len);
    strReplace(str,"#F","");
    return write_string_overwrite(data,len,str);
  };
  return NewHook(hp, "TeethingRing");
}