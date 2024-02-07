#include"Candy.h"

/********************************************************************************************
CandySoft hook:
  Game folder contains many *.fpk. Engine name is SystemC.
  I haven't seen this engine in other company/brand.

  AGTH /X3 will hook lstrlenA. One thread is the exactly result we want.
  But the function call is difficult to located programmatically.
  I find a equivalent points which is more easy to search.
  The script processing function needs to find 0x5B'[',
  so there should a instruction like cmp reg,5B
  Find this position and navigate to function entry.
  The first parameter is the string pointer.
  This approach works fine with game later than つよきす２学�

  But the original つよき�is quite different. I handle this case separately.

********************************************************************************************/

namespace { // unnamed Candy

// jichi 8/23/2013: split into two different engines
//if (_wcsicmp(processName, L"systemc.exe")==0)
// Process name is "SystemC.exe"
bool InsertCandyHook1()
{
  for (DWORD i = processStartAddress + 0x1000; i < processStopAddress - 4; i++)
    if ((*(DWORD *)i&0xffffff) == 0x24f980) // cmp cl,24
      for (DWORD j = i, k = i - 0x100; j > k; j--)
        if (*(DWORD *)j == 0xc0330a8a) { // mov cl,[edx]; xor eax,eax
          HookParam hp;
          hp.address = j;
          hp.offset=get_reg(regs::edx);
          hp.type = USING_STRING;
          ConsoleOutput("INSERT SystemC#1");
          
          //RegisterEngineType(ENGINE_CANDY);
          return NewHook(hp, "SystemC");
        }
  ConsoleOutput("CandyHook1: failed");
  return false;
}

// jichi 8/23/2013: Process name is NOT "SystemC.exe"
bool InsertCandyHook2()
{
  for (DWORD i = processStartAddress + 0x1000; i < processStopAddress - 4 ;i++)
    if (*(WORD *)i == 0x5b3c || // cmp al,0x5b
        (*(DWORD *)i & 0xfff8fc) == 0x5bf880) // cmp reg,0x5B
      for (DWORD j = i, k = i - 0x100; j > k; j--)
        if ((*(DWORD *)j & 0xffff) == 0x8b55) { // push ebp, mov ebp,esp, sub esp,*
          HookParam hp;
          hp.address = j;
          if(((*(BYTE *)(j+3)))==0x51) //push    ecx ,thiscall
            hp.offset=get_reg(regs::ecx);      //アイドルクリニック～恋の薬でHな処方～
          else
            hp.offset=get_stack(1);    // jichi: text in arg1
          hp.type = USING_STRING;
          
          //RegisterEngineType(ENGINE_CANDY);
          return NewHook(hp, "SystemC");
        }
  ConsoleOutput("CandyHook2: failed");
  return false;
}

/** jichi 10/2/2013: CHECKPOINT
 *
 *  [5/31/2013] 恋もHもお勉強も、おまかせ�お姉ちも�部
 *  base = 0xf20000
 *  + シナリオ: /HSN-4@104A48:ANEBU.EXE
 *    - off: 4294967288 = 0xfffffff8 = -8
 ,    - type: 1025 = 0x401
 *  + 選択肢: /HSN-4@104FDD:ANEBU.EXE
 *    - off: 4294967288 = 0xfffffff8 = -8
 *    - type: 1089 = 0x441
 */
//bool InsertCandyHook3()
//{
//  return false; // CHECKPOINT
//  const BYTE ins[] = {
//    0x83,0xc4, 0x0c, // add esp,0xc ; hook here
//    0x0f,0xb6,0xc0,  // movzx eax,al
//    0x85,0xc0,       // test eax,eax
//    0x75, 0x0e       // jnz XXOO ; it must be 0xe, or there will be duplication
//  };
//  enum { addr_offset = 0 };
//  ULONG range = min(processStopAddress - processStartAddress, MAX_REL_ADDR);
//  ULONG reladdr = SearchPattern(processStartAddress, range, ins, sizeof(ins));
//  reladdr = 0x104a48;
//  GROWL_DWORD(processStartAddress);
//  //GROWL_DWORD3(reladdr, processStartAddress, range);
//  if (!reladdr)
//    return false;
//
//  HookParam hp;
//  hp.address = processStartAddress + reladdr + addr_offset;
//  hp.offset=get_reg(regs::eax);
//  hp.type = USING_STRING|NO_CONTEXT;
//  NewHook(hp, "Candy");
//  return true;
//}

} // unnamed Candy

namespace{
bool candy3(){
  //お母さんは俺専用！～あなたの初めてを…母さんが貰ってア・ゲ・ル～
  //茉莉子さん家の性事情 ~伯母さんは僕のモノ~
  const BYTE bytes[] = {
      0x24, //XX||XX2
      0x75 
  };  
  for (auto addr : Util::SearchMemory(bytes, sizeof(bytes), PAGE_EXECUTE)){
    ConsoleOutput("%x",addr);
    if((*(BYTE*)(addr-1) ==0x3c)||((*(BYTE*)(addr-2) ==0x83)&&(*(BYTE*)(addr-1) ==0xf9))){
      addr=MemDbg::findEnclosingAlignedFunction(addr);
      if(addr==0)continue;
      ConsoleOutput("!%x",addr);
      HookParam hp;
      hp.type = USING_STRING;
      if(*(BYTE*)addr==0x55)
        hp.offset=get_stack(1);
      else if(*(BYTE*)addr==0x56)
        hp.offset=get_reg(regs::eax);
      else
        continue;
      hp.address = addr;
      
      return NewHook(hp, "candy3");
    } 
  }
  return false;
}
bool InsertCandyHook3() 
{
  
    /*
    * Sample games:
    * https://vndb.org/v24878
    */
  const BYTE bytes[] = {
    0xCC,                                // int 3 
    0x55,                                // push ebp        << hook here
    0x8B, 0xEC,                          // mov ebp,esp
    0x6A, 0xFF,                          // push -01
    0x68, XX4,                           // push iinari-omnibus.exe+C4366
    0x64, 0xA1, 0x00, 0x00, 0x00, 0x00,  // mov eax,fs:[00000000]
    0x50,                                // push eax
    0x83, 0xEC, 0x74,                    // sub esp,74
    0x53,                                // push ebx
    0x56,                                // push esi
    0x57                                 // push edi
  };

  ULONG range = min(processStopAddress - processStartAddress, MAX_REL_ADDR);
  ULONG addr = MemDbg::findBytes(bytes, sizeof(bytes), processStartAddress, processStartAddress + range);
  if (!addr) {
    ConsoleOutput("SystemC#3: pattern not found");
    return false;
  }
  HookParam hp;
  hp.address = addr + 1;
  hp.offset=get_stack(4);
  hp.type = USING_STRING | CODEC_UTF16;
  ConsoleOutput("INSERT SystemC#3");
  
  return NewHook(hp, "SystemC#3");
}
}
// jichi 10/2/2013: Add new candy hook
bool InsertCandyHook()
{
	PcHooks::hookOtherPcFunctions();
  //if (0 == _wcsicmp(processName, L"systemc.exe"))
  if (Util::CheckFile(L"SystemC.exe"))
    return InsertCandyHook1()||candy3();
  else{
    //return InsertCandyHook2();
    bool b2 = InsertCandyHook2(),
        b3 = InsertCandyHook3();
    return b2 || b3;
  }
}

bool Candy::attach_function() {
    
    return InsertCandyHook();
} 


bool WillowSoft::attach_function(){
  //お母さんがいっぱい!!限定ママBOX
  const BYTE bytes[] = {
      0xF7 ,0xC2 ,0x00 ,0x00 ,0xFF ,0x00,
      XX2,
      0xF7 ,0xC2 ,0x00 ,0x00 ,0x00 ,0xFF ,
      XX2
  }; 
  auto addr=MemDbg::findBytes(bytes, sizeof(bytes), processStartAddress, processStopAddress);
  if(addr==0)return false;
   addr=MemDbg::findEnclosingAlignedFunction(addr);
   if(addr==0)return false;

  HookParam hp;
  hp.type = USING_STRING;
  hp.offset=get_stack(2);
  hp.type |= DATA_INDIRECT;
	hp.index = 0;
  hp.address = addr;
 
  
  return NewHook(hp, "WillowSoft");
}