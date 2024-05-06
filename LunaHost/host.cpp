#include "host.h"
typedef LONG NTSTATUS;
#include"yapi.hpp"
#include"Lang/Lang.h"
namespace
{
	class ProcessRecord
	{
	public:
		ProcessRecord(DWORD processId, HANDLE pipe) :
			pipe(pipe),
			mappedFile(OpenFileMappingW(FILE_MAP_READ, FALSE, (ITH_SECTION_ + std::to_wstring(processId)).c_str())),
			mappedFile2(OpenFileMappingW(FILE_MAP_READ|FILE_MAP_WRITE, FALSE, (EMBED_SHARED_MEM + std::to_wstring(processId)).c_str())),
			view(*(const TextHook(*)[MAX_HOOK])MapViewOfFile(mappedFile, FILE_MAP_READ, 0, 0, MAX_HOOK * sizeof(TextHook))), // jichi 1/16/2015: Changed to half to hook section sizem
			viewMutex(ITH_HOOKMAN_MUTEX_ + std::to_wstring(processId))
			
		{ 
			embedsharedmem=(EmbedSharedMem*)MapViewOfFile(mappedFile2, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, sizeof(EmbedSharedMem)); 
			//放到构造表里就不行，不知道为何。
		}

		~ProcessRecord()
		{
			UnmapViewOfFile(view);
			UnmapViewOfFile(embedsharedmem);
		}

		TextHook GetHook(uint64_t addr)
		{
			if (!view) return {};
			std::scoped_lock lock(viewMutex);
			for (auto hook : view) if (hook.address == addr) return hook;
			return {};
		}

		template <typename T>
		void Send(T data)
		{
			static_assert(sizeof(data) < PIPE_BUFFER_SIZE);
			std::thread([=]
			{
				WriteFile(pipe, &data, sizeof(data), DUMMY, nullptr);
			}).detach();
		}

		Host::HookEventHandler OnHookFound = [](HookParam hp, std::wstring text)
		{
			Host::AddConsoleOutput(std::wstring(hp.hookcode) + L": " + text);
		};
		
		
		EmbedSharedMem *embedsharedmem;
	private:
		HANDLE pipe;
		AutoHandle<> mappedFile;
		AutoHandle<> mappedFile2;
		const TextHook(&view)[MAX_HOOK];
		WinMutex viewMutex;
	};

	size_t HashThreadParam(ThreadParam tp) { return std::hash<int64_t>()(tp.processId + tp.addr) + std::hash<int64_t>()(tp.ctx + tp.ctx2); }
	Synchronized<std::unordered_map<ThreadParam, TextThread, Functor<HashThreadParam>>> textThreadsByParams;
	Synchronized<std::unordered_map<DWORD, ProcessRecord>> processRecordsByIds;

	Host::ProcessEventHandler OnConnect, OnDisconnect;
	Host::ThreadEventHandler OnCreate, OnDestroy;
	Host::ConsoleHandler OnConsole=0;
	Host::HookInsertHandler HookInsert=0;
	Host::EmbedCallback embedcallback=0;
	void RemoveThreads(std::function<bool(ThreadParam)> removeIf)
	{
		std::vector<TextThread*> threadsToRemove;
		for (auto& [tp, thread] : textThreadsByParams.Acquire().contents) if (removeIf(tp)) threadsToRemove.push_back(&thread);
		for (auto thread : threadsToRemove)
		{
			OnDestroy(*thread);
			textThreadsByParams->erase(thread->tp);
		}
	}
	BOOL Is64BitProcess(HANDLE ph)
	{
		BOOL f64bitProc = FALSE;
		if (detail::Is64BitOS())
		{
			f64bitProc = !(IsWow64Process(ph, &f64bitProc) && f64bitProc);
		}
		return f64bitProc;
	}
	void CreatePipe(int pid)
	{
		HANDLE
			hookPipe = CreateNamedPipeW((std::wstring(HOOK_PIPE)+std::to_wstring(pid)).c_str(), PIPE_ACCESS_INBOUND, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, PIPE_UNLIMITED_INSTANCES, 0, PIPE_BUFFER_SIZE, MAXDWORD, &allAccess),
			hostPipe = CreateNamedPipeW((std::wstring(HOST_PIPE)+std::to_wstring(pid)).c_str(), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, PIPE_UNLIMITED_INSTANCES, PIPE_BUFFER_SIZE, 0, MAXDWORD, &allAccess);
		HANDLE pipeAvailableEvent = CreateEventW(&allAccess, FALSE, FALSE, (std::wstring(PIPE_AVAILABLE_EVENT)+std::to_wstring(pid)).c_str());

		Host::AddConsoleOutput((std::wstring(PIPE_AVAILABLE_EVENT)+std::to_wstring(pid)));
		SetEvent(pipeAvailableEvent);
		std::thread([hookPipe,hostPipe,pipeAvailableEvent]
		{
			ConnectNamedPipe(hookPipe, nullptr);
			CloseHandle(pipeAvailableEvent);
			BYTE buffer[PIPE_BUFFER_SIZE] = {};
			DWORD bytesRead, processId;
			ReadFile(hookPipe, &processId, sizeof(processId), &bytesRead, nullptr);
			processRecordsByIds->try_emplace(processId, processId, hostPipe);
			OnConnect(processId);
			Host::AddConsoleOutput(FormatString(PROC_CONN,processId));
			//CreatePipe();

			while (ReadFile(hookPipe, buffer, PIPE_BUFFER_SIZE, &bytesRead, nullptr))
				switch (*(HostNotificationType*)buffer)
				{
				case HOST_NOTIFICATION_FOUND_HOOK:
				{
					auto info = *(HookFoundNotif*)buffer;
					auto OnHookFound = processRecordsByIds->at(processId).OnHookFound; 
					std::wstring wide = info.text;
					if (wide.size() > STRING) OnHookFound(info.hp, std::move(info.text));
					info.hp.type &= ~CODEC_UTF16;
					if (auto converted = StringToWideString((char*)info.text, info.hp.codepage))
						if (converted->size() > STRING) 
						{
							wcscpy_s(info.hp.hookcode,HOOKCODE_LEN, HookCode::Generate(info.hp, processId).c_str());
							OnHookFound(info.hp, std::move(converted.value()));
						}
					if (auto converted = StringToWideString((char*)info.text, info.hp.codepage = CP_UTF8))
						if (converted->size() > STRING)
						{
							wcscpy_s(info.hp.hookcode,HOOKCODE_LEN, HookCode::Generate(info.hp, processId).c_str());
							OnHookFound(info.hp, std::move(converted.value()));
						}
				}
				break;
				case HOST_NOTIFICATION_RMVHOOK:
				{
					auto info = *(HookRemovedNotif*)buffer;
					RemoveThreads([&](ThreadParam tp) { return tp.processId == processId && tp.addr == info.address; });
				}
				break;
				case HOST_NOTIFICATION_INSERTING_HOOK:
				{
					if(HookInsert){
						auto info = *(HookInsertingNotif*)buffer;
						auto addr=info.addr;
						std::wstring hc=processRecordsByIds->at(processId).GetHook(addr).hp.hookcode;
            			HookInsert(addr,hc);
					}
				}
				break;
				case HOST_NOTIFICATION_TEXT:
				{
					auto info = *(ConsoleOutputNotif*)buffer;
					Host::AddConsoleOutput(StringToWideString(info.message));
				}
				break;
				default:
				{
					auto data=(TextOutput_T*)buffer;
					auto length= bytesRead - sizeof(TextOutput_T);
					auto tp = data->tp;
					auto textThreadsByParams = ::textThreadsByParams.Acquire();
					auto thread = textThreadsByParams->find(tp);
					if (thread == textThreadsByParams->end())
					{
						try { thread = textThreadsByParams->try_emplace(tp, tp, processRecordsByIds->at(tp.processId).GetHook(tp.addr).hp).first; }
						catch (std::out_of_range) { continue; } // probably garbage data in pipe, try again
						OnCreate(thread->second);
					}
					thread->second.hp.type=data->type;
					thread->second.Push(data->data, length);

					if(embedcallback){
						auto & hp=thread->second.hp;
						if(hp.type&EMBED_ABLE){
							if (auto t=commonparsestring(data->data,length,&hp,Host::defaultCodepage)){
								auto text=t.value();
								if(text.size()){
									embedcallback(text,tp);
								}
							} 
						}
						
					}
				}
				break;
				}

			RemoveThreads([&](ThreadParam tp) { return tp.processId == processId; });
			OnDisconnect(processId);
			Host::AddConsoleOutput(FormatString(PROC_DISCONN,processId));
			processRecordsByIds->erase(processId);
		}).detach();
	}
}

namespace Host
{
	void Start(ProcessEventHandler Connect, ProcessEventHandler Disconnect, ThreadEventHandler Create, ThreadEventHandler Destroy, TextThread::OutputCallback Output,bool createconsole)
	{
		OnConnect = Connect;
		OnDisconnect = Disconnect;
		OnCreate = [Create](TextThread& thread) { Create(thread); thread.Start(); };
		OnDestroy = [Destroy](TextThread& thread) { thread.Stop(); Destroy(thread); };
		TextThread::Output = Output;

		textThreadsByParams->try_emplace(console, console, HookParam{}, CONSOLE);
		
		if(createconsole){
			OnCreate(GetThread(console));
			Host::AddConsoleOutput(ProjectHomePage);
		}

		//CreatePipe();
		
	}
	void StartEx(ProcessEventHandler Connect, ProcessEventHandler Disconnect, ThreadEventHandler Create, ThreadEventHandler Destroy, TextThread::OutputCallback Output,ConsoleHandler console,HookInsertHandler hookinsert,EmbedCallback embed){
		Start(Connect,Disconnect,Create,Destroy,Output,false);

		OnConsole=console;
		HookInsert=hookinsert;
		embedcallback=embed;
		Host::AddConsoleOutput(ProjectHomePage);
	}
	constexpr auto  PROCESS_INJECT_ACCESS=(
			PROCESS_CREATE_THREAD |
			PROCESS_QUERY_INFORMATION |
			PROCESS_VM_OPERATION |
			PROCESS_VM_WRITE |
			PROCESS_VM_READ);
	bool SafeInject(HANDLE process,const std::wstring &location){ 
//#ifdef _WIN64
#if 0
			BOOL invalidProcess = FALSE;
			IsWow64Process(process, &invalidProcess);
			if (invalidProcess) return AddConsoleOutput(NEED_32_BIT);
#endif
			bool succ=false;
			if (LPVOID remoteData = VirtualAllocEx(process, nullptr, (location.size() + 1) * sizeof(wchar_t), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE))
			{
				WriteProcessMemory(process, remoteData, location.c_str(), (location.size() + 1) * sizeof(wchar_t), nullptr);
				if (AutoHandle<> thread = CreateRemoteThread(process, nullptr, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, remoteData, 0, nullptr)){
					WaitForSingleObject(thread, INFINITE);
					succ=true;
				}
				else if (GetLastError() == ERROR_ACCESS_DENIED){
					 AddConsoleOutput(NEED_64_BIT); // https://stackoverflow.com/questions/16091141/createremotethread-access-denied
					 succ=false;
				}
				VirtualFreeEx(process, remoteData, 0, MEM_RELEASE);
				
			}
			return succ;
	}
	bool UnSafeInject(HANDLE process,const std::wstring &location){
		
		DWORD64 injectedDll;
		yapi::YAPICall LoadLibraryW(process, _T("kernel32.dll"), "LoadLibraryW");
		if(x64)injectedDll = LoadLibraryW.Dw64()(location.c_str());
		else injectedDll = LoadLibraryW(location.c_str());
		if(injectedDll)return true;
		return false; 
		 
	}
	bool CheckProcess(DWORD processId){
		if (processId == GetCurrentProcessId()) return false;

		WinMutex(ITH_HOOKMAN_MUTEX_ + std::to_wstring(processId));
		if (GetLastError() == ERROR_ALREADY_EXISTS){AddConsoleOutput(ALREADY_INJECTED); return false;}
		return true;
	}
	bool InjectDll(DWORD processId,const std::wstring locationX){
		AutoHandle<> process = OpenProcess(PROCESS_INJECT_ACCESS, FALSE, processId);
		if(!process)return false;
		bool proc64=Is64BitProcess(process);
		auto dllname=proc64?LUNA_HOOK_DLL_64:LUNA_HOOK_DLL_32;
		std::wstring location =locationX.size()?(locationX+L"\\"+dllname): std::filesystem::path(GetModuleFilename().value()).replace_filename(dllname);
		AddConsoleOutput(location);
		if(proc64==x64){
			return (SafeInject(process,location));
		}
		else{ 
			return (UnSafeInject(process,location));
		} 
	}
	bool CreatePipeAndCheck(DWORD processId){
		CreatePipe(processId);
		return CheckProcess(processId);
	}
	void InjectProcess(DWORD processId,const std::wstring locationX)
	{
	
		auto check=CreatePipeAndCheck(processId);
		if(check==false)return;

		std::thread([=]
		{
			if(InjectDll(processId,locationX))return ;
			AddConsoleOutput(INJECT_FAILED);
		}).detach();
	}

	void DetachProcess(DWORD processId)
	{
		auto &prs=processRecordsByIds.Acquire().contents;
		if(prs.find(processId)==prs.end())return;
		prs.at(processId).Send(HOST_COMMAND_DETACH);
	}

	void InsertHook(DWORD processId, HookParam hp)
	{
		auto &prs=processRecordsByIds.Acquire().contents;
		if(prs.find(processId)==prs.end())return;
		prs.at(processId).Send(InsertHookCmd(hp));
	}

	void RemoveHook(DWORD processId, uint64_t address)
	{
		auto &prs=processRecordsByIds.Acquire().contents;
		if(prs.find(processId)==prs.end())return;
		prs.at(processId).Send(RemoveHookCmd(address));
	}

	void FindHooks(DWORD processId, SearchParam sp, HookEventHandler HookFound)
	{
		auto &prs=processRecordsByIds.Acquire().contents;
		if(prs.find(processId)==prs.end())return;
		if (HookFound) prs.at(processId).OnHookFound = HookFound;
		prs.at(processId).Send(FindHookCmd(sp));
	}

	TextThread& GetThread(ThreadParam tp)
	{
		return textThreadsByParams->at(tp);
	}

	TextThread* GetThread(int64_t handle)
	{
		for (auto& [tp, thread] : textThreadsByParams.Acquire().contents) if (thread.handle == handle) return &thread;
		return nullptr;	
	}
	EmbedSharedMem* GetEmbedSharedMem(DWORD processId){
		auto &prs=processRecordsByIds.Acquire().contents;
		if(prs.find(processId)==prs.end())return 0;
		return prs.at(processId).embedsharedmem;
	}
	void AddConsoleOutput(std::wstring text)
	{
		if(OnConsole)
			OnConsole(std::move(text));
		else
			GetThread(console).AddSentence(std::move(text));
	}
}
