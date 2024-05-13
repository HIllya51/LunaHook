#include "host.h"
#define C_LUNA_API extern "C" __declspec(dllexport)
BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved)
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

typedef void (*ProcessEvent)(DWORD);
typedef void (*ThreadEvent)(wchar_t *, char *, ThreadParam);
typedef bool (*OutputCallback)(wchar_t *, char *, ThreadParam, const wchar_t *);
typedef void (*ConsoleHandler)(const wchar_t *);
typedef void (*HookInsertHandler)(uint64_t, const wchar_t *);
typedef void (*EmbedCallback)(const wchar_t *, ThreadParam);
#define XXXX char name[HOOK_NAME_SIZE];\
wchar_t hookcode[HOOKCODE_LEN];\
wcscpy_s(hookcode, HOOKCODE_LEN, thread.hp.hookcode);\
strcpy_s(name, HOOK_NAME_SIZE, thread.hp.name);
C_LUNA_API void Luna_Start(ProcessEvent Connect, ProcessEvent Disconnect, ThreadEvent Create, ThreadEvent Destroy, OutputCallback Output, ConsoleHandler console, HookInsertHandler hookinsert, EmbedCallback embed)
{
    Host::StartEx(
        Connect,
        Disconnect,
        [=](const TextThread &thread)
        {
            XXXX
            Create(hookcode, name, thread.tp);
        },
        [=](const TextThread &thread)
        {
            XXXX
            Destroy(hookcode, name, thread.tp);
        },
        [=](const TextThread &thread, std::wstring &output)
        {
            XXXX
            return Output(hookcode, name, thread.tp, output.c_str());
        },
        [=](const std::wstring &output)
        {
            console(output.c_str());
        },
        [=](uint64_t addr, const std::wstring &output)
        {
            hookinsert(addr, output.c_str());
        },
        [=](const std::wstring &output, const ThreadParam &tp)
        {
            embed(output.c_str(), tp);
        });
}
C_LUNA_API void Luna_Inject(DWORD pid, LPCWSTR basepath)
{
    Host::InjectProcess(pid, basepath);
}
C_LUNA_API bool Luna_CreatePipeAndCheck(DWORD pid)
{
    return Host::CreatePipeAndCheck(pid);
}
C_LUNA_API void Luna_Detach(DWORD pid)
{
    Host::DetachProcess(pid);
}

C_LUNA_API void Luna_Settings(int flushDelay, bool filterRepetition, int defaultCodepage, int maxBufferSize, int maxHistorySize)
{
    TextThread::flushDelay = flushDelay;
    TextThread::filterRepetition = filterRepetition;
    Host::defaultCodepage = defaultCodepage;
    TextThread::maxBufferSize = maxBufferSize;
    TextThread::maxHistorySize=maxHistorySize;
}

C_LUNA_API bool Luna_InsertHookCode(DWORD pid, LPCWSTR hookcode)
{
    auto hp = HookCode::Parse(hookcode);
    if (hp)
        Host::InsertHook(pid, hp.value());
    return hp.has_value();
}
C_LUNA_API wchar_t* Luna_QueryThreadHistory(ThreadParam tp){
    auto s=Host::GetThread(tp).storage.Acquire();
    auto str=(wchar_t*)malloc(sizeof(wchar_t)*(s->size()+1));
    wcscpy(str,s->c_str());
    return str;
}
C_LUNA_API void Luna_FreePtr(void* ptr){
    free(ptr);
}
C_LUNA_API void Luna_RemoveHook(DWORD pid, uint64_t addr)
{
    Host::RemoveHook(pid, addr);
}
struct simplehooks
{
    wchar_t hookcode[HOOKCODE_LEN];
    wchar_t *text;
    simplehooks() : text(0){};
};
typedef void (*findhookcallback_t)(wchar_t *hookcode, const wchar_t *text);
C_LUNA_API void Luna_FindHooks(DWORD pid, SearchParam sp, findhookcallback_t findhookcallback)
{
    Host::FindHooks(pid, sp, [=](HookParam hp, std::wstring text)
                    {
                            wchar_t hookcode[HOOKCODE_LEN];
                            wcscpy_s(hookcode,HOOKCODE_LEN, hp.hookcode);
                            findhookcallback(hookcode,text.c_str()); });
}
C_LUNA_API void Luna_EmbedSettings(DWORD pid, UINT32 waittime, UINT8 fontCharSet, bool fontCharSetEnabled, wchar_t *fontFamily, UINT32 spaceadjustpolicy, UINT32 keeprawtext, bool fastskipignore)
{
    auto sm = Host::GetEmbedSharedMem(pid);
    if (!sm)
        return;
    sm->waittime = waittime;
    sm->fontCharSet = fontCharSet;
    sm->fontCharSetEnabled = fontCharSetEnabled;
    wcscpy_s(sm->fontFamily, 100, fontFamily);
    sm->spaceadjustpolicy = spaceadjustpolicy;
    sm->keeprawtext = keeprawtext;
    sm->fastskipignore = fastskipignore;
}
C_LUNA_API bool Luna_checkisusingembed(DWORD pid, uint64_t address, uint64_t ctx1, uint64_t ctx2)
{
    auto sm = Host::GetEmbedSharedMem(pid);
    if (!sm)
        return false;
    for (int i = 0; i < 10; i++)
    {
        if (sm->use[i])
        {
            if ((sm->addr[i] == address) && (sm->ctx1[i] == ctx1) && (sm->ctx2[i] == ctx2))
                return true;
        }
    }
    return false;
}
C_LUNA_API void Luna_useembed(DWORD pid, uint64_t address, uint64_t ctx1, uint64_t ctx2, bool use)
{
    auto sm = Host::GetEmbedSharedMem(pid);
    if (!sm)
        return;
    sm->codepage = Host::defaultCodepage;
    for (int i = 0; i < 10; i++)
    {
        if (sm->use[i])
        {
            if ((sm->addr[i] == address) && (sm->ctx1[i] == ctx1) && (sm->ctx2[i] == ctx2))
            {
                if (use == false)
                {
                    sm->addr[i] = sm->ctx1[i] = sm->ctx2[i] = sm->use[i] = 0;
                }
            }
        }
    }
    if (use)
    {
        for (int i = 0; i < 10; i++)
        {
            if (sm->use[i] == 0)
            {
                sm->use[i] = 1;
                sm->addr[i] = address;
                sm->ctx1[i] = ctx1;
                sm->ctx2[i] = ctx2;
                break;
            }
        }
    }
}

inline UINT64 djb2_n2(const unsigned char *str, size_t len, UINT64 hash = 5381)
{
    int i = 0;
    while (len--)
    {
        hash = ((hash << 5) + hash) + (*str++); // hash * 33 + c
    }
    return hash;
}
C_LUNA_API void Luna_embedcallback(DWORD pid, LPCWSTR text, LPCWSTR trans)
{
    auto sm = Host::GetEmbedSharedMem(pid);
    if (!sm)
        return;
    wcscpy_s(sm->text, 1000, trans);
    char eventname[1000];
    sprintf(eventname, LUNA_EMBED_notify_event, pid, djb2_n2((const unsigned char *)(text), wcslen(text) * 2));
    win_event event1(eventname);
    event1.signal(true);
}