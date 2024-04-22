#pragma once

#include "const.h"

class WinMutex // Like CMutex but works with scoped_lock
{
public:
	WinMutex(std::wstring name = L"", LPSECURITY_ATTRIBUTES sa = nullptr) : m(CreateMutexW(sa, FALSE, name.empty() ? NULL : name.c_str())) {}
	void lock() { if (m) WaitForSingleObject(m, INFINITE); }
	void unlock() { if (m) ReleaseMutex(m); }

private:
	AutoHandle<> m;
};

inline SECURITY_ATTRIBUTES allAccess = std::invoke([] // allows non-admin processes to access kernel objects made by admin processes
{
	static SECURITY_DESCRIPTOR sd = {};
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
	return SECURITY_ATTRIBUTES{ sizeof(SECURITY_ATTRIBUTES), &sd, FALSE };
});
 

struct hook_stack
{

#ifndef _WIN64
	uintptr_t _eflags;		//pushfd
	uintptr_t edi,        // pushad
		esi,
		ebp,
		esp,
		ebx,
		edx,
		ecx,        // this
		eax;        // 0x28
	
#else
	uintptr_t r15,
		r14,
		r13,
		r12,
		r11,
		r10,
		r9,
		r8,
		rdi,
		rsi,
		rbp,
		rsp,
		rdx,
		rcx,
		rbx,
		rax;
#endif
	uintptr_t eflags;     // pushaf
	union
	{
		uintptr_t stack[1];   // beginning of the runtime stack
		uintptr_t retaddr;
		BYTE base[1];
	};
	uintptr_t get_base(){
		return (uintptr_t)this+sizeof(hook_stack)-sizeof(uintptr_t);
	}
};

inline hook_stack* get_hook_stack(uintptr_t lpDataBase){
	return (hook_stack*)(lpDataBase-sizeof(hook_stack)+sizeof(uintptr_t));
}
// jichi 3/7/2014: Add guessed comment

#define ALIGNPTR(Y,X) union { \
     ##Y; \
    ##X; \
};


enum class JITTYPE{
	PC,//not a jit
    YUZU,
    PPSSPP,
	VITA3K,
	RPCS3
};
struct HookParam
{
	ALIGNPTR(uint64_t __11,uintptr_t address); // absolute or relative address
	int offset, // offset of the data in the memory
		index, // deref_offset1
		split, // offset of the split character
		split_index; // deref_offset2

	wchar_t module[MAX_MODULE_SIZE];

	char function[MAX_MODULE_SIZE];
	uint64_t type; // flags
	UINT codepage; // text encoding
	short length_offset; // index of the string length
	ALIGNPTR(uint64_t __1,uintptr_t padding); // padding before string
	ALIGNPTR(uint64_t __12,uintptr_t user_value);
	ALIGNPTR(uint64_t __2,void(*text_fun)(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len))
	ALIGNPTR(uint64_t __3,bool(*filter_fun)(void* data, size_t* len, HookParam* hp)); // jichi 10/24/2014: Add filter function. Return false to skip the text
	ALIGNPTR(uint64_t __6,bool (*hook_before)(hook_stack* stack,void* data, size_t* len,uintptr_t*role));
	ALIGNPTR(uint64_t __7,void (*hook_after)(hook_stack* stack,void* data, size_t len));
	ALIGNPTR(uint64_t __8,uintptr_t hook_font);
	ALIGNPTR(uint64_t __9,const wchar_t* newlineseperator);
	char name[HOOK_NAME_SIZE];
	wchar_t hookcode[HOOKCODE_LEN];
	HookParam(){
		ZeroMemory(this,sizeof(HookParam));
	}
	ALIGNPTR(uint64_t __10,uintptr_t emu_addr);
	int argidx;
	JITTYPE jittype;
};

struct ThreadParam
{
	bool operator==(ThreadParam other) const { return processId == other.processId && addr == other.addr && ctx == other.ctx && ctx2 == other.ctx2; }
	DWORD processId;
	uint64_t addr;
	uint64_t ctx; // The context of the hook: by default the first value on stack, usually the return address
	uint64_t ctx2;  // The subcontext of the hook: 0 by default, generated in a method specific to the hook
};

struct SearchParam
{
	BYTE pattern[PATTERN_SIZE] = { x64 ? 0xcc : 0x55, x64 ? 0xcc : 0x8b, x64 ? 0x48 : 0xec, 0x89 }; // pattern in memory to search for
	int address_method=0;
	int search_method=0;
	int length = x64 ? 4 : 3, // length of pattern (zero means this SearchParam is invalid and the default should be used)
		offset = x64 ? 2 : 0, // offset from start of pattern to add hook
		searchTime = 30000, // ms
		maxRecords = 100000,
		codepage = SHIFT_JIS;
	//uintptr_t padding = 0, // same as hook param padding
	//	minAddress = 0, maxAddress = (uintptr_t)-1; // hook all functions between these addresses (used only if both modules empty)
	ALIGNPTR(uint64_t __1,uintptr_t padding = 0);
	ALIGNPTR(uint64_t __2,uintptr_t minAddress = 0);
	ALIGNPTR(uint64_t __3,uintptr_t maxAddress = (uintptr_t)-1);
	wchar_t boundaryModule[MAX_MODULE_SIZE] = {}; // hook all functions within this module (middle priority)
	wchar_t exportModule[MAX_MODULE_SIZE] = {}; // hook the exports of this module (highest priority)
	wchar_t text[PATTERN_SIZE] = {}; // text to search for
	JITTYPE jittype;
};

struct InsertHookCmd // From host
{
	InsertHookCmd(HookParam hp) : hp(hp) {}
	HostCommandType command = HOST_COMMAND_NEW_HOOK;
	HookParam hp;
};
struct RemoveHookCmd // From host
{
	RemoveHookCmd(uint64_t address) : address(address) {}
	HostCommandType command = HOST_COMMAND_REMOVE_HOOK;
	uint64_t address;
};

struct FindHookCmd // From host
{
	FindHookCmd(SearchParam sp) : sp(sp) {}
	HostCommandType command = HOST_COMMAND_FIND_HOOK;
	SearchParam sp;
};

struct ConsoleOutputNotif // From dll
{
	ConsoleOutputNotif(std::string message = "") { strncpy_s(this->message, message.c_str(), MESSAGE_SIZE - 1); }
	HostNotificationType command = HOST_NOTIFICATION_TEXT;
	char message[MESSAGE_SIZE] = {};
};

struct HookFoundNotif // From dll
{
	HookFoundNotif(HookParam hp, wchar_t* text) : hp(hp) { wcsncpy_s(this->text, text, MESSAGE_SIZE - 1); }
	HostNotificationType command = HOST_NOTIFICATION_FOUND_HOOK;
	HookParam hp;
	wchar_t text[MESSAGE_SIZE] = {}; // though type is wchar_t, may not be encoded in UTF-16 (it's just convenient to use wcs* functions)
};

struct HookRemovedNotif // From dll
{
	HookRemovedNotif(uint64_t address) : address(address) {};
	HostNotificationType command = HOST_NOTIFICATION_RMVHOOK;
	uint64_t address;
};

struct HookInsertingNotif // From dll
{
	HookInsertingNotif(uint64_t addr1):addr(addr1){}
	HostNotificationType command = HOST_NOTIFICATION_INSERTING_HOOK; 
	uint64_t addr;
};

struct TextOutput_T
{
	ThreadParam tp;
	uint64_t type;
	BYTE data[0];
};