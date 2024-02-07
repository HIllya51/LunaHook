#include"Jellyfish.h"

bool Jellyfish_attach_function() {  
  //https://vndb.org/r13456
  //GREEN～秋空のスクリーン～ STANDARD EDITION
  //https://vndb.org/r1136
  //LOVERS～恋に落ちたら…～

  auto ism=GetModuleHandle(L"ism.dll");
  if(ism==0)return false;
  auto [minaddr,maxaddr]=Util::QueryModuleLimits(ism);
  const BYTE bytes[] = {
    0x8a,XX,0x01, //mov     cl, [ecx+1]
    0x80,XX,0x6e, //cmp     cl, 6Eh ; 'n' 
    0x75,XX,
  };
  ULONG addr = MemDbg::findBytes(bytes, sizeof(bytes), minaddr, maxaddr);
  if (addr == 0)return false;
  addr = MemDbg::findEnclosingAlignedFunction(addr,0x1000);
  if (addr == 0)return false;
  HookParam hp;
  hp.address = addr;
  hp.offset=get_stack(1);
  hp.type = USING_STRING; 
  hp.filter_fun=[](LPVOID data, size_t *size, HookParam *){
    if(*size==2)return false;
    StringCharReplacer(reinterpret_cast<char*>(data), size, "\\n", 2, '\n');
    StringCharReplacer(reinterpret_cast<char*>(data), size, "\\N", 2, '\n');
    auto str=std::string(reinterpret_cast<char*>(data),*size);
    str = std::regex_replace(str, std::regex("\\\\[0-7a-zA-Z]"), "");

    *size = str.size() ;
    strcpy(reinterpret_cast<char*>(data), str.c_str()); 
    return true;
  };
  
  return NewHook(hp, "Jellyfish");
}  
bool Jellyfish::attach_function(){
  return Jellyfish_attach_function();
}