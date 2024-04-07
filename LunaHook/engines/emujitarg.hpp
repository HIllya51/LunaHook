#pragma once
#ifdef _WIN64
namespace YUZU
{
class emu_arg{
    hook_stack* stack;
public:
    emu_arg(hook_stack* stack_):stack(stack_){};
    uintptr_t operator [](int idx){
        auto base=stack->r13;
        auto args=(uintptr_t*)stack->r15;
        return base+args[idx];
    }
};
}
namespace VITA3K
{
class emu_addr{
    hook_stack* stack;
    DWORD addr;
public:
    emu_addr(hook_stack* stack_,DWORD addr_):stack(stack_),addr(addr_){};
    operator uintptr_t(){
        auto base=stack->r13;
        return base+addr;
    }
    operator DWORD*(){
        return (DWORD*)(uintptr_t)*this;
    }
};
class emu_arg{
    hook_stack* stack;
public:
    emu_arg(hook_stack* stack_):stack(stack_){};
    uintptr_t operator [](int idx){
        auto args=(uint32_t*)stack->r15;
        return emu_addr(stack,args[idx]);
    }
};
}
#endif
namespace PPSSPP{
    inline DWORD x86_baseaddr;
class emu_addr{
    hook_stack* stack;
    DWORD addr;
public:
    emu_addr(hook_stack* stack_,DWORD addr_):stack(stack_),addr(addr_){};
    operator uintptr_t(){
        #ifndef _WIN64
        auto base=x86_baseaddr;
        #else
        auto base=stack->rbx;
        #endif
        return base+addr;
    }
    operator DWORD*(){
        return (DWORD*)(uintptr_t)*this;
    }
};
class emu_arg{
    hook_stack* stack;
public:

    emu_arg(hook_stack* stack_):stack(stack_){};
    uintptr_t operator [](int idx){
        #ifndef _WIN64
        auto args=stack->ebp;
        #else
        auto args=stack->r14;
        #endif
		auto offR = -0x80;
		auto offset = offR + 0x10 + idx * 4;
        return (uintptr_t)emu_addr(stack,*(uint32_t*)(args+offset));
    }
};
}