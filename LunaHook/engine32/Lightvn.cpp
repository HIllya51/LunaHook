#include"Lightvn.h"
 
//https://vndb.org/r?f=fwLight_evn-
 
void SpecialHookLightvnA(hook_stack*, HookParam*, uintptr_t* data, uintptr_t* split, size_t* len)
{
	//[Parser::ReadScriptBreak] curline:'"「次は[水縹]<みはなだ>駅、水縹駅――お出口は左側です」' 

	//[PARSETOKENS] line:.始発でここまで来ているのは俺くらいなものだろう。
 	//(scenario:T) (script:00.txt, lineNo:30) 
	//[PARSETOKENS] line:"電車には俺のほかに数人乗っている程度。\c
 	//(scenario:F) (script:00.txt, lineNo:29) 
	std::string s((char*)*data);
	//std::regex _1("\\[Parser::ReadScriptBreak\\] curline:'[\"\\.]([\\s\\S]*?)'([\\s\\S]*?)");//对于多行显示不全
	//std::regex _2("\\[PARSETOKENS\\] line:([\\s\\S]*?)\\(scenario:([\\s\\S]*?)");
	std::regex _2("\\[PARSETOKENS\\] line:[-\"\\.]+([\\s\\S]*?)\\(scenario:([\\s\\S]*?)");
	std::regex _3("\\[PARSETOKENS\\] line:([\\s\\S]*?)backlogName = '([\\s\\S]*?)'([\\s\\S]*?)");
	std::smatch match; std::string _;
	if (std::regex_match(s, match, _2)) { 
		_=std::string(match[1]);
		_ = std::regex_replace(_, std::regex("\\[(.*?)\\]<(.*?)>"), "$1");
		strReplace(_,"\\c","");
		strReplace(_,"\\w","");
		*split=1;
	} 
	else if (std::regex_match(s, match, _3)) { 
		_=std::string(match[2]);
		*split=2;
	}  
	auto _s=new char[_.size()+1];strcpy(_s,_.c_str());
	*data=(uintptr_t)_s;*len=_.size();
}

void SpecialHookLightvnW(hook_stack*, HookParam*, uintptr_t* data, uintptr_t* split, size_t* len)
{ 
	std::wstring s((wchar_t*)*data); 
	std::wregex _2(L"\\[PARSETOKENS\\] line:[-\"\\.]+([\\s\\S]*?)\\(scenario:([\\s\\S]*?)");
	std::wregex _3(L"\\[PARSETOKENS\\] line:([\\s\\S]*?)backlogName = '([\\s\\S]*?)'([\\s\\S]*?)");
	std::wsmatch match; std::wstring _;
	if (std::regex_match(s, match, _2)) { 
		_=std::wstring(match[1]);
		_ = std::regex_replace(_, std::wregex(L"\\[(.*?)\\]<(.*?)>"), L"$1");
		strReplace(_,L"\\c",L"");
		strReplace(_,L"\\w",L"");
		*split=1;
	} 
	else if (std::regex_match(s, match, _3)) { 
		_=std::wstring(match[2]);
		*split=2;
	}  
	auto _s=new wchar_t[_.size()+1];wcscpy(_s,_.c_str());
	*data=(uintptr_t)_s;*len=_.size()*2;
}
bool InsertLightvnHook()
{
	wcscpy_s(spDefault.boundaryModule, L"Engine.dll");
	/*// This hooking method also has decent results, but hooking OutputDebugString seems better
	const BYTE bytes[] = { 0x8d, 0x55, 0xfe, 0x52 };
	for (auto addr : Util::SearchMemory(bytes, sizeof(bytes), PAGE_EXECUTE_READ, (uintptr_t)GetModuleHandleW(L"Engine.dll")))
	{
		HookParam hp;
		hp.address = MemDbg::findEnclosingAlignedFunction(addr);
		hp.type = CODEC_UTF16 | USING_STRING;
		hp.offset=get_stack(1);
		NewHook(hp, "Light.vn");
	}*/
	VirtualProtect(IsDebuggerPresent, 2, PAGE_EXECUTE_READWRITE, DUMMY);
	*(uint16_t*)IsDebuggerPresent = 0xc340; // asm for inc eax ret
	HookParam hp;
	hp.address = (uintptr_t)OutputDebugStringA;
	hp.type = CODEC_UTF8 | USING_STRING;
	hp.offset=get_stack(1);
	hp.text_fun = SpecialHookLightvnA;
	auto succ=NewHook(hp, "OutputDebugStringA");
	hp.address = (uintptr_t)OutputDebugStringW;
	hp.type = CODEC_UTF16 | USING_STRING;
	hp.text_fun = SpecialHookLightvnW;
	succ|=NewHook(hp, "OutputDebugStringW");
	return succ;
}

bool Lightvn::attach_function() {
    
    return InsertLightvnHook();
}  