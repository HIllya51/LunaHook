#include"RRE.h"

static void SpecialRunrunEngine(hook_stack* stack,  HookParam *, uintptr_t *data, uintptr_t *split, size_t*len)
{
  //CC_UNUSED(split);
  DWORD eax = stack->eax, // *(DWORD *)(esp_base - 0x8),
        edx = stack->edx; // *(DWORD *)(esp_base - 0x10);
  DWORD addr = eax + edx; // eax + edx
  *data = *(WORD *)(addr);
  *len = 2;
}
bool InsertRREHook()
{
  ULONG addr = MemDbg::findCallAddress((ULONG)::IsDBCSLeadByte, processStartAddress, processStopAddress);
  if (!addr) {
    ConsoleOutput("RRE: function call does not exist");
    return false;
  }
  WORD sig = 0x51c3;
  HookParam hp;
  hp.address = addr;
  hp.type = NO_CONTEXT|DATA_INDIRECT|USING_CHAR;
  if ((*(WORD *)(addr-2) != sig)) {
    hp.text_fun = SpecialRunrunEngine;
    ConsoleOutput("INSERT Runrun#1");
    return NewHook(hp, "RunrunEngine Old");
  } else {
    hp.offset=get_reg(regs::eax);
    ConsoleOutput("INSERT Runrun#2");
    return NewHook(hp, "RunrunEngine");
  }
  //ConsoleOutput("RunrunEngine, hook will only work with text speed set to slow or normal!");
  //else ConsoleOutput("Unknown RunrunEngine engine");
}

bool RRE::attach_function() {
    
    return InsertRREHook();
} 