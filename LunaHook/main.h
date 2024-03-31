#pragma once

// main.h
// 8/23/2013 jichi
// Branch: ITH/IHF_DLL.h, rev 66

#include "types.h"

void TextOutput(ThreadParam tp, TextOutput_T (*buffer), int len);
void ConsoleOutput(LPCSTR text, ...);
void NotifyHookFound(HookParam hp, wchar_t* text);
void NotifyHookRemove(uint64_t addr, LPCSTR name);
bool NewHook(HookParam hp, LPCSTR name);
bool NewHookJit(HookParam hp, LPCSTR name);

void RemoveHook(uint64_t addr, int maxOffset = 9);
std::string LoadResData(LPCWSTR pszResID,LPCWSTR _type);
inline SearchParam spDefault;

// EOF
int HookStrLen(HookParam*,BYTE* data);
inline std::unordered_map<uintptr_t,std::pair<JITTYPE,std::set<uintptr_t> > >emuaddr2jitaddr;
inline std::unordered_map<uintptr_t,std::pair<JITTYPE,uintptr_t>>jitaddr2emuaddr;
void jitaddrclear();
void jitaddraddr(uintptr_t em_addr,uintptr_t jitaddr,JITTYPE);

void context_get(hook_stack*,PCONTEXT);
void context_set(hook_stack*,PCONTEXT);
