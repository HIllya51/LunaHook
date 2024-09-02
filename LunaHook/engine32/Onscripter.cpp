#include "Onscripter.h"

namespace
{
  // Monster Girl Quest Remastered
  bool hook()
  {
    auto SDL_PushEvent = GetProcAddress(GetModuleHandle(0), "SDL_PushEvent");
    if (!SDL_PushEvent)
      return false;
    BYTE bytes[] = {
        0x8D, 0x45, 0xE8,
        0x50,
        0xC6, 0x45, 0xE8, 0x1D,
        0xE8, XX4};
    for (auto addr : Util::SearchMemory(bytes, sizeof(bytes), PAGE_EXECUTE, processStartAddress, processStopAddress))
    {
      addr += sizeof(bytes) - 5;
      auto check = *(DWORD *)(addr + 1) + addr + 5;
      if (check != (DWORD)SDL_PushEvent)
        continue;
      HookParam hp;
      hp.address = addr;
      hp.offset = get_reg(regs::ebx);
      hp.type = USING_CHAR | DATA_INDIRECT | NO_CONTEXT; // 快的和慢的分别一支。慢的快进会被截断
      return NewHook(hp, "onscripter");
    }
    return false;
  }
  bool hook2()
  {
    BYTE bytes[] = {
        //clang-format off
        0x8b, 0xbe, XX2, 0x00, 0x00,
        0x80, 0x3c, 0x07, 0x00,
        0x8d, 0x1c, 0x07,
        0x75, XX,
        0x8b, 0xce,
        0xe8, XX4,
        //clang-format on
    };
    auto addr = MemDbg::findBytes(bytes, sizeof(bytes), processStartAddress, processStopAddress);
    if (!addr)
      return false;
    HookParam hp;
    hp.address = addr;
    hp.offset = get_reg(regs::eax);
    hp.type = USING_STRING | CODEC_UTF8;
    hp.filter_fun = [](LPVOID data, size_t *size, HookParam *)
    {
      auto xx = std::string((char *)data, *size);
      static std::string last;
      if (xx == last)
        return false;
      last = xx;
      strReplace(xx, "@", "");
      strReplace(xx, "\\", "");
      strReplace(xx, "_", "\n");
      strReplace(xx, "/", "");
      // # ( ) < 代码里，但C了一会儿没遇到，不管了先
      return write_string_overwrite(data, size, xx);
    };
    return NewHook(hp, "onscripter");
  }
}
bool Onscripter::attach_function()
{
  return hook2() || hook();
}