#pragma once

#include"memdbg/memsearch.h"
// util.h
// 8/23/2013 jichi

#define XX2 XX,XX       // WORD
#define XX4 XX2,XX2     // DWORD
#define XX8 XX4,XX4     // QWORD
enum : DWORD { X64_MAX_REL_ADDR = 0x00300000 };
enum : DWORD { MAX_REL_ADDR = 0x00300000 };

namespace{
static union {
  char text_buffer[0x1000];
  wchar_t wc_buffer[0x800];

};
DWORD buffer_index,
      buffer_length;
}


namespace Util {

#ifndef _WIN64
DWORD GetCodeRange(DWORD hModule,DWORD *low, DWORD *high);
DWORD FindCallAndEntryBoth(DWORD fun, DWORD size, DWORD pt, DWORD sig);
DWORD FindCallOrJmpRel(DWORD fun, DWORD size, DWORD pt, bool jmp);
DWORD FindCallOrJmpAbs(DWORD fun, DWORD size, DWORD pt, bool jmp);
DWORD FindCallBoth(DWORD fun, DWORD size, DWORD pt);
DWORD FindCallAndEntryAbs(DWORD fun, DWORD size, DWORD pt, DWORD sig);
DWORD FindCallAndEntryRel(DWORD fun, DWORD size, DWORD pt, DWORD sig);
DWORD FindImportEntry(DWORD hModule, DWORD fun);
#endif

bool CheckFile(LPCWSTR name);

bool SearchResourceString(LPCWSTR str);

std::pair<uintptr_t, uintptr_t> QueryModuleLimits(HMODULE module,uintptr_t addition=0x1000,DWORD protect=PAGE_EXECUTE);
std::vector<uintptr_t> SearchMemory(const void* bytes, short length, DWORD protect = PAGE_EXECUTE, uintptr_t minAddr = 0, uintptr_t maxAddr = -1ULL);
uintptr_t FindFunction(const char* function);

} // namespace Util


uintptr_t SafeFindEnclosingAlignedFunction(uintptr_t addr, uintptr_t range);
uintptr_t SafeFindBytes(LPCVOID pattern, size_t patternSize, uintptr_t lowerBound, uintptr_t upperBound); 
#ifndef _WIN64

ULONG _SafeMatchBytesInMappedMemory(LPCVOID pattern, DWORD patternSize, BYTE wildcard,
                                   ULONG start, ULONG stop, ULONG step);
ULONG SafeMatchBytesInGCMemory(LPCVOID pattern, DWORD patternSize);

std::vector<DWORD> findrelativecall(const BYTE* pattern ,int length,DWORD calladdress,DWORD start, DWORD end);
std::vector<DWORD> findxref_reverse_checkcallop(DWORD addr, DWORD from, DWORD to,BYTE op) ;
uintptr_t finddllfunctioncall(uintptr_t funcptr,uintptr_t start, uintptr_t end,WORD sig=0x15ff,bool reverse=false);
uintptr_t findfuncstart(uintptr_t addr,uintptr_t range=0x100);
#endif

uintptr_t find_pattern(const char* pattern,uintptr_t start,uintptr_t end);
uintptr_t reverseFindBytes(const BYTE* pattern, int length, uintptr_t start, uintptr_t end,int offset=0,bool checkalign=false);
std::vector<uintptr_t> findxref_reverse(uintptr_t addr, uintptr_t from, uintptr_t to);


namespace Engine{
bool isAddressReadable(const uintptr_t *p);
bool isAddressReadable(const char *p, size_t count = 1);
bool isAddressReadable(const wchar_t *p, size_t count = 1);
bool isAddressWritable(const uintptr_t *p);
bool isAddressWritable(const char *p, size_t count = 1);
bool isAddressWritable(const wchar_t *p, size_t count = 1);
inline bool isAddressReadable(const void *addr) { return isAddressReadable((const uintptr_t *)addr); }
inline bool isAddressReadable(uintptr_t addr) { return isAddressReadable((const void *)addr); }
inline bool isAddressWritable(const void *addr) { return isAddressWritable((const uintptr_t *)addr); }
inline bool isAddressWritable(uintptr_t addr) { return isAddressWritable((const void *)addr); }
}

void write_string_new(uintptr_t* data, size_t* len,const std::wstring& s);
void write_string_new(uintptr_t* data, size_t* len,const std::string& s);
bool write_string_overwrite(void* data, size_t* len,const std::wstring& s);
bool write_string_overwrite(void* data, size_t* len,const std::string& s);


struct WindowInfo {
  HWND handle;
  std::wstring title;
};
std::vector<WindowInfo>get_proc_windows();