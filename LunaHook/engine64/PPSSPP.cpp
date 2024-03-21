#include"PPSSPP.h"
#include<queue>
/** Artikash 6/7/2019
*   PPSSPP JIT code has pointers, but they are all added to an offset before being used.
	Find that offset so that hook searching works properly.
	To find the offset, find a page of mapped memory with size 0x1f00000, read and write permissions, take its address and subtract 0x8000000.
	The above is useful for emulating PSP hardware, so unlikely to change between versions.
*/
bool PPSSPPinithooksearch(){
	bool found = false;
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);
	for (BYTE* probe = NULL; probe < systemInfo.lpMaximumApplicationAddress;)
	{
		MEMORY_BASIC_INFORMATION info;
		if (!VirtualQuery(probe, &info, sizeof(info)))
		{
			probe += systemInfo.dwPageSize;
		}
		else
		{
			if (info.RegionSize == 0x1f00000 && info.Protect == PAGE_READWRITE && info.Type == MEM_MAPPED)
			{
				found = true;
				ConsoleOutput("PPSSPP memory found: searching for hooks should yield working hook codes");
				// PPSSPP 1.8.0 compiles jal to sub dword ptr [r14+0x360],??
				memcpy(spDefault.pattern, Array<BYTE>{ 0x41, 0x83, 0xae, 0x60, 0x03, 0x00, 0x00 }, spDefault.length = 7);
				spDefault.offset = 0;
				spDefault.minAddress = 0;
				spDefault.maxAddress = -1ULL;
				spDefault.padding = (uintptr_t)probe - 0x8000000;
				spDefault.hookPostProcessor = [](HookParam& hp)
				{
					hp.type |= NO_CONTEXT | USING_SPLIT | SPLIT_INDIRECT;
					hp.split = get_reg(regs::r14);
					hp.split_index = -8; // this is where PPSSPP 1.8.0 stores its return address stack
				};
			}
			probe += info.RegionSize;
		}
	}
	return found;
}
namespace{ 
uintptr_t getDoJitAddress() {
    auto DoJitSig1 = "C7 83 ?? 0? 00 00 11 00 00 00 F6 83 ?? 0? 00 00 01 C7 83 ?? 0? 00 00 E4 00 00 00";
    auto first=find_pattern(DoJitSig1,processStartAddress,processStopAddress); 
    if (first) {
        auto beginSubSig1 = "55 41 ?? 41 ?? 41";
        auto lookbackSize = 0x400;
        auto address=first-lookbackSize;
        auto subs=find_pattern(beginSubSig1,address,address+lookbackSize);
        if(subs){
            return subs;
        }
    }
    return 0;
}

class emu_arg{
    hook_stack* stack;
public:
    emu_arg(hook_stack* stack_):stack(stack_){};
    uintptr_t operator [](int idx){
        auto base=stack->rbx;
        auto args=stack->r14;
		auto offR = -0x80;
		auto offset = offR + 0x10 + idx * 4;
        return base+*(uint32_t*)(args+offset);
    }
};
struct emfuncinfo{
    const char* hookname;
    void* hookfunc;
	void* filterfun;
    const wchar_t* _id;
};
std::unordered_map<uintptr_t,emfuncinfo>emfunctionhooks;
std::unordered_set<uintptr_t>breakpoints;
}
bool hookPPSSPPDoJit(){
    ConsoleOutput("[Compatibility] PPSSPP 1.12.3-867 -> v1.16.1-35");
	auto DoJitPtr=getDoJitAddress();
   if(DoJitPtr==0)return false;
   HookParam hp;
   hp.address=DoJitPtr;//Jit::DoJit
   ConsoleOutput("DoJitPtr %p",DoJitPtr);
   
   hp.text_fun=[](hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
        auto em_address=stack->ARG2;

		if(emfunctionhooks.find(em_address)==emfunctionhooks.end())return;
		
		static emfuncinfo op;
		op=emfunctionhooks.at(em_address);
		HookParam hpinternal;
		hpinternal.user_value=em_address;
		hpinternal.address=stack->retaddr;
		hpinternal.text_fun=[](hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
			hp->text_fun=nullptr;hp->type=HOOK_EMPTY;
			
			auto ret=stack->rax;
			if(breakpoints.find(ret)!=breakpoints.end())return;
			breakpoints.insert(ret);
			DWORD _;
			VirtualProtect((LPVOID)ret,0x10,PAGE_EXECUTE_READWRITE,&_);
			HookParam hpinternal;
			hpinternal.address=ret;
			hpinternal.user_value=hp->user_value;
			hpinternal.type=CODEC_UTF16|USING_STRING|NO_CONTEXT;
			hpinternal.text_fun=(decltype(hpinternal.text_fun))op.hookfunc;
			hpinternal.filter_fun=(decltype(hpinternal.filter_fun))op.filterfun;
			NewHook(hpinternal,op.hookname);
		};
		NewHook(hpinternal,"DoJitPtrRet");
   };
   
  return NewHook(hp,"PPSSPPDoJit");
}

bool PPSSPP::attach_function()
{
	return PPSSPPinithooksearch()| hookPPSSPPDoJit();
}

// @name         [ULJS00403] Shinigami to Shoujo
// @version      0.1
// @author       [DC]
// @description  PPSSPP x64
// * TAKUYO
// * TAKUYO
// *
void ULJS00403(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
     hp->type=USING_STRING|NO_CONTEXT;
	 hp->codepage=932;
	 auto address= emu_arg(stack)[1];
	 *data=address;
	*len=strlen((char*)address);
}
bool ULJS00403_filter(void* data, size_t* len, HookParam* hp){
     std::string result = std::string((char*)data,*len);
    std::regex newlinePattern(R"((\\n)+)");
	result = std::regex_replace(result, newlinePattern, " ");
    std::regex pattern(R"((\\d$|^\@[a-z]+|#.*?#|\$))");
    result = std::regex_replace(result, pattern, "");
	return write_string_overwrite(data,len,result);
}

// @name         [ULJS00339] Amagami
// @version      v1.02
// @author       [DC]
// @description  
// * Kadokawa
// * K2X_Script
void ULJS00339(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
     hp->type=USING_STRING|NO_CONTEXT;
	 hp->codepage=932;
	 auto base=stack->rbx;
	 auto a2= emu_arg(stack)[0];

	auto vm = *(DWORD*)(a2+(0x28));
	vm=*(DWORD*)(vm+base);
	vm=*(DWORD*)(vm+base+8);
	auto address=vm+base;
	auto len1=*(DWORD*)(address+4);
	auto p=address+0x20;
	if(len1>4 && *(WORD*)(p+2)==0){
		auto p1=*(DWORD*)(address+8);
		vm=*(DWORD*)(vm+base);
		vm=*(DWORD*)(vm+base+0xC);
		p=vm+base;
	}
	static int fm=0;
	static std::string pre;
	auto b=fm;
	auto s=[](uintptr_t address){
		auto frist = *(WORD*)address;
		auto lo = frist & 0xFF; // uppercase: 41->5A
		auto hi = frist >> 8;
		if (hi == 0 && (lo > 0x5a || lo < 0x41)  /* T,W,? */) {
			return std::string();
		} 
    	std::string s ;int i = 0;WORD c;
		char buf[3]={0};
		while ((c = *(WORD*)(address+i)) != 0) {
			// reverse endian: ShiftJIS BE => LE
			buf[0] = c >> 8;
			buf[1] = c & 0xFF;

			if (c == 0x815e /* ／ */) {
				s += ' '; // single line
			}
			else if (buf[0] == 0) {
				//// UTF16 LE turned BE: 5700=>0057, 3100, 3500
				//// 4e00 6d00=>PLAYER
				// do nothing
				if (buf[1] == 0x4e) {
					s += "PLAYER";
					fm++;
				}
			}
			else {
				s+=buf;
			}
			i += 2;
		}
		return s;
	}(p);
	if(b>0){
		fm--;
		return;
	}
	if(s==pre)return ;
	pre=s;
	write_string_new(data,len,s);
}

// @name         [NPJH50909] Sekai de Ichiban Dame na Koi
// @version      0.1
// @author       [DC]
// @description  PPSSPP x64
// * 
void NPJH50909(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
     hp->type=USING_STRING|NO_CONTEXT;
	 hp->codepage=932;
	 auto address= emu_arg(stack)[0];
	 *data=address;
	*len=strlen((char*)address);
}
bool NPJH50909_filter(void* data, size_t* len, HookParam* hp){
     std::string result = std::string((char*)data,*len);

    // Remove single line markers
    result = std::regex_replace(result, std::regex("(\\%N)+"), " ");

    // Remove scale marker
    result = std::regex_replace(result, std::regex("\\%\\@\\%\\d+"), "");

    // Reformat name
    std::smatch match;
    if (std::regex_search(result, match, std::regex("(^[^「]+)「"))) {
        std::string name = match[1].str();
        result = std::regex_replace(result, std::regex("^[^「]+"), "");
        result = name + "\n" + result;
    }
	return write_string_overwrite(data,len,result);
}

// @name         [ULJM06119] Dunamis15
// @version      0.1
// @author       [DC]
// @description  PPSSPP x64
// * Division ZERO & MAGES. GAME
// * Kaleido ADV Workshop
void ULJM06119(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
     hp->type=USING_STRING|NO_CONTEXT|CODEC_UTF8;
	 auto address= emu_arg(stack)[0];
	*data=address;
	*len=strlen((char*)address);
}
bool ULJM06119_filter(void* data, size_t* len, HookParam* hp){
     std::string s = std::string((char*)data,*len);

    std::regex pattern(R"(/\[[^\]]+./g)");
    s = std::regex_replace(s, pattern, "");

    std::regex tagPattern(R"(/\\k|\\x|%C|%B)");
    s = std::regex_replace(s, tagPattern, "");

    std::regex colorPattern(R"(/\%\d+\#[0-9a-fA-F]*\;)");
    s = std::regex_replace(s, colorPattern, "");

    std::regex newlinePattern(R"(/\n+)");
    s = std::regex_replace(s, newlinePattern, " ");
	return write_string_overwrite(data,len,s);
}
// @name         [ULJM06036] Princess Evangile Portable
// @version      0.1
// @author       [DC]
// @description  PPSSPP x64
void ULJM06036(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
     hp->type=USING_STRING|NO_CONTEXT|CODEC_UTF16;
	 auto address= emu_arg(stack)[2];
	*data=address;
	*len=wcslen((wchar_t*)address)*2;
}
bool ULJM06036_filter(void* data, size_t* len, HookParam* hp){
     std::wstring result = std::wstring((wchar_t*)data,*len/2);
    std::wregex pattern(LR"(<R([^\/]+).([^>]+).>)");
    result = std::regex_replace(result, pattern, L"$2");
    std::wregex tagPattern(LR"(<[A-Z]+>)");
    result = std::regex_replace(result, tagPattern, L"");
	return write_string_overwrite(data,len,result);
}

namespace Corda{
	std::string readBinaryString(uintptr_t address,bool* haveName){
		* haveName=false;
		if ((*(WORD*)address & 0xF0FF) == 0x801b) {
			*haveName = true;
			address = address+2; // (1)
    	}
		std::string s;int i=0;uint8_t c;
		while ((c = *(uint8_t*)(address+i)) != 0) {
			if (c == 0x1b) {
				if (*haveName)
					return s; // (1) skip junk after name

				c = *(uint8_t*)(address+(i + 1));
				if (c == 0x7f)
					i += 5;
				else
					i += 2;
			}
			else if (c == 0x0a) {
				s += '\n';
				i += 1;
			}
			else if (c == 0x20) {
				s += ' ';
				i += 1;
			}
			else {
				auto len=1+(IsDBCSLeadByteEx(932,*(BYTE*)(address+i)));
				s += std::string((char*)(address+i),len);
				i += len;//encoder.encode(c).byteLength;
			}
		}
		return s;
	}
}
// @name         [ULJM05428] Kin'iro no Corda 2f
// @version      0.1
// @author       [DC]
// @description  PPSSPP x64
// * 
// * Koei

void ULJM05428(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
     hp->type=USING_STRING|NO_CONTEXT;
	 hp->codepage=932;
	auto address= emu_arg(stack)[1];
	bool haveNamve;
	auto s=Corda::readBinaryString(address,&haveNamve);
	*split=haveNamve;
	write_string_new(data,len,s);
}
// @name         [ULJM05054] Kin'iro no Corda
// @version      0.1
// @author       [DC]
// @description  PPSSPP x64
// * 
// * Koei

void ULJM05054(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
     hp->type=USING_STRING|NO_CONTEXT;
	 hp->codepage=932;
	 if (hp->user_value != 0x886162c) {
		auto addr=emu_arg(stack)[0]+0x3c;
		*data=addr;*len=strlen((char*)addr);
        return;
    }
	auto address= emu_arg(stack)[1];
	bool haveNamve;
	auto s=Corda::readBinaryString(address,&haveNamve);
	*split=haveNamve;
	write_string_new(data,len,s);
}
namespace{
auto _=[](){
    emfunctionhooks={
            {0x883bf34,{"Shinigami to Shoujo",ULJS00403,ULJS00403_filter,L"PCSG01282"}},
            {0x0886775c,{"Amagami",ULJS00339,0,L"ULJS00339"}},
            {0x8814adc,{"Sekai de Ichiban Dame na Koi",NPJH50909,NPJH50909_filter,L"NPJH50909"}},
            {0x8850b2c,{"Sekai de Ichiban Dame na Koi",NPJH50909,NPJH50909_filter,L"NPJH50909"}},
            {0x0891D72C,{"Dunamis15",ULJM06119,ULJM06119_filter,L"ULJM06119"}},
            {0x88506d0,{"Princess Evangile Portable",ULJM06036,ULJM06036_filter,L"ULJM06036"}},
            {0x89b59dc,{"Kin'iro no Corda 2f",ULJM05428,0,L"ULJM05428"}},
            {0x886162c,{"Kin'iro no Corda",ULJM05054,0,L"ULJM05054"}},
            {0x8899e90,{"Kin'iro no Corda",ULJM05054,0,L"ULJM05054"}},
    };
    return 1;
}();
}