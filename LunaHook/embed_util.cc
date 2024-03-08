#include"embed_util.h"
#include"MinHook.h"
#include"stringutils.h"
#include"main.h"
#include"detours.h"
#include"hijackfuns.h"
#include"winevent.hpp"
#include"defs.h"
#include"stringfilters.h"
DynamicShiftJISCodec *dynamiccodec=new DynamicShiftJISCodec(932);


void cast_back(const HookParam& hp,void*data ,size_t *len,const std::wstring& trans,bool normal){
  
	if((hp.type&EMBED_CODEC_UTF16)||(hp.type&CODEC_UTF16)){//renpy
		wcscpy((wchar_t*)data,trans.c_str());
		*len=trans.size()*2;
	}
	else{
    std::string astr;
    if(hp.type&EMBED_DYNA_SJIS&&!normal){
      astr=dynamiccodec->encodeSTD(trans,0);
    }
    else{
      astr=WideStringToString(trans,hp.codepage?hp.codepage:((hp.type&CODEC_UTF8)?CP_UTF8:embedsharedmem->codepage));
    }
		strcpy((char*)data,astr.c_str());
    *len=astr.size();
	}
}

struct FunctionInfo {
    const char *name; // for debugging purpose
    uintptr_t *oldFunction,
          newFunction;
    bool attached ;
    uintptr_t addr;
    explicit FunctionInfo(const uintptr_t _addr=0,const char *name = "", uintptr_t *oldFunction = nullptr, uintptr_t newFunction = 0,
        bool attached = false )
      : name(name), oldFunction(oldFunction), newFunction(newFunction)
      , attached(attached),addr(_addr)
    {}
  };
std::unordered_map<uintptr_t, FunctionInfo> funcs; // attached functions
std::vector<uintptr_t > replacedfuns; // attached functions
bool _1f()
{
#define ADD_FUN(_f) funcs[F_##_f] = FunctionInfo((uintptr_t)_f,#_f, (uintptr_t *)&Hijack::old##_f, (uintptr_t)Hijack::new##_f);
  ADD_FUN(CreateFontA)
  ADD_FUN(CreateFontW)
  ADD_FUN(CreateFontIndirectA)
  ADD_FUN(CreateFontIndirectW)
  ADD_FUN(GetGlyphOutlineA)
  ADD_FUN(GetGlyphOutlineW)
  ADD_FUN(GetTextExtentPoint32A)
  ADD_FUN(GetTextExtentPoint32W)
  ADD_FUN(GetTextExtentExPointA)
  ADD_FUN(GetTextExtentExPointW)
  //ADD_FUN(GetCharABCWidthsA)
  //ADD_FUN(GetCharABCWidthsW)
  ADD_FUN(TextOutA)
  ADD_FUN(TextOutW)
  ADD_FUN(ExtTextOutA)
  ADD_FUN(ExtTextOutW)
  ADD_FUN(DrawTextA)
  ADD_FUN(DrawTextW)
  ADD_FUN(DrawTextExA)
  ADD_FUN(DrawTextExW)
  ADD_FUN(CharNextA)
  //ADD_FUN(CharNextW)
  //ADD_FUN(CharNextExA)
  //ADD_FUN(CharNextExW)
  ADD_FUN(CharPrevA)
  //ADD_FUN(CharPrevW)
  ADD_FUN(MultiByteToWideChar)
  ADD_FUN(WideCharToMultiByte)
#undef ADD_FUN
return 0;
}
extern bool DetourAttachedUserAddr;
extern bool hostconnected;
bool _1=_1f();
void ReplaceFunction(PVOID* oldf,PVOID newf){
  
  RemoveHook((uintptr_t)*oldf);
  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread()); 
  DetourAttach((PVOID*)oldf, (PVOID)newf);
  DetourTransactionCommit();
}
void attachFunction(uintptr_t _hook_font_flag)
{
  for(auto & _func:funcs){
    if(_func.first&_hook_font_flag){
      if(_func.second.attached)continue;
      _func.second.attached = true; 
      *_func.second.oldFunction=_func.second.addr; 
      replacedfuns.push_back(_func.first);
      ReplaceFunction((PVOID*)_func.second.oldFunction,(PVOID)_func.second.newFunction); 
    }
  }
}
void detachall( )
{  
  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
   for(auto _flag:replacedfuns){
    auto info=funcs.at(_flag);
      DetourDetach((PVOID*)info.oldFunction, (PVOID)info.newFunction); 
      
   }
   DetourTransactionCommit();
}
void solvefont(HookParam hp){
  if(hp.hook_font){
     attachFunction(hp.hook_font);
  }
  if(hp.hook_font&F_MultiByteToWideChar)
    disable_mbwc=true;
  if(hp.hook_font&F_WideCharToMultiByte)
    disable_wcmb=true;
 
  if (auto current_patch_fun = patch_fun.exchange(nullptr)){
    current_patch_fun();
    DetourAttachedUserAddr=true;
  }
}
static std::wstring alwaysInsertSpacesSTD(const std::wstring& text)
  {
      std::wstring ret;
      for(auto c: text) {
          ret.push_back(c);
          if (c  >= 32) // ignore non-printable characters
              ret.push_back(L' '); // or insert \u3000 if needed
      }
      return ret;
  }
  bool  charEncodableSTD(const wchar_t& ch, UINT codepage)
{
     
    if (ch  <= 127) // ignore ascii characters
        return true;
    std::wstring s ;
    s.push_back(ch);
    return StringToWideString(WideStringToString(s, codepage), codepage).value() == s;
} 
  static std::wstring insertSpacesAfterUnencodableSTD(const std::wstring& text, HookParam hp)
  {
     
      std::wstring ret;
      for(const wchar_t & c: text) {
          ret.push_back(c);
          if (!charEncodableSTD(c, hp.codepage?hp.codepage:embedsharedmem->codepage))
              ret.push_back(L' ');
      }
      return ret;
  }
std::wstring adjustSpacesSTD(const std::wstring& text,HookParam hp) 
{
  
  switch (embedsharedmem->spaceadjustpolicy)
  {
  case 0: return text;
  case 1:return alwaysInsertSpacesSTD(text);
  case 2:return insertSpacesAfterUnencodableSTD(text, hp);
  default:return text;
  } 
} 
bool isPauseKeyPressed()
{
  return WinKey::isKeyControlPressed()
      || WinKey::isKeyShiftPressed() && !WinKey::isKeyReturnPressed();
}
inline UINT64 djb2_n2(const unsigned char * str, size_t len, UINT64 hash =  5381)
{
  int i=0;
    while (len--){
        hash = ((hash << 5) + hash) + (*str++); // hash * 33 + c
    }
    return hash;
}
std::unordered_map<UINT64,std::wstring>translatecache;
bool check_is_thread_selected(const ThreadParam& tp){
	for(int i=0;i<10;i++)
    if(embedsharedmem->use[i])
      if((embedsharedmem->addr[i]==tp.addr)&&(embedsharedmem->ctx1[i]==tp.ctx)&&(embedsharedmem->ctx2[i]==tp.ctx2))
        return true;
  return false;
}
bool check_embed_able(const ThreadParam& tp){
  return hostconnected&&check_is_thread_selected(tp)&&(isPauseKeyPressed()==false);
}
bool waitforevent(UINT32 timems,const ThreadParam& tp,const std::wstring &origin){
    char eventname[1000];
    sprintf(eventname,LUNA_EMBED_notify_event,GetCurrentProcessId(),djb2_n2((const unsigned char*)(origin.c_str()),origin.size()*2));
    auto event=win_event(eventname);
    while(timems){
      if(check_embed_able(tp)==false)return false;
      auto sleepstep=min(100,timems);
      if(event.wait(sleepstep))return true;
      timems-=sleepstep;
    }
    return false;
}

void TextHook::parsenewlineseperator(void*data ,size_t*len)
{
  if(!(hp.type&EMBED_ABLE && hp.newlineseperator))return;

  if (hp.type & CODEC_UTF16)
  {
    StringReplacer((wchar_t*)data,len,hp.newlineseperator,wcslen(hp.newlineseperator),L"\n",1);
  }
  else if(hp.type&CODEC_UTF32) return;
  else
  {
    //ansi/utf8，newlineseperator都是简单字符
    std::string newlineseperatorA;
    for(int i=0;i<wcslen(hp.newlineseperator);i++)newlineseperatorA+=(char)hp.newlineseperator[i];
    StringReplacer((char*)data,len,newlineseperatorA.c_str(),newlineseperatorA.size(),"\n",1);
  }
}
UINT64 texthash(void*data,size_t len)
{
  UINT64 sum=0;
  auto u8data=(UINT8*)data;
  for(int i=0;i< len;i++){
    sum+=u8data[i];
    sum=sum<<1;
  }
  return sum;
}
bool checktranslatedok(void*data ,size_t len)
{
  if(len>1000)return true;
  return(translatecache.find(texthash(data,len))!=translatecache.end());
}
bool TextHook::waitfornotify(TextOutput_T* buffer,void*data ,size_t*len,ThreadParam tp){
  std::wstring origin;
  if (auto t=commonparsestring(data,*len,&hp,embedsharedmem->codepage)) origin=t.value();
	else return false;

  std::wstring translate;
  auto hash=texthash(data,*len);
  if(translatecache.find(hash)!=translatecache.end()){
    translate=translatecache.at(hash);
  }
  else{
    if(waitforevent(embedsharedmem->waittime,tp,origin)==false)return false;
    translate=embedsharedmem->text; 
    if((translate.size()==0)||(translate==origin))return false;
    translatecache.insert(std::make_pair(hash,translate));
  }
	if(hp.newlineseperator)strReplace(translate,L"\n",hp.newlineseperator);
  translate=adjustSpacesSTD(translate,hp);
  if(embedsharedmem->keeprawtext)translate=origin+L" "+translate;
  solvefont(hp);
	cast_back(hp,data,len,translate,false);
	return true;
}