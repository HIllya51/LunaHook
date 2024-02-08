// main.cc
// 8/24/2013 jichi
// Branch: LUNA_HOOK_DLL/main.cpp, rev 128
// 8/24/2013 TODO: Clean up this file

#include "main.h"
#include "defs.h"
#include "texthook.h"
#include "hookfinder.h"
#include "util.h"
#include "MinHook.h"
#include"hookcode.h"
#include"Lang/Lang.h"
void Hijack();
void detachall();
HMODULE hLUNAHOOKDLL;
WinMutex viewMutex;
EmbedSharedMem *embedsharedmem;
namespace
{
	AutoHandle<> hookPipe = INVALID_HANDLE_VALUE, 
				mappedFile = INVALID_HANDLE_VALUE,
				mappedFile3=INVALID_HANDLE_VALUE;
	TextHook(*hooks)[MAX_HOOK]; 
	int currentHook = 0;
}
bool DetourAttachedUserAddr=false;
bool hostconnected=false;
DWORD WINAPI Pipe(LPVOID)
{
	for (bool running = true; running; hookPipe = INVALID_HANDLE_VALUE)
	{
		DWORD count = 0;
		BYTE buffer[PIPE_BUFFER_SIZE] = {};
		AutoHandle<> hostPipe = INVALID_HANDLE_VALUE;
	
		while (!hostPipe || !hookPipe)
		{
			// WinMutex connectionMutex(CONNECTING_MUTEX, &allAccess);
			// std::scoped_lock lock(connectionMutex);
			WaitForSingleObject(AutoHandle<>(CreateEventW(&allAccess, FALSE, FALSE, (std::wstring(PIPE_AVAILABLE_EVENT)+std::to_wstring(GetCurrentProcessId())).c_str())), INFINITE);
			hostPipe = CreateFileW((std::wstring(HOST_PIPE)+std::to_wstring(GetCurrentProcessId())).c_str(), GENERIC_READ | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			hookPipe = CreateFileW((std::wstring(HOOK_PIPE)+std::to_wstring(GetCurrentProcessId())).c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		}
		DWORD mode = PIPE_READMODE_MESSAGE;
		SetNamedPipeHandleState(hostPipe, &mode, NULL, NULL);

		*(DWORD*)buffer = GetCurrentProcessId();
		WriteFile(hookPipe, buffer, sizeof(DWORD), &count, nullptr);

		ConsoleOutput(PIPE_CONNECTED);
		Hijack();
		hostconnected=true;
		while (running && ReadFile(hostPipe, buffer, PIPE_BUFFER_SIZE, &count, nullptr))
			switch (*(HostCommandType*)buffer)
			{
			case HOST_COMMAND_NEW_HOOK:
			{
				auto info = *(InsertHookCmd*)buffer;
				static int userHooks = 0;
				NewHook(info.hp, ("UserHook" + std::to_string(userHooks += 1)).c_str());
			}
			break;
			case HOST_COMMAND_REMOVE_HOOK:
			{
				auto info = *(RemoveHookCmd*)buffer;
				RemoveHook(info.address, 0);
			}
			break;
			case HOST_COMMAND_FIND_HOOK:
			{
				auto info = *(FindHookCmd*)buffer;
				if (*info.sp.text) SearchForText(info.sp.text, info.sp.codepage);
				else SearchForHooks(info.sp);
			}
			break;
			case HOST_COMMAND_DETACH:
			{
				running = false;
			}
			break;
			}
	}

	if(DetourAttachedUserAddr){
		hostconnected=false;
		return Pipe(0);
	}else{
			
		MH_Uninitialize();
		for (auto& hook : *hooks) hook.Clear();
		FreeLibraryAndExitThread(GetModuleHandleW(LUNA_HOOK_DLL), 0);
	}
}

void TextOutput(ThreadParam tp, TextOutput_T*buffer, int len)
{
	if (len < 0 || len > PIPE_BUFFER_SIZE - sizeof(tp)) ConsoleOutput(InvalidLength, len, tp.addr);
	buffer->tp=tp;
	WriteFile(hookPipe, buffer, sizeof(TextOutput_T) + len, DUMMY, nullptr);
}

void ConsoleOutput(LPCSTR text, ...)
{
	ConsoleOutputNotif buffer;
	va_list args;
	va_start(args, text);
	vsnprintf(buffer.message, MESSAGE_SIZE, text, args);
	WriteFile(hookPipe, &buffer, sizeof(buffer), DUMMY, nullptr);
}

void NotifyHookFound(HookParam hp,wchar_t*text)
{
	wcscpy_s(hp.hookcode,HOOKCODE_LEN, HookCode::Generate(hp, GetCurrentProcessId()).c_str());
	HookFoundNotif buffer(hp, text);
	WriteFile(hookPipe, &buffer, sizeof(buffer), DUMMY, nullptr);
}
void NotifyHookRemove(uint64_t addr, LPCSTR name)
{
	if (name) ConsoleOutput(REMOVING_HOOK, name);
	HookRemovedNotif buffer(addr);
	WriteFile(hookPipe, &buffer, sizeof(buffer), DUMMY, nullptr);
}
void NotifyHookInserting(uint64_t addr)
{
	HookInsertingNotif buffer(addr);
	WriteFile(hookPipe, &buffer, sizeof(buffer), DUMMY, nullptr);
}
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD fdwReason, LPVOID)
{
	switch (fdwReason) 
	{
	case DLL_PROCESS_ATTACH:
	{
		hLUNAHOOKDLL=hModule;
		viewMutex = WinMutex(ITH_HOOKMAN_MUTEX_ + std::to_wstring(GetCurrentProcessId()), &allAccess);
		if (GetLastError() == ERROR_ALREADY_EXISTS) return FALSE;
		DisableThreadLibraryCalls(hModule);

		auto createfm=[](AutoHandle<> &handle,void**ptr,DWORD sz,std::wstring&name ){ 
			handle=CreateFileMappingW(INVALID_HANDLE_VALUE, &allAccess, PAGE_EXECUTE_READWRITE, 0, sz, (name).c_str());
			*ptr=MapViewOfFile(handle, FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, 0, 0, sz);
			memset(*ptr, 0, sz);
		};
		createfm(mappedFile,(void**)&hooks,MAX_HOOK * sizeof(TextHook),ITH_SECTION_ + std::to_wstring(GetCurrentProcessId()));
		createfm(mappedFile3,(void**)&embedsharedmem,  sizeof(EmbedSharedMem),EMBED_SHARED_MEM + std::to_wstring(GetCurrentProcessId()));
		  

		MH_Initialize();

		CloseHandle(CreateThread(nullptr, 0, Pipe, nullptr, 0, nullptr)); // Using std::thread here = deadlock
	} 
	break;
	case DLL_PROCESS_DETACH:
	{
		MH_Uninitialize();
		detachall( );
		UnmapViewOfFile(hooks);
		UnmapViewOfFile(embedsharedmem);
	}
	break;
	}
	return TRUE;
}
bool NewHook(HookParam hp, LPCSTR lpname)
{
	if (++currentHook >= MAX_HOOK){
		ConsoleOutput(TOO_MANY_HOOKS);
		return false;
	}
	if (lpname && *lpname) strncpy_s(hp.name, lpname, HOOK_NAME_SIZE - 1);
	ConsoleOutput(INSERTING_HOOK, hp.name);
	RemoveHook(hp.address, 0);
	
	wcscpy_s(hp.hookcode,HOOKCODE_LEN,HookCode::Generate(hp, GetCurrentProcessId()).c_str());
	if (!(*hooks)[currentHook].Insert(hp))
	{
		ConsoleOutput(InsertHookFailed,WideStringToString(hp.hookcode).c_str());
		(*hooks)[currentHook].Clear();
		return false;
	}
	else{
		NotifyHookInserting(hp.address);
		return true;
	}
}

void RemoveHook(uint64_t addr, int maxOffset)
{
	for (auto& hook : *hooks) if (abs((long long)(hook.address - addr)) <= maxOffset) return hook.Clear();
}
std::string LoadResData(LPCWSTR pszResID,LPCWSTR _type) 
{ 
	HMODULE hModule=hLUNAHOOKDLL;
	HRSRC hRsrc = ::FindResourceW (hModule, pszResID,_type); 
	if (!hRsrc)  
		return 0;  
	DWORD len = SizeofResource(hModule, hRsrc);  
	BYTE* lpRsrc = (BYTE*)LoadResource(hModule, hRsrc);  
	if (!lpRsrc)  
		return 0;  
	HGLOBAL m_hMem = GlobalAlloc(GMEM_FIXED, len);  
	BYTE* pmem = (BYTE*)GlobalLock(m_hMem);  
	memcpy(pmem,lpRsrc,len);  
	auto data=std::string((char*)pmem,len);
	GlobalUnlock(m_hMem); 
	GlobalFree(m_hMem);
	FreeResource(lpRsrc); 
	return data;
}  