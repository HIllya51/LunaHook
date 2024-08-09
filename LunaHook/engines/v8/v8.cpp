#include "v8.h"
namespace
{
	constexpr auto magicsend = L"\x01LUNAFROMJS\x01";
	constexpr auto magicrecv = L"\x01LUNAFROMHOST\x01";
	bool hookClipboard()
	{
		HookParam hp;
		hp.address = (uintptr_t)SetClipboardData;
		hp.type = USING_STRING | NO_CONTEXT | CODEC_UTF16 | EMBED_ABLE | EMBED_BEFORE_SIMPLE;
		hp.text_fun = [](hook_stack *stack, HookParam *hp, uintptr_t *data, uintptr_t *split, size_t *len)
		{
			HGLOBAL hClipboardData = (HGLOBAL)stack->ARG2;
			auto text = (wchar_t *)GlobalLock(hClipboardData);
			if (startWith(text, magicsend))
			{
				text += wcslen(magicsend);
				auto spl = wcschr(text, L'\x03');
				strcpy(hp->name, wcasta(std::wstring(text, spl - text)).c_str());
				text = spl + 1;
				spl = wcschr(text, L'\x02');
				*split = std::stoi(std::wstring(text, spl - text));
				text = spl + 1;
				*data = (uintptr_t)text;
				*len = wcslen(text) * 2;
			}

			GlobalUnlock(hClipboardData);
		};
		hp.hook_after = [](hook_stack *s, void *data, size_t len)
		{
			std::wstring transwithfont = magicrecv;
			transwithfont += embedsharedmem->fontFamily;
			transwithfont += L'\x02';
			transwithfont += std::wstring((wchar_t *)data, len / 2);
			HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, transwithfont.size() * 2 + 2);
			auto pchData = (wchar_t *)GlobalLock(hClipboardData);
			wcscpy(pchData, (wchar_t *)transwithfont.c_str());
			GlobalUnlock(hClipboardData);
			s->ARG2 = (uintptr_t)hClipboardData;
		};
		return NewHook(hp, "nwjs/electron rpgmakermv/tyranoscript");
	}
}
namespace v8script
{
	HMODULE hmodule;

	typedef void (*RequestInterrupt_callback)(void *, void *);
#ifndef _WIN64

#define fnRequestInterrupt "?RequestInterrupt@Isolate@v8@@QAEXP6AXPAV12@PAX@Z1@Z"
#define fnNewFromUtf8v2 "?NewFromUtf8@String@v8@@SA?AV?$MaybeLocal@VString@v8@@@2@PAVIsolate@2@PBDW4NewStringType@2@H@Z"
#define fnNewFromUtf8v1 "?NewFromUtf8@String@v8@@SA?AV?$Local@VString@v8@@@2@PAVIsolate@2@PBDW4NewStringType@12@H@Z"
#define fnGetCurrentContext "?GetCurrentContext@Isolate@v8@@QAE?AV?$Local@VContext@v8@@@2@XZ"
#define fnCompilev1 "?Compile@Script@v8@@SA?AV?$Local@VScript@v8@@@2@V?$Handle@VString@v8@@@2@PAVScriptOrigin@2@@Z"
#define fnCompilev12 "?Compile@Script@v8@@SA?AV?$Local@VScript@v8@@@2@V?$Local@VString@v8@@@2@PAVScriptOrigin@2@@Z"
#define fnRunv1 "?Run@Script@v8@@QAE?AV?$Local@VValue@v8@@@2@XZ"
#define fnCompilev2 "?Compile@Script@v8@@SA?AV?$MaybeLocal@VScript@v8@@@2@V?$Local@VContext@v8@@@2@V?$Local@VString@v8@@@2@PAVScriptOrigin@2@@Z"
#define fnRunv2 "?Run@Script@v8@@QAE?AV?$MaybeLocal@VValue@v8@@@2@V?$Local@VContext@v8@@@2@@Z"

#else
#define fnRequestInterrupt "?RequestInterrupt@Isolate@v8@@QEAAXP6AXPEAV12@PEAX@Z1@Z"
#define fnNewFromUtf8v2 "?NewFromUtf8@String@v8@@SA?AV?$MaybeLocal@VString@v8@@@2@PEAVIsolate@2@PEBDW4NewStringType@2@H@Z"
#define fnNewFromUtf8v1 "?NewFromUtf8@String@v8@@SA?AV?$Local@VString@v8@@@2@PEAVIsolate@2@PEBDW4NewStringType@12@H@Z"
#define fnGetCurrentContext "?GetCurrentContext@Isolate@v8@@QEAA?AV?$Local@VContext@v8@@@2@XZ"
#define fnCompilev1 "?Compile@Script@v8@@SA?AV?$Local@VScript@v8@@@2@V?$Handle@VString@v8@@@2@PEAVScriptOrigin@2@@Z"
#define fnCompilev12 fnCompilev1
#define fnRunv1 "?Run@Script@v8@@QEAA?AV?$Local@VValue@v8@@@2@XZ"
#define fnCompilev2 "?Compile@Script@v8@@SA?AV?$MaybeLocal@VScript@v8@@@2@V?$Local@VContext@v8@@@2@V?$Local@VString@v8@@@2@PEAVScriptOrigin@2@@Z"
#define fnRunv2 "?Run@Script@v8@@QEAA?AV?$MaybeLocal@VValue@v8@@@2@V?$Local@VContext@v8@@@2@@Z"

#endif
	typedef void *(THISCALL *GetCurrentContextt)(void *, void *);
	typedef void *(THISCALL *Runt1)(void *, void *);
	typedef void *(THISCALL *Runt2)(void *, void *, void *);
	typedef void *(THISCALL *RequestInterruptt)(void *, RequestInterrupt_callback, void *);

	typedef void *(*NewFromUtf8t)(void *, void *, const char *, int, int);
	typedef void *(*Compilet)(void *, void *, void *, void *);
	RequestInterruptt RequestInterrupt;
	NewFromUtf8t NewFromUtf8 = 0, NewFromUtf8v2, NewFromUtf8v1;
	GetCurrentContextt GetCurrentContext;
	Compilet Compile;
	void *Run;
	void _interrupt_function(void *isolate, void *)
	{
		void *context;
		void *v8string;
		void *script;
		void *useless;
		ConsoleOutput("isolate %p", isolate);
		GetCurrentContext(isolate, &context);
		ConsoleOutput("context %p", context);
		if (context == 0)
			return;
		int is_packed = 0;
		if (auto moduleFileName = getModuleFilename())
		{

			AutoHandle hFile = CreateFile(moduleFileName.value().c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile)
			{
				LARGE_INTEGER fileSize;
				if (GetFileSizeEx(hFile, &fileSize))
				{
					if (fileSize.QuadPart > 1024 * 1024 * 200)
					{
						// 200mb
						is_packed = 1;
					}
				}
			}
		}
		NewFromUtf8(&v8string, isolate, FormatString(LoadResData(L"lunajspatch", L"JSSOURCE").c_str(), is_packed).c_str(), 1, -1);
		ConsoleOutput("v8string %p", v8string);
		if (v8string == 0)
			return;
		if (NewFromUtf8v1)
		{
			Compile(&script, v8string, 0, 0);
			ConsoleOutput("script %p", script);
			if (script == 0)
				return;
			((Runt1)Run)(script, &useless);
			ConsoleOutput("useless %p", useless);
		}
		else if (NewFromUtf8v2)
		{
			Compile(&script, context, v8string, 0);
			ConsoleOutput("script %p", script);
			if (script == 0)
				return;
			((Runt2)Run)(script, &useless, context);
			ConsoleOutput("useless %p", useless);
		}
	}
	bool v8runscript_isolate(void *isolate)
	{

		if (isolate == 0)
			return false;
		RequestInterrupt = (decltype(RequestInterrupt))GetProcAddress(hmodule, fnRequestInterrupt);

		NewFromUtf8v2 = (decltype(NewFromUtf8))GetProcAddress(hmodule, fnNewFromUtf8v2);
		NewFromUtf8v1 = (decltype(NewFromUtf8))GetProcAddress(hmodule, fnNewFromUtf8v1);

		GetCurrentContext = (decltype(GetCurrentContext))GetProcAddress(hmodule, fnGetCurrentContext);

		if (NewFromUtf8v1)
		{
			NewFromUtf8 = NewFromUtf8v1;
			Compile = (decltype(Compile))GetProcAddress(hmodule, fnCompilev1);
			if (!Compile)
				Compile = (decltype(Compile))GetProcAddress(hmodule, fnCompilev12);
			Run = (decltype(Run))GetProcAddress(hmodule, fnRunv1);
		}
		else if (NewFromUtf8v2)
		{
			NewFromUtf8 = NewFromUtf8v2;
			Compile = (decltype(Compile))GetProcAddress(hmodule, fnCompilev2);
			Run = (decltype(Run))GetProcAddress(hmodule, fnRunv2);
		}
		ConsoleOutput("%p %p", NewFromUtf8v1, NewFromUtf8v2);
		ConsoleOutput("%p %p %p %p", GetCurrentContext, NewFromUtf8, Compile, Run);
		if (!(GetCurrentContext && NewFromUtf8 && Compile && Run && RequestInterrupt))
			return false;

		if (RequestInterrupt == 0)
			return false;
		RequestInterrupt(isolate, _interrupt_function, 0);

		return true;
	}

	void v8runscript_isolate_bypass(hook_stack *stack, HookParam *hp, uintptr_t *data, uintptr_t *split, size_t *len)
	{

		hp->type = HOOK_EMPTY;
		hp->text_fun = nullptr;

		auto isolate = (void *)stack->ARG2; // 测试正确，且和v8::Isolate::GetCurrent结果相同
		v8runscript_isolate(isolate);
	}
	void *v8getcurrisolate(HMODULE hmod)
	{
#ifndef _WIN64
#define fnGetCurrent "?GetCurrent@Isolate@v8@@SAPAV12@XZ"
#define fnTryGetCurrent "?TryGetCurrent@Isolate@v8@@SAPAV12@XZ"
#else
#define fnGetCurrent "?GetCurrent@Isolate@v8@@SAPEAV12@XZ"
#define fnTryGetCurrent "?TryGetCurrent@Isolate@v8@@SAPEAV12@XZ"
#endif
		void *GetCurrent;
		GetCurrent = GetProcAddress(hmod, fnGetCurrent);
		if (!GetCurrent)
			GetCurrent = GetProcAddress(hmod, fnTryGetCurrent);
		if (!GetCurrent)
			return 0;
		auto isolate = ((void *(*)())GetCurrent)();
		return isolate;
	}
	bool v8runscript(HMODULE _hmodule)
	{
		auto isolate = v8getcurrisolate(_hmodule);
		if (isolate)
			return v8runscript_isolate(isolate);
#ifndef _WIN64
#define fnisolategetters {"?New@Number@v8@@SA?AV?$Local@VNumber@v8@@@2@PEAVIsolate@2@N@Z", "?New@Number@v8@@SA?AV?$Local@VNumber@v8@@@2@PAVIsolate@2@N@Z", "?NewFromUtf8@String@v8@@SA?AV?$Local@VString@v8@@@2@PAVIsolate@2@PBDW4NewStringType@12@H@Z"}
#else
#define fnisolategetters {"?New@Integer@v8@@SA?AV?$Local@VInteger@v8@@@2@PEAVIsolate@2@H@Z", "?New@Number@v8@@SA?AV?$Local@VNumber@v8@@@2@PEAVIsolate@2@N@Z", "?New@Number@v8@@SA?AV?$Local@VNumber@v8@@@2@PAVIsolate@2@N@Z", "?NewFromUtf8@String@v8@@SA?AV?$Local@VString@v8@@@2@PEAVIsolate@2@PEBDW4NewStringType@12@H@Z", "?Utf8Length@String@v8@@QEBAHPEAVIsolate@2@@Z"}
#endif
		bool succ = false;
		for (auto fnisolategetter : fnisolategetters)
		{
			auto isolategetter = GetProcAddress(_hmodule, fnisolategetter);
			if (!isolategetter)
				continue;
			hmodule = _hmodule;
			HookParam hp;
			hp.address = (uintptr_t)isolategetter;
			hp.text_fun = v8runscript_isolate_bypass;
			succ |= NewHook(hp, "isolategetter");
		}
		return succ;
	}
}
namespace
{
#ifndef _WIN64
#define v8StringLength "?Length@String@v8@@QBEHXZ"
#define v8StringWriteUtf8 "?WriteUtf8@String@v8@@QBEHPADHPAHH@Z"
#define v8StringUtf8Length "?Utf8Length@String@v8@@QBEHXZ"
#define v8StringWrite "?Write@String@v8@@QBEHPAGHHH@Z"
#define v8StringWriteIsolate "?Write@String@v8@@QBEHPAVIsolate@2@PAGHHH@Z"
#else
#define v8StringLength "?Length@String@v8@@QEBAHXZ"
#define v8StringWriteUtf8 "?WriteUtf8@String@v8@@QEBAHPEADHPEAHH@Z"
#define v8StringUtf8Length "?Utf8Length@String@v8@@QEBAHXZ"
#define v8StringWrite "?Write@String@v8@@QEBAHPEAGHHH@Z"
#define v8StringWriteIsolate "?Write@String@v8@@QEBAHPEAVIsolate@2@PEAGHHH@Z"
#endif
	uintptr_t WriteUtf8;
	uintptr_t Utf8Length;
	bool hookstring(HMODULE hm)
	{
		WriteUtf8 = (uintptr_t)GetProcAddress(hm, v8StringWriteUtf8);
		Utf8Length = (uintptr_t)GetProcAddress(hm, v8StringUtf8Length);
		if (WriteUtf8 == 0 || Utf8Length == 0)
			return false;

		HookParam hp;
		hp.type = USING_STRING | CODEC_UTF8;
		hp.text_fun =
			[](hook_stack *stack, HookParam *hp, uintptr_t *data, uintptr_t *split, size_t *len)
		{
			auto length = ((size_t(THISCALL *)(void *))Utf8Length)((void *)stack->THISCALLTHIS);
			if (!length)
				return;
			auto u8str = new char[length + 1];
			int writen;
			((size_t(THISCALL *)(void *, char *, int, int *, int))WriteUtf8)((void *)stack->THISCALLTHIS, u8str, length, &writen, 0);
			*data = (uintptr_t)u8str;
			*len = length;
		};
		hp.filter_fun = [](void *data, size_t *len, HookParam *hp)
		{
			if (strstr((char *)data, R"(\\?\)") != 0)
				return false; // 过滤路径
			return true;
		};
		bool succ = false;

		auto pv8StringLength = GetProcAddress(hm, v8StringLength);
		if (pv8StringLength)
		{

			hp.address = (uintptr_t)pv8StringLength;
			succ |= NewHook(hp, "v8::String::Length");
		}
		auto pv8StringWrite = GetProcAddress(hm, v8StringWrite);
		if (pv8StringWrite)
		{

			hp.address = (uintptr_t)pv8StringWrite;
			succ |= NewHook(hp, "v8::String::Write");
		}
		auto pv8StringWriteIsolate = GetProcAddress(hm, v8StringWriteIsolate);
		if (pv8StringWriteIsolate)
		{
			hp.address = (uintptr_t)pv8StringWriteIsolate;
			succ |= NewHook(hp, "v8::String::Write::isolate");
		}
		return succ;
	}
}
bool tryhookv8_internal(HMODULE hm)
{
	auto succ = hookstring(hm);
	if (!std::filesystem::exists(std::filesystem::path(getModuleFilename().value()).replace_filename("disable.clipboard")))
		if (v8script::v8runscript(hm))
			succ |= hookClipboard();
	return succ;
}
bool tryhookv8()
{
	for (const wchar_t *moduleName : {(const wchar_t *)NULL, L"node.dll", L"nw.dll"})
	{
		auto hm = GetModuleHandleW(moduleName);
		if (hm == 0)
			continue;
		bool ok = tryhookv8_internal(hm);
		if (ok)
			return true;
	}
	return false;
}
