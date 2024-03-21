#include"lucasystem.h"

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
  write_string_overwrite(text,size,result2);
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
    hp.type = CODEC_UTF16 | USING_STRING |EMBED_ABLE|EMBED_BEFORE_SIMPLE|EMBED_AFTER_NEW;//可以内嵌英文
    hp.filter_fun = IG64filter;
    hp.offset=get_reg(regs::rdx);//rdx 
    ok|=NewHook(hp, "IG642");
  }
  return ok;
}
bool InsertLucaSystemHook()
{
  /*
  * Sample games:
  * https://vndb.org/r108105
  */
  const BYTE bytes[] = {
    0xCC,                                // int 3 
    0x48, XX4,                           // mov [rsp+18],rbx       <- hook here
    0x55,                                // push rbp
    0x56,                                // push rsi
    0x57,                                // push rdi
    0x41, 0x54,                          // push r12
    0x41, 0x55,                          // push r13
    0x41, 0x56,                          // push r14
    0x41, 0x57,                          // push r15
    0x48, 0x8D, 0xAC, 0x24, XX4,         // lea rbp,[rsp-00003810]
    0xB8, XX4                            // mov eax,00003910
  };

  auto addr=MemDbg::findBytes(bytes, sizeof(bytes),processStartAddress, processStopAddress);
  if(!addr)return false;
  HookParam hp = {};
  hp.address = addr + 1;
  hp.offset = get_reg(regs::rdx) ; //RDX
  hp.filter_fun = [](LPVOID data, size_t *size, HookParam *)
  {
    static std::wstring prevText;
    auto text = reinterpret_cast<LPWSTR>(data);
    auto len = reinterpret_cast<size_t *>(size);

    if (text[0] == L'\x3000') { //removes space at the beginning of the sentence
      *len -= 2;
      ::memmove(text, text + 1, *len);
    }

    if ( *text == L'@' ) //Name in square brackets instead of '@'
      if ( wchar_t *match2 = cpp_wcsnchr(text+1, L'@', *len/2-1) ) {
        *text = L'[';
        *match2 = L']';
      }

    StringFilterBetween(text, len, L"$C(", 3, L")", 1);
    StringFilter(text, len, L"$A", 3); // remove $A followed by 1 char
    StringCharReplacer(text, len, L"$d", 2, L'\n');
    CharFilter(text, len, L'\xFF3F');
    //ruby
    StringFilter(text, len, L"$[", 2);
    StringFilterBetween(text, len, L"$/", 2, L"$]", 2);

    return true;
  };
  hp.type = CODEC_UTF16 | USING_STRING;
  NewHook(hp, "LucaSystem");
  return true;
}
bool lucasystem::attach_function() { 
  return InsertIG64Hook2()|InsertLucaSystemHook();
}  
 