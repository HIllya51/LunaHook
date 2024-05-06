#include"cef.h"
typedef wchar_t char16;

typedef struct _cef_string_wide_t {
	wchar_t* str;
	size_t length;
	void (*dtor)(wchar_t* str);
} cef_string_wide_t;

typedef struct _cef_string_utf8_t {
	char* str;
	size_t length;
	void (*dtor)(char* str);
} cef_string_utf8_t;

typedef struct _cef_string_utf16_t {
	char16* str;
	size_t length;
	void (*dtor)(char16* str);
} cef_string_utf16_t;
static void hook_cef_string_utf16_t(hook_stack* stack,  HookParam *hp, uintptr_t* data, uintptr_t* split, size_t* len)
{
	if (auto p = (_cef_string_utf16_t*)stack->stack[1]) {
		*data = (DWORD)p->str;
		*len = p->length; // for widechar

		auto s = stack->ecx;
		for (int i = 0; i < 0x10; i++) // traverse pointers until a non-readable address is met
			if (s && !::IsBadReadPtr((LPCVOID)s, sizeof(DWORD)))
				s = *(DWORD*)s;
			else
				break;
		if (!s)
			s = hp->address;
		if (hp->type & USING_SPLIT) *split = s;
	}
}
static void hook_cef_string_wide_t(hook_stack* stack,  HookParam *hp, uintptr_t* data, uintptr_t* split, size_t* len)
{
	if (auto p = (_cef_string_wide_t*)stack->stack[1]) {
		*data = (DWORD)p->str;
		*len = p->length; // for widechar

		auto s = stack->ecx;
		for (int i = 0; i < 0x10; i++) // traverse pointers until a non-readable address is met
			if (s && !::IsBadReadPtr((LPCVOID)s, sizeof(DWORD)))
				s = *(DWORD*)s;
			else
				break;
		if (!s)
			s = hp->address;
		if (hp->type & USING_SPLIT) *split = s;
	}
}
static void hook_cef_string_utf8_t(hook_stack* stack,  HookParam *hp, uintptr_t* data, uintptr_t* split, size_t* len)
{
	if (auto p = (_cef_string_utf8_t*)stack->stack[1]) {
		*data = (DWORD)p->str;
		*len = p->length; // for widechar

		auto s = stack->ecx;
		for (int i = 0; i < 0x10; i++) // traverse pointers until a non-readable address is met
			if (s && !::IsBadReadPtr((LPCVOID)s, sizeof(DWORD)))
				s = *(DWORD*)s;
			else
				break;
		if (!s)
			s = hp->address;
		if (hp->type & USING_SPLIT) *split = s;
	}
}
bool InsertlibcefHook(HMODULE module)
{
	if (!module)return false; 
	bool ret = false;
	

	struct libcefFunction { // argument indices start from 0 for SpecialHookMonoString, otherwise 1
		const char* functionName;
		size_t textIndex; // argument index
		short lengthIndex; // argument index
		unsigned long hookType; // HookParam type
		void* text_fun; // HookParam::text_fun_t
	};  

	HookParam hp;
	const libcefFunction funcs[] = {
		{"cef_string_utf8_set",1,0,USING_STRING | CODEC_UTF8 | NO_CONTEXT,NULL}, //ok
		{"cef_string_utf8_to_utf16",1,0,USING_STRING | CODEC_UTF8 | NO_CONTEXT,NULL},
		{"cef_string_utf8_to_wide",1,0,USING_STRING | CODEC_UTF8 | NO_CONTEXT,NULL}, //ok
		{"cef_string_utf8_clear",0,0,USING_STRING | CODEC_UTF8 | NO_CONTEXT,hook_cef_string_utf8_t}, 

		{"cef_string_utf16_set",1,0,USING_STRING|CODEC_UTF16 | NO_CONTEXT,NULL}, //ok
		{"cef_string_utf16_clear",0,0,USING_STRING|CODEC_UTF16,hook_cef_string_utf16_t},//ok
		{"cef_string_utf16_to_utf8",1,0,USING_STRING|CODEC_UTF16 | NO_CONTEXT,NULL},//ok
		{"cef_string_utf16_to_wide",1,0,USING_STRING|CODEC_UTF16 | NO_CONTEXT,NULL},

		{"cef_string_ascii_to_utf16",1,0,USING_STRING | NO_CONTEXT,NULL},
		{"cef_string_ascii_to_wide",1,0,USING_STRING | NO_CONTEXT,NULL},

		{"cef_string_wide_set",1,0,USING_STRING | CODEC_UTF16 | NO_CONTEXT,NULL},//ok
		{"cef_string_wide_to_utf16",1,0,USING_STRING| CODEC_UTF16 | NO_CONTEXT,NULL},
		{"cef_string_wide_to_utf8",1,0,USING_STRING | CODEC_UTF16 | NO_CONTEXT,NULL}, 
		{"cef_string_wide_clear",0,0,USING_STRING|CODEC_UTF16,hook_cef_string_wide_t} 
	};
	for (auto func : funcs) {
		if (FARPROC addr = ::GetProcAddress(module, func.functionName)) {
			if (addr == 0)continue;
			hp.address = (DWORD)addr;
			hp.type = func.hookType; 
			hp.offset = func.textIndex * 4;
			hp.length_offset = func.lengthIndex * 4;
			hp.text_fun = (decltype(hp.text_fun))func.text_fun;
			ConsoleOutput("libcef: INSERT");
			ret|=NewHook(hp, func.functionName);
		}
	}

	if (!ret)
		ConsoleOutput("libcef: failed to find function address");
	return ret;
}
bool libcefhook(HMODULE module) {
	//https://vndb.org/v12297
	//魔降ル夜ノ凜 Animation ダウンロード版

	if (module == 0)return false;
	auto [minAddress, maxAddress] = Util::QueryModuleLimits(module);
	ConsoleOutput("check v8libcefhook %p %p", minAddress,maxAddress);
	const BYTE bytes[] = {
		0x50,
		0x51,
		0x52,
		0x57,
		0xff,0xd6,
		0x83,0xc4,0x10,
		0x8b,0x4d,XX,
		0x89,0xc6,
		0x31,0xe9,
		0xe8,XX4,
		0x89,0xF0,
		0x83,0xC4,0x18,
		0x5e,
		0x5f,
		0x5d,
		0xc3

	};
	auto addr = reverseFindBytes(bytes, sizeof(bytes), minAddress, maxAddress);

	HookParam hp; 
	hp.address = addr+6; 
	hp.offset=get_stack(1); 
	hp.filter_fun=[] (void* data, uintptr_t * size, HookParam*) {
		std::wstring s = L""; 
		int i = 0;
		for (; i < *size /2; i++) {
			auto c = ((LPWSTR)data)[i];
			if (c == L'[') {
				break;
			}
			else {
				s += c;
			}
		}
		strReplace(s,L"<br>",L"\n");
		static std::wstring last;
		if(s==last)return false;
		last=s;
		return write_string_overwrite(data,size,s);
	};
	hp.type = USING_STRING | CODEC_UTF16|NO_CONTEXT; 
	ConsoleOutput("v8libcefhook %p", addr);

	return NewHook(hp, "v8libcefhook");

}
bool cef::attach_function(){
	auto hm = GetModuleHandleW(L"libcef.dll");
	
	//InsertlibcefHook(hm);
		 
	return libcefhook(hm);
}