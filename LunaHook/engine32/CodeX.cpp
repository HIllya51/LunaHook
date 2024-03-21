#include"CodeX.h"

bool CodeXFilter(LPVOID data, size_t *size, HookParam *)
{
  auto text = reinterpret_cast<LPSTR>(data);
  auto len = reinterpret_cast<size_t *>(size);

  StringCharReplacer(text, len, "^n", 2, ' ');
  
  //|晒[さら]
  std::string result = std::string((char*)data,*len);
  result = std::regex_replace(result, std::regex("\\|(.+?)\\[(.+?)\\]"), "$1");
  
  return write_string_overwrite(data,len,result);
}

bool InsertCodeXHook() 
{
  
    /*
    * Sample games:
    * https://vndb.org/v41664
    * https://vndb.org/v36122
    */
  const BYTE bytes[] = {
    0x83, 0xC4, 0x08,           // add esp,08                  << hook here
    0x8D, 0x85, XX4,            // lea eax,[ebp-00000218]
    0x50,                       // push eax
    0x68, XX4,                  // push ???????????!.exe+10A76C
    0x85, 0xF6,                 // test esi,esi
    0x74, 0x4F,                 // je ???????????!.exe+2A95B
    0xFF, 0x15, XX4,            // call dword ptr [???????????!.exe+C8140]
    0x8B, 0x85, XX4             // mov eax,[ebp-00000220]      << alternative hook here
  };

  ULONG range = min(processStopAddress - processStartAddress, MAX_REL_ADDR);
  ULONG addr = MemDbg::findBytes(bytes, sizeof(bytes), processStartAddress, processStartAddress + range);
  if (!addr) {
    ConsoleOutput("CodeX: pattern not found");
    return false;
  }

  HookParam hp;
  hp.address = addr;
  hp.offset=get_reg(regs::eax);
  hp.index = 0;
  hp.type = USING_STRING;
  hp.filter_fun = CodeXFilter;
  ConsoleOutput("INSERT CodeX");
  
  return NewHook(hp, "CodeX");
}
namespace{
  bool hook(){
    //霞外籠逗留記
    BYTE _[]={0x90,0x90,0x68,0x64,0x7B,0x4C,0x00}; //aHdL db 'hd{L',0 
    ULONG addr = MemDbg::findBytes(_, sizeof(_), processStartAddress, processStopAddress);
    if(addr==0)return false;
    addr+=2;
    BYTE bytes[]={0x68,XX4};  
    memcpy(bytes+1,&addr,4);    
    auto addrs = Util::SearchMemory(bytes, sizeof(bytes), PAGE_EXECUTE, processStartAddress, processStopAddress);
    bool succ=false;
    for(auto adr:addrs){
      adr=MemDbg::findEnclosingAlignedFunction(adr);
      if(adr==0)continue;
      HookParam hp;
      hp.address = adr;
      hp.offset=get_stack(1);
      hp.type = CODEC_ANSI_BE;
      succ|=NewHook(hp, "CodeX");
    }
      return succ;
  }
}
bool CodeX::attach_function() { 
    return InsertCodeXHook()||hook();
} 