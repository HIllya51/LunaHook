#ifndef __LUNA_STRINGUILTS_H
#define __LUNA_STRINGUILTS_H

enum { VNR_TEXT_CAPACITY = 1500 }; // estimated max number of bytes allowed in VNR, slightly larger than VNR's text limit (1000)

template<class StringT>
StringT stolower(StringT s){
	std::transform(s.begin(), s.end(), s.begin(), tolower);
	return s;
}

LPCSTR reverse_search_begin(const char *s, int maxsize = VNR_TEXT_CAPACITY);

bool all_ascii(const char *s, int maxsize = VNR_TEXT_CAPACITY);
bool all_ascii(const wchar_t *s, int maxsize = VNR_TEXT_CAPACITY);
void strReplace(std::string& str, const std::string& oldStr, const std::string& newStr);
void strReplace(std::wstring& str, const std::wstring& oldStr, const std::wstring& newStr);
std::vector<std::string> strSplit(const std::string& s, const std::string&  delim);
std::vector<std::wstring> strSplit(const std::wstring& s, const std::wstring&  delim);
bool startWith(const std::string& s,const std::string& s2);
bool startWith(const std::wstring& s,const std::wstring &s2);
bool endWith(const std::string& s,const std::string& s2);
bool endWith(const std::wstring& s,const std::wstring& s2);

std::wstring utf32_to_utf16(void* data,size_t size);
std::string WideStringToString(const std::wstring& text,UINT cp=CP_UTF8);
std::wstring StringToWideString(const std::string& text);
std::optional<std::wstring> StringToWideString(const std::string& text, UINT encoding);

size_t u32strlen(uint32_t* data);
inline bool disable_mbwc=false;
inline bool disable_wcmb=false;

inline void Trim(std::wstring& text)
{
	text.erase(text.begin(), std::find_if_not(text.begin(), text.end(), iswspace));
	text.erase(std::find_if_not(text.rbegin(), text.rend(), iswspace).base(), text.end());
}

template <typename T> inline auto FormatArg(T arg) { return arg; }
template <typename C> inline auto FormatArg(const std::basic_string<C>& arg) { return arg.c_str(); }

#pragma warning(push)
#pragma warning(disable: 4996)
template <typename... Args>
inline std::string FormatString(const char* format, const Args&... args)
{
	std::string buffer(snprintf(nullptr, 0, format, FormatArg(args)...), '\0');
	sprintf(buffer.data(), format, FormatArg(args)...);
	return buffer;
}

template <typename... Args>
inline std::wstring FormatString(const wchar_t* format, const Args&... args)
{
	std::wstring buffer(_snwprintf(nullptr, 0, format, FormatArg(args)...), L'\0');
	_swprintf(buffer.data(), format, FormatArg(args)...);
	return buffer;
}
#pragma warning(pop)
#endif