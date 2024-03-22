#include"yuzusuyu.h"
#include"mages/mages.h"
namespace{
        
    auto isFastMem = true;

    auto isVirtual = true;//Process.arch === 'x64' && Process.platform === 'windows';
    auto idxDescriptor = isVirtual == true ? 2 : 1;
    auto idxEntrypoint = idxDescriptor + 1;

uintptr_t getDoJitAddress() {
    auto RegisterBlockSig1 = "E8 ?? ?? ?? ?? 4? 8B ?? 4? 8B ?? 4? 8B ?? E8 ?? ?? ?? ?? 4? 89?? 4? 8B???? ???????? 4? 89?? ?? 4? 8B?? 4? 89";
    auto RegisterBlock=find_pattern(RegisterBlockSig1,processStartAddress,processStopAddress); 
    if (RegisterBlock) {
        auto beginSubSig1 = "CC 40 5? 5? 5?";
        auto lookbackSize = 0x400;
        auto address=RegisterBlock-lookbackSize;
        auto subs=find_pattern(beginSubSig1,address,address+lookbackSize);
        if(subs){
            return subs+1;
        }
    }

    auto PatchSig1 = "4????? 4????? 4????? FF?? ?? 4????? ?? 4????? 75 ?? 4????? ?? 4????? ?? 4?";
    auto Patch = find_pattern(PatchSig1,processStartAddress,processStopAddress);
    if (Patch) {
        auto beginSubSig1 = "4883EC ?? 48";
        auto lookbackSize = 0x80;
        auto address = Patch-lookbackSize;
        auto subs = find_pattern(beginSubSig1,address,address+lookbackSize);
        if (subs) {
            idxDescriptor = 1;
            idxEntrypoint = 2;
            return subs;
        }
    }
    return 0;
    /*
    这块不知道怎么实现。
    // DebugSymbol: RegisterBlock
    // ?RegisterBlock@EmitX64@X64@Backend@Dynarmic@@IEAA?AUBlockDescriptor@1234@AEBVLocationDescriptor@IR@4@PEBX_K@Z <- new
    // ?RegisterBlock@EmitX64@X64@Backend@Dynarmic@@IEAA?AUBlockDescriptor@1234@AEBVLocationDescriptor@IR@4@PEBX1_K@Z
    const symbols = DebugSymbol.findFunctionsMatching('Dynarmic::Backend::X64::EmitX64::RegisterBlock');
    if (symbols.length !== 0) {
        return symbols[0];
    }

    // DebugSymbol: Patch
    // ?Patch@EmitX64@X64@Backend@Dynarmic@@IEAAXAEBVLocationDescriptor@IR@4@PEBX@Z
    const patchs = DebugSymbol.findFunctionsMatching('Dynarmic::Backend::X64::EmitX64::Patch');
    if (patchs.length !== 0) {
        idxDescriptor = 1;
        idxEntrypoint = 2;
        return patchs[0];
    }
    */
}

uintptr_t* argidx(hook_stack* stack,int idx){
    auto offset=0;
    switch (idx)
    {
    case 0:offset=get_reg(regs::rcx);break;
    case 1:offset=get_reg(regs::rdx);break;
    case 2:offset=get_reg(regs::r8);break;
    case 3:offset=get_reg(regs::r9);break;
    }
    return (uintptr_t*)((uintptr_t)stack+sizeof(hook_stack)-sizeof(uintptr_t)+offset);
}

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
struct emfuncinfo{
    const char* hookname;
    void* hookfunc;
	void* filterfun;
    const wchar_t* _id;
    const wchar_t* _version;
};
std::unordered_map<uintptr_t,emfuncinfo>emfunctionhooks;


bool checkiscurrentgame(const emfuncinfo& em){
	auto wininfos=get_proc_windows();
	for(auto&& info:wininfos){
		if(info.title.find(em._version)!=info.title.npos)return true;
	}
	return false;
}
}
bool yuzusuyu::attach_function()
{
    ConsoleOutput("[Compatibility] Yuzu 1616+");
   auto DoJitPtr=getDoJitAddress();
   if(DoJitPtr==0)return false;
   ConsoleOutput("DoJitPtr %p",DoJitPtr);
   HookParam hp;
   hp.address=DoJitPtr;
   hp.text_fun=[](hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
        auto descriptor = *argidx(stack,idxDescriptor); // r8
        auto entrypoint = *argidx(stack,idxEntrypoint); // r9
        auto em_address = *(uintptr_t*)descriptor;
        em_address-=0x80004000;
        if(emfunctionhooks.find(em_address)==emfunctionhooks.end() || !entrypoint)return;
        auto op=emfunctionhooks.at(em_address);
        if(!(checkiscurrentgame(op)))return;

        HookParam hpinternal;
        hpinternal.address=entrypoint;
        hpinternal.type=CODEC_UTF16|USING_STRING|NO_CONTEXT;
        hpinternal.text_fun=(decltype(hpinternal.text_fun))op.hookfunc;
		hpinternal.filter_fun=(decltype(hpinternal.filter_fun))op.filterfun;
        NewHook(hpinternal,op.hookname);
    
   };
  return NewHook(hp,"YuzuDoJit");
} 
// @name         [0100978013276000] Memories Off
// @version      1.0.0 - 1.0.1
// @author       Koukdw
// @description  Yuzu
// * MAGES. inc.
// * MAGES Engine
void _0100978013276000(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto s=mages::readString(emu_arg(stack)[0],0);
    write_string_new(data,len,s);
}

// @name         [0100A460141B8000] Shiro to Kuro no Alice
// @version      1.0.0
// @author       Koukdw
// @description  
// * Kogado Girls Project
// * Idea Factory (アイディアファクトリー) & Otomate
// * AliceNX_MPA (string inside binary)
// *
void _0100A460141B8000(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=emu_arg(stack)[0];
    hp->type=USING_STRING|CODEC_UTF8|NO_CONTEXT;
    *data=address;*len=strlen((char*)address);
}
namespace{
auto _=[](){
    emfunctionhooks={
            {0x8003eeac - 0x80004000,{"Memories Off",_0100978013276000,0,L"0100978013276000",L"1.0.0"}},
            {0x8003eebc - 0x80004000,{"Memories Off",_0100978013276000,0,L"0100978013276000",L"1.0.1"}},
            
            // Shiro to Kuro no Alice
            {0x80013f20 - 0x80004000,{"Shiro to Kuro no Alice",_0100A460141B8000,NewLineCharFilterW,L"0100A460141B8000",L"1.0.0"}},
            {0x80013f94 - 0x80004000,{"Shiro to Kuro no Alice",_0100A460141B8000,NewLineCharFilterW,L"0100A460141B8000",L"1.0.0"}},
            {0x8001419c - 0x80004000,{"Shiro to Kuro no Alice",_0100A460141B8000,NewLineCharFilterW,L"0100A460141B8000",L"1.0.0"}},
            // Shiro to Kuro no Alice -Twilight line-
            {0x80014260 - 0x80004000,{"Shiro to Kuro no Alice -Twilight line-",_0100A460141B8000,NewLineCharFilterW,L"0100A460141B8000",L"1.0.0"}},
            {0x800142d4 - 0x80004000,{"Shiro to Kuro no Alice -Twilight line-",_0100A460141B8000,NewLineCharFilterW,L"0100A460141B8000",L"1.0.0"}},
            {0x800144dc - 0x80004000,{"Shiro to Kuro no Alice -Twilight line-",_0100A460141B8000,NewLineCharFilterW,L"0100A460141B8000",L"1.0.0"}},
    };
    return 1;
}();
}