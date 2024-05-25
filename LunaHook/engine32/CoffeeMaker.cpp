#include "CoffeeMaker.h"

bool CoffeeMaker::attach_function()
{
  // https://vndb.org/v4025
  // こころナビ
  const BYTE bytes[] = {
      0x81,0xF9,0xD4,0x2B,0x00,0x00,
      0x7F,XX,
      0xB8,0x5D,0x41,0x4C,0xAE,
  };
  auto addr = MemDbg::findBytes(bytes, sizeof(bytes), processStartAddress, processStopAddress);
  if (!addr)
    return false;
  addr = MemDbg::findEnclosingAlignedFunction(addr, 0x10);
  if (!addr)
    return false;
  auto addrs = findxref_reverse_checkcallop(addr, addr - 0x1000, addr + 0x1000, 0xe8);
  if (addrs.size() != 1)
    return false;
  auto addr2 = addrs[0];
  addr2 = MemDbg::findEnclosingAlignedFunction(addr2, 0x40);
  if (!addr2)
    return false;
  HookParam hp;
  hp.address = addr2;
  hp.type = USING_CHAR | CODEC_ANSI_BE | NO_CONTEXT;
  hp.user_value = addr;
  hp.text_fun = [](hook_stack *stack, HookParam *hp, uintptr_t *data, uintptr_t *split, size_t *len)
  {
    auto a2 = stack->stack[1];
    if (a2 > 0x2bd4)
      return;
    auto sub_429050 = (int(__stdcall *)(signed int a1))hp->user_value;
    *data = sub_429050(a2);
    static int idx = 0;
    *len = 2 * (idx % 2);
    idx += 1;
  };

  return NewHook(hp, "CoffeeMaker");
}