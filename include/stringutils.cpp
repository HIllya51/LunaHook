
LPCSTR reverse_search_begin(const char *s, int maxsize)
{
  if (*s)
    for (int i = 0; i < maxsize; i++, s--)
      if (!*s)
        return s + 1;
  return nullptr;
}

template <class CharT>
inline bool all_ascii_impl(const CharT *s, int maxsize)
{
  if (s)
    for (int i = 0; i < maxsize && *s; i++, s++)
      if ((unsigned)*s > 127)
        return false;
  return true;
}

template <class StringT>
inline void strReplace_impl(StringT &str, const StringT &oldStr, const StringT &newStr)
{
  size_t pos = 0;
  while ((pos = str.find(oldStr, pos)) != StringT::npos)
  {
    str.replace(pos, oldStr.length(), newStr);
    pos += newStr.length();
  }
}

template <class StringT>
inline std::vector<StringT> strSplit_impl(const StringT &s, const StringT &delim)
{
  StringT item;
  std::vector<StringT> tokens;

  StringT str = s;

  size_t pos = 0;
  while ((pos = str.find(delim)) != StringT::npos)
  {
    item = str.substr(0, pos);
    tokens.push_back(item);
    str.erase(0, pos + delim.length());
  }
  tokens.push_back(str);
  return tokens;
}

template <class StringT>
inline bool endWith_impl(const StringT &s, const StringT &s2)
{
  if (s2.size() && (s.size() >= s2.size()) && (s.substr(s.size() - s2.size(), s2.size()) == s2))
  {
    return true;
  }
  return false;
}

template <class StringT>
inline bool startWith_impl(const StringT &s, const StringT &s2)
{
  if (s2.size() && (s.size() >= s2.size()) && (s.substr(0, s2.size()) == s2))
  {
    return true;
  }
  return false;
}

bool all_ascii(const char *s, int maxsize) { return all_ascii_impl<char>(s, maxsize); }
bool all_ascii(const wchar_t *s, int maxsize) { return all_ascii_impl<wchar_t>(s, maxsize); }

void strReplace(std::string &str, const std::string &oldStr, const std::string &newStr) { strReplace_impl<std::string>(str, oldStr, newStr); }
void strReplace(std::wstring &str, const std::wstring &oldStr, const std::wstring &newStr) { strReplace_impl<std::wstring>(str, oldStr, newStr); }
std::vector<std::string> strSplit(const std::string &s, const std::string &delim) { return strSplit_impl<std::string>(s, delim); }
std::vector<std::wstring> strSplit(const std::wstring &s, const std::wstring &delim) { return strSplit_impl<std::wstring>(s, delim); }
bool startWith(const std::string &s, const std::string &s2) { return startWith_impl<std::string>(s, s2); }
bool startWith(const std::wstring &s, const std::wstring &s2) { return startWith_impl<std::wstring>(s, s2); }
bool endWith(const std::string &s, const std::string &s2) { return endWith_impl<std::string>(s, s2); }
bool endWith(const std::wstring &s, const std::wstring &s2) { return endWith_impl<std::wstring>(s, s2); }

typedef HRESULT(WINAPI *CONVERTINETMULTIBYTETOUNICODE)(
    LPDWORD lpdwMode,
    DWORD dwSrcEncoding,
    LPCSTR lpSrcStr,
    LPINT lpnMultiCharCount,
    LPWSTR lpDstStr,
    LPINT lpnWideCharCount);
typedef HRESULT(WINAPI *CONVERTINETUNICODETOMULTIBYTE)(
    LPDWORD lpdwMode,
    DWORD dwEncoding,
    LPCWSTR lpSrcStr,
    LPINT lpnWideCharCount,
    LPSTR lpDstStr,
    LPINT lpnMultiCharCount);

std::optional<std::wstring> StringToWideString(const std::string &text, UINT encoding)
{
  std::vector<wchar_t> buffer(text.size() + 1);
  if (disable_mbwc)
  {
    int _s = text.size();
    int _s2 = buffer.size();
    auto h = LoadLibrary(TEXT("mlang.dll"));
    if (h == 0)
      return {};
    auto ConvertINetMultiByteToUnicode = (CONVERTINETMULTIBYTETOUNICODE)GetProcAddress(h, "ConvertINetMultiByteToUnicode");
    if (ConvertINetMultiByteToUnicode == 0)
      return {};
    auto hr = ConvertINetMultiByteToUnicode(0, encoding, text.c_str(), &_s, buffer.data(), &_s2);
    if (SUCCEEDED(hr))
    {
      return std::wstring(buffer.data(), _s2);
    }
    else
      return {};
  }
  else
  {
    if (int length = MultiByteToWideChar(encoding, 0, text.c_str(), text.size() + 1, buffer.data(), buffer.size()))
      return std::wstring(buffer.data(), length - 1);
    return {};
  }
}

std::wstring StringToWideString(const std::string &text)
{
  return StringToWideString(text, CP_UTF8).value();
}

std::string WideStringToString(const std::wstring &text, UINT cp)
{
  std::vector<char> buffer((text.size() + 1) * 4);
  if (disable_wcmb)
  {
    int _s = text.size();
    int _s2 = buffer.size();
    auto h = LoadLibrary(TEXT("mlang.dll"));
    if (h == 0)
      return {};
    auto ConvertINetUnicodeToMultiByte = (CONVERTINETUNICODETOMULTIBYTE)GetProcAddress(h, "ConvertINetUnicodeToMultiByte");
    if (ConvertINetUnicodeToMultiByte == 0)
      return {};
    auto hr = ConvertINetUnicodeToMultiByte(0, cp, text.c_str(), &_s, buffer.data(), &_s2);
    if (SUCCEEDED(hr))
    {
      return std::string(buffer.data(), _s2);
    }
    else
      return {};
  }
  else
  {
    WideCharToMultiByte(cp, 0, text.c_str(), -1, buffer.data(), buffer.size(), nullptr, nullptr);
    return buffer.data();
  }
}
inline unsigned int convertUTF32ToUTF16(unsigned int cUTF32, unsigned int &h, unsigned int &l)
{
  if (cUTF32 < 0x10000)
  {
    h = 0;
    l = cUTF32;
    return cUTF32;
  }
  unsigned int t = cUTF32 - 0x10000;
  h = (((t << 12) >> 22) + 0xD800);
  l = (((t << 22) >> 22) + 0xDC00);
  unsigned int ret = ((h << 16) | (l & 0x0000FFFF));
  return ret;
}

std::basic_string<uint32_t> utf16_to_utf32(const wchar_t *u16str, size_t size)
{
  std::basic_string<uint32_t> utf32String;
  for (size_t i = 0; i < size; i++)
  {
    auto u16c = u16str[i];
    if (u16c - 0xd800u < 2048u)
      if ((u16c & 0xfffffc00 == 0xd800) && (i < size - 1) && (u16str[i + 1] & 0xfffffc00 == 0xdc00))
      {
        utf32String += (u16c << 10) + u16str[i + 1] - 0x35fdc00;
        i += 1;
      }
      else
      {
        // error invalid u16 char
      }
    else
      utf32String += u16str[i];
  }
  return utf32String;
}

std::wstring utf32_to_utf16(uint32_t *u32str, size_t size)
{
  std::wstring u16str;
  for (auto i = 0; i < size; i++)
  {
    unsigned h, l;
    convertUTF32ToUTF16(u32str[i], h, l);
    if (h)
      u16str.push_back((wchar_t)h);
    u16str.push_back((wchar_t)l);
  }
  return u16str;
}

size_t u32strlen(uint32_t *data)
{
  size_t s = 0;
  while (data[s])
    s++;
  return s;
}

std::string wcasta(const std::wstring &x)
{
  std::string xx;
  for (auto c : x)
    xx += c;
  return xx;
}

std::wstring acastw(const std::string &x)
{
  std::wstring xx;
  for (auto c : x)
    xx += c;
  return xx;
}
std::optional<std::wstring> commonparsestring(void *data, size_t length, void *php, DWORD df)
{
  auto hp = (HookParam *)php;
  if (hp->type & CODEC_UTF16)
    return std::wstring((wchar_t *)data, length / sizeof(wchar_t));
  else if (hp->type & CODEC_UTF32)
    return (std::move(utf32_to_utf16((uint32_t *)data, length / sizeof(uint32_t))));
  else if (auto converted = StringToWideString(std::string((char *)data, length), (hp->type & CODEC_UTF8) ? CP_UTF8 : (hp->codepage ? hp->codepage : df)))
    return (converted.value());
  else
    return {};
}