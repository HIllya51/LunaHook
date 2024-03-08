

bool InsertRenpy3Hook();
bool InsertRenpyHook();


extern "C" __declspec(dllexport) const wchar_t* internal_renpy_call_host(const wchar_t* text,int split);


void hookrenpy(HMODULE module);
