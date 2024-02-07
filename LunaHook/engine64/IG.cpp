#include"IG.h"

bool InsertIG64Hook() {
  //���륿���� FHD.exe
  //char __fastcall sub_14004D820(_QWORD *a1, __int16 *a2, size_t a3)
    
  const BYTE BYTES[] = {
    0x48,0x8b,0x43,0x08,
    0x33,0xc9,
    0x66,0x89,0x08 
  };
  auto addrs = Util::SearchMemory(BYTES, sizeof(BYTES), PAGE_EXECUTE, processStartAddress, processStopAddress);
  auto suc=false;
  for (auto addr : addrs) { 
    ConsoleOutput("IG64 %p", addr);
    const BYTE aligned [] = {0xCC,0xCC};
    addr = reverseFindBytes(aligned, sizeof(aligned), addr-0x1000, addr);
    if (addr == 0)continue;
    addr += 2;
    ConsoleOutput("IG64 %p", addr);
    HookParam hp;
    hp.address = addr;
    hp.type = CODEC_UTF16 | USING_STRING;
    hp.offset=get_reg(regs::rdx);//rdx 
    suc|=NewHook(hp, "IG64"); 
  } 
  return suc;
} 
bool IG64filter(void* data, size_t* size, HookParam*) {
  
  auto text = reinterpret_cast<LPWSTR>(data); 
  std::wstring str =std::wstring(text,*size / 2);
  std::wregex reg1(L"\\$\\[(.*?)\\$/(.*?)\\$\\]");
  std::wstring result1 = std::regex_replace(str, reg1, L"$1");
    
  std::wregex reg2(L"@[^@]*@");
  std::wstring result2 = std::regex_replace(result1, reg2, L"");
    
  *size = (result2.size()) * 2;
  wcscpy(text, result2.c_str());
  return true;
};
bool InsertIG64Hook2() {
  //hook1 ��ע�͵�ʱ���Ͽ������hook����Ͽ������ǻᱣ��һЩ@[]֮��Ľű����š�
  //���륿���� FHD.exe
    
  const BYTE BYTES[] = {
      0xBA,0x3F,0xFF,0x00,0x00,
      XX,0x8B,XX,
      0xE8,XX2,0x00,0x00
  };
  bool ok=false;
  auto addrs = Util::SearchMemory(BYTES, sizeof(BYTES), PAGE_EXECUTE, processStartAddress, processStopAddress);
  for (auto addr : addrs) {
    ConsoleOutput("IG642 %p", addr);
    const BYTE aligned[] = { 0xCC,0xCC };
    addr = reverseFindBytes(aligned, sizeof(aligned), addr - 0x10000, addr);
    if (addr == 0)continue;
    addr += 2;
    ConsoleOutput("IG642 %p", addr);
    HookParam hp;
    hp.address = addr;
    hp.type = CODEC_UTF16 | USING_STRING;
    hp.filter_fun = IG64filter;
    hp.offset=get_reg(regs::rdx);//rdx 
    ok|=NewHook(hp, "IG642");
  }
  return ok;
}
bool IG::attach_function() { 
  return InsertIG64Hook2();
}  
 