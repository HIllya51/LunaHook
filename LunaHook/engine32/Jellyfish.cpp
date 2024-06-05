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
    
    return write_string_overwrite(data,size,std::regex_replace(std::string(reinterpret_cast<char*>(data),*size), std::regex("\\\\[0-7a-zA-Z]"), ""));
  };
  
  return NewHook(hp, "Jellyfish");
}

bool Jellyfish_attach_function2() {  
  //https://vndb.org/r109826
  //Sisters: Last Day of Summer


  auto ism=GetModuleHandle(L"ism.dll");
  if(ism==0)return false;
  auto [minaddr,maxaddr]=Util::QueryModuleLimits(ism);
  const BYTE bytes[] = {
    0x68,0xB0,0x04,0x00,0x00,0x68,0x40,0x06,0x00,0x00
  };
  std::map<DWORD,int>count;
  DWORD maxa=0;int maxi=0;
  for(auto _:Util::SearchMemory(bytes,sizeof(bytes),PAGE_EXECUTE,minaddr,maxaddr)){
    _ = findfuncstart(_);
    if(_==0)continue;
    if(count.find(_)==count.end())
      count[_]=0;
    count[_]+=1;
    if(count[_]>=maxi){
      maxi=count[_];
      maxa=_;
    }
  }
  if (maxa == 0)return false;
  HookParam hp;
  hp.address = maxa;//0x2F2E1+(DWORD)ism;
  hp.type =USING_CHAR|CODEC_UTF16;
  hp.text_fun=[](hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
      if(stack->ARG3==3)return;
      *split=(stack->ARG3)!=0;//多行文本
      *data=stack->ARG1;
      *len=2;
      //不可以快进，否则会有重复
  };
  
  return NewHook(hp, "Jellyfish");
}  
bool Jellyfish::attach_function(){
  return Jellyfish_attach_function()||Jellyfish_attach_function2();
}