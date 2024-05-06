#pragma once

#include "textthread.h"
namespace Host
{
	using ConsoleHandler =std::function<void(std::wstring&)>;
	using ProcessEventHandler = std::function<void(DWORD)>;
	using ThreadEventHandler = std::function<void(TextThread&)>;
	using HookEventHandler = std::function<void(HookParam, std::wstring text)>;
	using HookInsertHandler= std::function<void(uint64_t,std::wstring&)>;
	using EmbedCallback= std::function<void(std::wstring&,ThreadParam&)>;
	void Start(ProcessEventHandler Connect, ProcessEventHandler Disconnect, ThreadEventHandler Create, ThreadEventHandler Destroy, TextThread::OutputCallback Output,bool createconsole=true);
	void StartEx(ProcessEventHandler Connect, ProcessEventHandler Disconnect, ThreadEventHandler Create, ThreadEventHandler Destroy, TextThread::OutputCallback Output,ConsoleHandler console,HookInsertHandler hookinsert,EmbedCallback embed);
	void InjectProcess(DWORD processId,const std::wstring locationX=L"");
	bool CreatePipeAndCheck(DWORD processId);

	void DetachProcess(DWORD processId);

	void InsertHook(DWORD processId, HookParam hp);
	void RemoveHook(DWORD processId, uint64_t address);
	void FindHooks(DWORD processId, SearchParam sp, HookEventHandler HookFound = {});
	EmbedSharedMem* GetEmbedSharedMem(DWORD pid);
	TextThread* GetThread(int64_t handle);
	TextThread& GetThread(ThreadParam tp);

	void AddConsoleOutput(std::wstring text);

	inline int defaultCodepage = SHIFT_JIS;

	constexpr ThreadParam console{ 0, 0, 0, 0 };
}
