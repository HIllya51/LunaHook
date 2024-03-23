#pragma once

// texthook.h
// 8/24/2013 jichi
// Branch: IHF_DLL/IHF_CLIENT.h, rev 133
//
// 8/24/2013 TODO:
// - Clean up this file
// - Reduce global variables. Use namespaces or singleton classes instead.
#include "types.h"

// Artikash 6/17/2019 TODO: These have the wrong values on x64
/** jichi 12/24/2014
	  *  @param  addr  function address
	  *  @param  frame  real address of the function, supposed to be the same as addr
	  *  @param  stack  address of current stack - 4
	  *  @return  If success, which is reverted
  */
#ifndef _WIN64
inline std::atomic<bool (*)(LPVOID addr, uintptr_t ebp, uintptr_t esp)> trigger_fun = nullptr;
#endif
// jichi 9/25/2013: This class will be used by NtMapViewOfSectionfor
// interprocedure communication, where constructor/destructor will NOT work.
struct EmbedSharedMem{
	uint64_t use[10];
	uint64_t addr[10];
	uint64_t ctx1[10];
	uint64_t ctx2[10];
	UINT32 waittime;
	UINT32 spaceadjustpolicy;
	UINT32 keeprawtext;
	uint64_t hash;
	wchar_t text[1000];
	bool fontCharSetEnabled;
	UINT8 fontCharSet;
	wchar_t fontFamily[100];
	UINT codepage;
}; 
class TextHook
{
public:
	HookParam hp;
	ALIGNPTR(uint64_t address,void* location);  

	bool Insert(HookParam hp);
	void Clear();

private:
	void Read();
	bool InsertHookCode();
	bool InsertReadCode();
	bool InsertBreakPoint();
	bool RemoveBreakPoint();
	void breakpointcontext(PCONTEXT);
	void Send(uintptr_t dwDatabase);
	int GetLength(hook_stack* stack, uintptr_t in); // jichi 12/25/2013: Return 0 if failed
	int HookStrlen(BYTE* data);
	void RemoveHookCode();
	void RemoveReadCode();
	bool waitfornotify(TextOutput_T* buffer,void*data ,size_t*len,ThreadParam tp);
	void parsenewlineseperator(void*data ,size_t*len);
	volatile DWORD useCount;
	ALIGNPTR(uint64_t __1,HANDLE readerThread);
	ALIGNPTR(uint64_t __2,HANDLE readerEvent); 
	bool err;
	ALIGNPTR(BYTE __4[ 140],BYTE trampoline[x64 ? 140 : 40]); 
	ALIGNPTR(uint64_t __3,BYTE* local_buffer); 
};

enum { MAX_HOOK = 2500};

// EOF
