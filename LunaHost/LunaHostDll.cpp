#include"host.h"
#include"hookcode.h"
#include"defs.h"
#include"winevent.hpp"
#define C_LUNA_API extern "C" __declspec(dllexport) 
BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: 
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
} 
  
static HANDLE HostMessageSender;
    
struct messagelist{ 
    bool read;
    int type;
    DWORD pid;
    char name[HOOK_NAME_SIZE];
	wchar_t hookcode[HOOKCODE_LEN];
    ThreadParam tp;
    wchar_t* stringptr;
    uint64_t addr;
    messagelist(int _t):read(false),type(_t),pid(0),tp({}),stringptr(nullptr),addr(0){}; 
    void sethp(const HookParam& hp){ 
        wcscpy_s(hookcode,HOOKCODE_LEN,hp.hookcode);
        strcpy_s(name,HOOK_NAME_SIZE,hp.name);
    }
    void setstring(const std::wstring& s){
        stringptr=new wchar_t[s.size()+1];
        wcscpy(stringptr,s.c_str());
    }
    ~messagelist(){
        DWORD _;
        WriteFile(HostMessageSender,this,sizeof(messagelist),&_,NULL);
    }
}; 
C_LUNA_API void Luna_Start( HANDLE* hRead ){
    CreatePipe(hRead,&HostMessageSender,NULL,0);
    Host::StartEx(
            [](DWORD pid){
                messagelist message(0);
                message.pid=pid;
            }, 
            [](DWORD pid){
                messagelist message(1);
                message.pid=pid; 
            }, 
            [](TextThread& thread) {
                messagelist message(2);
                message.sethp(thread.hp);
                message.tp=thread.tp;
            }, 
            [](TextThread& thread) {
                messagelist message(3);
                message.sethp(thread.hp);
                message.tp=thread.tp;
            }, 
            [](TextThread& thread, std::wstring& output){
                messagelist message(4);
                message.sethp(thread.hp);
                message.tp=thread.tp;
                message.setstring(output);
                return true;
            },
            [](std::wstring& output){
                messagelist message(5);
                message.setstring(output);
            },
            [](uint64_t addr,std::wstring& output){
                messagelist message(6);
                message.setstring(output);
                message.addr=addr;
            },
            [](std::wstring& output,ThreadParam& tp){
                messagelist message(7);
                message.setstring(output);
                message.tp=tp;
            });  
}
C_LUNA_API void Luna_Inject(DWORD pid,LPCWSTR basepath){
    Host::InjectProcess(pid,basepath);
}
C_LUNA_API bool Luna_CreatePipeAndCheck(DWORD pid){
    return Host::CreatePipeAndCheck(pid);
}
C_LUNA_API void Luna_Detach(DWORD pid){
    Host::DetachProcess(pid);
}

C_LUNA_API void Luna_cfree(void* ptr){
    delete ptr;
}

C_LUNA_API void Luna_Settings(int flushDelay,bool filterRepetition,int defaultCodepage,int maxBufferSize){
    TextThread::flushDelay=flushDelay;
    TextThread::filterRepetition=filterRepetition;
    Host::defaultCodepage=defaultCodepage;
    TextThread::maxBufferSize=maxBufferSize;
}

C_LUNA_API bool Luna_InsertHookCode(DWORD pid,LPCWSTR hookcode){
    auto hp = HookCode::Parse(hookcode);
    if(hp)
        Host::InsertHook(pid, hp.value());
    return hp.has_value();
}

C_LUNA_API void Luna_RemoveHook(DWORD pid,uint64_t addr){
    Host::RemoveHook(pid,addr);
}
struct simplehooks{
    wchar_t hookcode[HOOKCODE_LEN];
    wchar_t *text;
    simplehooks():text(0){};
};
C_LUNA_API void Luna_FindHooks(DWORD pid,SearchParam sp,HANDLE* hRead,int** pc){
     
    auto count=new int{0};
    *pc=count;
    HANDLE hWrite;
    CreatePipe(hRead,&hWrite,NULL,0);
    Host::FindHooks(pid,sp,[=](HookParam hp, std::wstring text) {
                        //if (std::regex_search(text, std::wregex(L"[\u3000-\ua000]"))) {
                            simplehooks sh;
                            wcscpy_s(sh.hookcode,HOOKCODE_LEN, hp.hookcode);
                            sh.text=new wchar_t[text.size()+1];
                            wcscpy(sh.text, text.c_str());
                            *count+=1;
                            if(0==WriteFile(hWrite,&sh,sizeof(sh),NULL,NULL))
                                CloseHandle(hWrite);
                    });  
}
C_LUNA_API void Luna_FindHooks_waiting(int* count){
    for (int lastSize = 0; *count == 0 || *count != lastSize; Sleep(2000)) lastSize = *count; 
    delete count;
}
C_LUNA_API void Luna_EmbedSettings(DWORD pid,UINT32 waittime,UINT8 fontCharSet,bool fontCharSetEnabled,wchar_t *fontFamily,UINT32 spaceadjustpolicy,UINT32 keeprawtext,bool fastskipignore){
    auto sm=Host::GetEmbedSharedMem(pid);
    if(!sm)return;
    sm->waittime=waittime;
    sm->fontCharSet=fontCharSet;
    sm->fontCharSetEnabled=fontCharSetEnabled;
    wcscpy_s(sm->fontFamily,100,fontFamily);
    sm->spaceadjustpolicy=spaceadjustpolicy;
    sm->keeprawtext=keeprawtext;
    sm->fastskipignore=fastskipignore;
}
C_LUNA_API bool Luna_checkisusingembed(DWORD pid,uint64_t address,uint64_t ctx1,uint64_t ctx2){
    auto sm=Host::GetEmbedSharedMem(pid);
    if(!sm)return false;
    for(int i=0;i<10;i++){
        if(sm->use[i]){
            if((sm->addr[i]==address)&&(sm->ctx1[i]==ctx1)&&(sm->ctx2[i]==ctx2))return true;
        }
    }
    return false;
}
C_LUNA_API void Luna_useembed(DWORD pid,uint64_t address,uint64_t ctx1,uint64_t ctx2,bool use){
    auto sm=Host::GetEmbedSharedMem(pid);
    if(!sm)return ;
    sm->codepage=Host::defaultCodepage;
    for(int i=0;i<10;i++){
        if(sm->use[i]){
            if((sm->addr[i]==address)&&(sm->ctx1[i]==ctx1)&&(sm->ctx2[i]==ctx2)){
                if(use==false){
                    sm->addr[i]=sm->ctx1[i]=sm->ctx2[i]=sm->use[i]=0;
                }
            }
        }
    }
    if(use){
        for(int i=0;i<10;i++){
            if(sm->use[i]==0){
                sm->use[i]=1;
                sm->addr[i]=address;
                sm->ctx1[i]=ctx1;
                sm->ctx2[i]=ctx2;
            }
        }
    } 
}

inline UINT64 djb2_n2(const unsigned char * str, size_t len, UINT64 hash =  5381)
{
  int i=0;
    while (len--){
        hash = ((hash << 5) + hash) + (*str++); // hash * 33 + c
    }
    return hash;
}
C_LUNA_API void Luna_embedcallback(DWORD pid,LPCWSTR text,LPCWSTR trans){
    auto sm=Host::GetEmbedSharedMem(pid);
    if(!sm)return;
    wcscpy_s(sm->text,1000,trans);
    char eventname[1000];
    sprintf(eventname,LUNA_EMBED_notify_event,pid,djb2_n2((const unsigned char*)(text),wcslen(text)*2));
    win_event event1(eventname); 
    event1.signal(true);
}