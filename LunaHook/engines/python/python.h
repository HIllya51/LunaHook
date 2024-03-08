

bool InsertRenpy3Hook();
bool InsertRenpyHook();


extern "C" __declspec(dllexport) const wchar_t* internal_renpy_call_host(const wchar_t* text,int split);

typedef enum {PyGILState_LOCKED, PyGILState_UNLOCKED}
    PyGILState_STATE;
typedef PyGILState_STATE (*PyGILState_Ensure_t)(void);
typedef void (*PyGILState_Release_t)(PyGILState_STATE);
typedef int (*PyRun_SimpleString_t)(const char *command);

void LoadPyRun(HMODULE module);
void PyRunScript(const char* script);
void patchfontfunction();
void hook_internal_renpy_call_host();
void hookrenpy(HMODULE module);
inline PyRun_SimpleString_t PyRun_SimpleString;
inline PyGILState_Release_t PyGILState_Release;
inline PyGILState_Ensure_t PyGILState_Ensure;
extern const char* Renpy_hook_text;
extern const char* Renpy_hook_font;
