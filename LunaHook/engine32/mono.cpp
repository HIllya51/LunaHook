#include"mono.h"
#include"mono/monocommon.hpp"
#include "mono/monoobject.h"

bool monobdwgc() {
    
    HMODULE module = GetModuleHandleW(L"mono-2.0-bdwgc.dll");
	if (module == 0)return false;
	auto [minAddress, maxAddress] = Util::QueryModuleLimits(module);
	BYTE bytes[] = {
		0x3D,0x00,0x00,0x01,0x00,
		0x73,XX,
		0xb8,0x03,0x00,0x00,0x00,
		0xEB,XX
	};
	auto addrs =Util::SearchMemory(bytes, sizeof(bytes),PAGE_EXECUTE, minAddress, maxAddress);
	auto succ=false;
	for (auto addr : addrs) { 
		ConsoleOutput("monobdwgcdll %p", addr);
		HookParam hp;
		hp.address = (DWORD)addr;
		hp.offset=get_reg(regs::eax);
		hp.type = CODEC_UTF16|NO_CONTEXT;
		succ|=NewHook(hp, "monobdwgcdll");
	}
	return succ;
}  
bool monodll() {
    
    HMODULE module = GetModuleHandleW(L"mono.dll");
	if (module == 0)return false;
	auto [minAddress, maxAddress] = Util::QueryModuleLimits(module);
	BYTE bytes[] = {
		0x81,0xFB,XX4,
		0x73
	};
	auto addrs = Util::SearchMemory(bytes, sizeof(bytes), PAGE_EXECUTE, minAddress, maxAddress);
	auto succ=false;
	for (auto addr : addrs) {
		ConsoleOutput("monodll %p", addr);
		HookParam hp;
		hp.address = (DWORD)addr;
		hp.offset=get_reg(regs::ebx);
		hp.type = CODEC_UTF16|NO_CONTEXT;
		succ|=NewHook(hp, "monodll");
	}
	return succ;
}  


bool mono::attach_function() {
    bool il2=monocommon::il2cpp();
	bool bmonobdwgc=monobdwgc();
	bool bmonodll=monodll();
    bool mono=monocommon::hook_mono();
    return il2||bmonobdwgc||bmonodll||mono;
}  