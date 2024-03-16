#include"types.h"
#include"main.h"
#include"v8.h"
#ifndef _WIN64
#define arg2 stack[2]
#else
#define arg2 rdx
#endif
namespace{
    
	bool hookClipboard(){
		HookParam hp;
		hp.address=(uintptr_t)SetClipboardData;
		hp.type= USING_STRING|CODEC_UTF16|EMBED_ABLE|EMBED_BEFORE_SIMPLE;
		hp.text_fun=[](hook_stack* stack, HookParam *hp, uintptr_t* data, uintptr_t* split, size_t* len){
			HGLOBAL hClipboardData=(HGLOBAL)stack->arg2;
			*data=(uintptr_t)GlobalLock(hClipboardData);
			*len=wcslen((wchar_t*)*data)*2;
			GlobalUnlock(hClipboardData); 
		};
		hp.hook_after=[](hook_stack*s,void* data, size_t len){
			HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, len +2);
			auto pchData = (wchar_t*)GlobalLock(hClipboardData);
			wcscpy(pchData, (wchar_t*)data);
			GlobalUnlock(hClipboardData); 
			s->arg2=(uintptr_t)hClipboardData;
		};
		return NewHook(hp,"hookClipboard");
	}
}
namespace v8script{
HMODULE hmodule;

typedef void(*RequestInterrupt_callback)(void*, void*);
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
typedef void*(__thiscall *GetCurrentContextt)(void*, void*);

typedef void*(__thiscall*Runt1)(void*,void*);
typedef void*(__thiscall*Runt2)(void*,void*,void*);

typedef void*(__thiscall *RequestInterruptt)(void*, RequestInterrupt_callback, void*);
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
typedef void*(*GetCurrentContextt)(void*, void*);
typedef void*(*Runt1)(void*,void*);
typedef void*(*Runt2)(void*,void*,void*);

typedef void*(*RequestInterruptt)(void*, RequestInterrupt_callback, void*);

#endif
typedef void*(*NewFromUtf8t)(void*, void*, const char*, int, int) ;
typedef void*(*Compilet)(void*, void*, void*, void*);
RequestInterruptt RequestInterrupt;
NewFromUtf8t NewFromUtf8=0,NewFromUtf8v2,NewFromUtf8v1;
GetCurrentContextt GetCurrentContext ;
Compilet Compile;
void* Run;
bool v8runscript_isolate(void* isolate){
	
	if(isolate==0)return false;
	RequestInterrupt= (decltype(RequestInterrupt))GetProcAddress(hmodule, fnRequestInterrupt);
	
	NewFromUtf8v2 = (decltype(NewFromUtf8))GetProcAddress(hmodule, fnNewFromUtf8v2);
	NewFromUtf8v1 = (decltype(NewFromUtf8))GetProcAddress(hmodule, fnNewFromUtf8v1);

	GetCurrentContext = (decltype(GetCurrentContext))GetProcAddress(hmodule, fnGetCurrentContext);
		
	if(NewFromUtf8v1)
	{
		NewFromUtf8=NewFromUtf8v1;
		Compile = (decltype(Compile))GetProcAddress(hmodule, fnCompilev1);
		if(!Compile) Compile=(decltype(Compile))GetProcAddress(hmodule, fnCompilev12);
		Run = (decltype(Run))GetProcAddress(hmodule, fnRunv1);
	}
	else if(NewFromUtf8v2)
	{
		NewFromUtf8=NewFromUtf8v2;
		Compile = (decltype(Compile))GetProcAddress(hmodule, fnCompilev2);
		Run = (decltype(Run))GetProcAddress(hmodule, fnRunv2);
	}
	ConsoleOutput("%p %p",NewFromUtf8v1,NewFromUtf8v2);
	ConsoleOutput("%p %p %p %p",GetCurrentContext, NewFromUtf8, Compile, Run);
	if(!(GetCurrentContext && NewFromUtf8 && Compile && Run && RequestInterrupt))return false;
		
	if(RequestInterrupt==0)return false;
	RequestInterrupt(isolate,+[](void*isolate,void*){
		struct TimerDeleter { void operator()(HANDLE h) { DeleteTimerQueueTimer(NULL, h, INVALID_HANDLE_VALUE); } };
		void* context;
		void* v8string;
		void* script;
		void* useless;
		ConsoleOutput("isolate %p",isolate);
		GetCurrentContext(isolate,&context);
		ConsoleOutput("context %p",context);
		if(context==0)return;
		NewFromUtf8(&v8string,isolate,LoadResData(L"lunajspatch",L"JSSOURCE").c_str(),1,-1);
		ConsoleOutput("v8string %p",v8string);
		if(v8string==0)return;
		if(NewFromUtf8v1)
		{	
			Compile(&script,v8string,0,0);
			ConsoleOutput("script %p",script);
			if(script==0)return;
			((Runt1)Run)(script,&useless);
		}
		else if(NewFromUtf8v2)
		{	
			Compile(&script,context,v8string,0);
			ConsoleOutput("script %p",script);
			if(script==0)return ;
			((Runt2)Run)(script,&useless,context);
			ConsoleOutput("useless %p",useless);
		}

	},0);
	
	return true;
}

void v8runscript_isolate_bypass(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    
    hp->type=HOOK_EMPTY;hp->text_fun=nullptr;
#ifndef _WIN64
#define isolatearg stack[2]
#else
#define isolatearg rdx
#endif
	auto isolate=(void*)stack->isolatearg;//测试正确，且和v8::Isolate::GetCurrent结果相同
    
    v8runscript_isolate(isolate);
}
void* v8getcurrisolate(HMODULE hmod){
#ifndef _WIN64
#define fnGetCurrent "?GetCurrent@Isolate@v8@@SAPAV12@XZ"
#define fnTryGetCurrent "?TryGetCurrent@Isolate@v8@@SAPAV12@XZ"
#else
#define fnGetCurrent "?GetCurrent@Isolate@v8@@SAPEAV12@XZ"
#define fnTryGetCurrent "?TryGetCurrent@Isolate@v8@@SAPEAV12@XZ"
#endif
    void* GetCurrent;
    GetCurrent = GetProcAddress(hmod, fnGetCurrent);
    if ( !GetCurrent )
      GetCurrent = GetProcAddress(hmod, fnTryGetCurrent);
    if(!GetCurrent)return 0;
    auto isolate=((void*(*)())GetCurrent)();
    return isolate;
}
bool v8runscript(HMODULE _hmodule){
    auto isolate=v8getcurrisolate(_hmodule);
    if(isolate) 
        return v8runscript_isolate(isolate);
#ifndef _WIN64
#define fnisolategetter "?NewFromUtf8@String@v8@@SA?AV?$Local@VString@v8@@@2@PAVIsolate@2@PBDW4NewStringType@12@H@Z"
#define fnisolategetter2 fnisolategetter
#else
#define fnisolategetter "?Utf8Length@String@v8@@QEBAHPEAVIsolate@2@@Z"
#define fnisolategetter2 "?NewFromUtf8@String@v8@@SA?AV?$Local@VString@v8@@@2@PEAVIsolate@2@PEBDW4NewStringType@12@H@Z"
#endif
	auto isolategetter=GetProcAddress(_hmodule,fnisolategetter);
	if(!isolategetter)
		 isolategetter=GetProcAddress(_hmodule,fnisolategetter2);
	if(!isolategetter)return false;

    hmodule=_hmodule;
    HookParam hp;
    hp.address=(uintptr_t)isolategetter;
    hp.text_fun=v8runscript_isolate_bypass;
    return NewHook(hp,"v8isolate");
      
}
}
namespace{
	bool hookstringlength(HMODULE hm){
        #ifndef _WIN64
        #define v8StringLength "?Length@String@v8@@QBEHXZ"
        #define v8StringWriteUtf8 "?WriteUtf8@String@v8@@QBEHPADHPAHH@Z"
        #define v8StringUtf8Length "?Utf8Length@String@v8@@QBEHXZ"
        #else
		#define v8StringLength "?Length@String@v8@@QEBAHXZ"
        #define v8StringWriteUtf8 "?WriteUtf8@String@v8@@QEBAHPEADHPEAHH@Z"
        #define v8StringUtf8Length "?Utf8Length@String@v8@@QEBAHXZ"
        #endif
        auto Length=GetProcAddress(hm,v8StringLength);
		static uintptr_t WriteUtf8;
		static uintptr_t Utf8Length;
		WriteUtf8=(uintptr_t)GetProcAddress(hm,v8StringWriteUtf8);
		Utf8Length=(uintptr_t)GetProcAddress(hm,v8StringUtf8Length);
		if(Length==0||WriteUtf8==0||Utf8Length==0)return false;
		HookParam hp;
		hp.address=(uintptr_t)Length;
		hp.type=USING_STRING|CODEC_UTF8;
		hp.text_fun=
			[](hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len)
			{
                #ifndef _WIN64
                auto length=((size_t(__thiscall*)(void*))Utf8Length)((void*)stack->ecx);
                #else
				auto length=((size_t(*)(void*))Utf8Length)((void*)stack->rcx);
                #endif
				if(!length)return;
				auto u8str=new char[length+1];
				int writen;
                #ifndef _WIN64
                ((size_t(__thiscall*)(void*,char*,int,int*,int))WriteUtf8)((void*)stack->ecx,u8str,length,&writen,0);
                #else
				((size_t(*)(void*,char*,int,int*,int))WriteUtf8)((void*)stack->rcx,u8str,length,&writen,0);
                #endif
				*data=(uintptr_t)u8str;
				*len=length;

			};
		hp.filter_fun=[](void* data, size_t* len, HookParam* hp){
			if(strstr((char*)data,R"(\\?\)")!=0)return false;//过滤路径
			return true;
		};
		return NewHook(hp,"v8::String::Length");
	}
}
bool tryhookv8(HMODULE hm){
    auto succ=hookstringlength(hm);
    if(v8script::v8runscript(hm))
		succ|= hookClipboard();
    return succ;
}