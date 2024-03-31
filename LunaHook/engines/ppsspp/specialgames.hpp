#include<queue>
namespace ppsspp{
    inline DWORD x86_baseaddr;
}
namespace
{
	
template<int index,int offset=0>
void simple932getter(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
     hp->type=USING_STRING|NO_CONTEXT;
	 hp->codepage=932;
	 auto address= emu_arg(stack)[index]+offset;
	*data=address;*len=strlen((char*)address);
}
template<int index>
void simpleutf8getter(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=emu_arg(stack)[index];
    hp->type=USING_STRING|CODEC_UTF8|NO_CONTEXT;
    *data=address;*len=strlen((char*)address);
}
template<int index,DWORD _type=0>
void simpleutf16getter(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=emu_arg(stack)[index];
    hp->type=USING_STRING|CODEC_UTF16|NO_CONTEXT|_type;
    *data=address;*len=wcslen((wchar_t*)address)*2;
}
class emu_addr{
    hook_stack* stack;
    DWORD addr;
public:
    emu_addr(hook_stack* stack_,DWORD addr_):stack(stack_),addr(addr_){};
    operator uintptr_t(){
        #ifndef _WIN64
        auto base=ppsspp::x86_baseaddr;
        #else
        auto base=stack->rbx;
        #endif
        return base+addr;
    }
    operator DWORD*(){
        return (DWORD*)(uintptr_t)*this;
    }
};
class emu_arg{
    hook_stack* stack;
public:

    emu_arg(hook_stack* stack_):stack(stack_){};
    uintptr_t operator [](int idx){
        #ifndef _WIN64
        auto args=stack->ebp;
        #else
        auto args=stack->r14;
        #endif
		auto offR = -0x80;
		auto offset = offR + 0x10 + idx * 4;
        return (uintptr_t)emu_addr(stack,*(uint32_t*)(args+offset));
    }
};
} 

bool ULJS00403_filter(void* data, size_t* len, HookParam* hp){
     std::string result = std::string((char*)data,*len);
    std::regex newlinePattern(R"((\\n)+)");
	result = std::regex_replace(result, newlinePattern, " ");
    std::regex pattern(R"((\\d$|^\@[a-z]+|#.*?#|\$))");
    result = std::regex_replace(result, pattern, "");
	return write_string_overwrite(data,len,result);
}


void ULJS00339(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
     hp->type=USING_STRING|NO_CONTEXT;
	 hp->codepage=932;
	 auto a2= emu_arg(stack)[0];

	auto vm = *(DWORD*)(a2+(0x28));
	vm=*(DWORD*)emu_addr(stack,vm);
	vm=*(DWORD*)emu_addr(stack,vm+8);
	uintptr_t address=emu_addr(stack,vm);
	auto len1=*(DWORD*)(address+4);
	auto p=address+0x20;
	if(len1>4 && *(WORD*)(p+2)==0){
		auto p1=*(DWORD*)(address+8);
		vm=*(DWORD*)emu_addr(stack,vm);
		vm=*(DWORD*)emu_addr(stack,vm+0xC);
		p=emu_addr(stack,vm);
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

void ULJM05428(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
     hp->type=USING_STRING|NO_CONTEXT;
	 hp->codepage=932;
	auto address= emu_arg(stack)[1];
	bool haveNamve;
	auto s=Corda::readBinaryString(address,&haveNamve);
	*split=haveNamve;
	write_string_new(data,len,s);
}

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


bool ULJM05943F(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    std::regex pattern1("#n+");
    std::string replacement1 = " ";
    std::string result1 = std::regex_replace(s, pattern1, replacement1);
    std::regex pattern2("#[A-Za-z]+\\[(\\d*\\.)?\\d+\\]+");
    std::string replacement2 = "";
    std::string result2 = std::regex_replace(result1, pattern2, replacement2);
	return write_string_overwrite(data,len,result2);
}

bool NPJH50619F(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    std::regex pattern1("[\\r\\n]+");
    std::string replacement1 = "";
    std::string result1 = std::regex_replace(s, pattern1, replacement1);
    std::regex pattern2("^(.*?)\\)+");
    std::string replacement2 = "";
    std::string result2 = std::regex_replace(result1, pattern2, replacement2);
    std::regex pattern3("#ECL+");
    std::string replacement3 = "";
    std::string result3 = std::regex_replace(result2, pattern3, replacement3);
    std::regex pattern4("(#.+?\\))+");
    std::string replacement4 = "";
    std::string result4 = std::regex_replace(result3, pattern4, replacement4);
	return write_string_overwrite(data,len,result4);
}


bool NPJH50505F(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    
    std::regex pattern2("#RUBS(#[A-Z0-9]+)*[^#]+");
    std::string replacement2 = "";
    std::string result2 = std::regex_replace(s, pattern2, replacement2);

    std::regex pattern3("#FAMILY");
    std::string replacement3 = "$FAMILY";
    std::string result3 = std::regex_replace(result2, pattern3, replacement3);

    std::regex pattern4("#GIVE");
    std::string replacement4 = "$GIVE";
    std::string result4 = std::regex_replace(result3, pattern4, replacement4);

    std::regex pattern5("(#[A-Z0-9\\-]+)+");
    std::string replacement5 = "";
    std::string result5 = std::regex_replace(result4, pattern5, replacement5);

    std::regex pattern6("\\n+");
    std::string replacement6 = " ";
    std::string result6 = std::regex_replace(result5, pattern6, replacement6);

	return write_string_overwrite(data,len,result6);
}

void QNPJH50909(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
     hp->type=USING_STRING|NO_CONTEXT;
	 hp->codepage=932;
	 uintptr_t addr = emu_addr(stack,0x08975110);
	 auto adr=addr+0x20;
	 auto len1=*(DWORD*)(addr+0x14)*2;
	 
	 *data=adr+len1-2;*len=2;
	 if(0x6e87==*(WORD*)*data)*len=0;
	 if(0x000a==*(WORD*)*data)*len=0;
}
namespace ppsspp{
    std::unordered_map<uintptr_t,emfuncinfo>emfunctionhooks= {
            {0x883bf34,{"Shinigami to Shoujo",simple932getter<1>,ULJS00403_filter,L"ULJS00403"}},
            {0x0886775c,{"Amagami",ULJS00339,0,L"ULJS00339"}},// String.length()
            {0x8814adc,{"Sekai de Ichiban Dame na Koi",simple932getter<0>,NPJH50909_filter,L"ULJM05878"}},// name + dialouge
            {0x8850b2c,{"Sekai de Ichiban Dame na Koi",simple932getter<0>,NPJH50909_filter,L"ULJM05878"}},// onscreen toast
            {0x0891D72C,{"Dunamis15",simpleutf8getter<0>,ULJM06119_filter,L"ULJM06119"}},
            {0x88506d0,{"Princess Evangile Portable",simpleutf16getter<2>,ULJM06036_filter,L"ULJM06036"}},// [0x88506d0(2)...0x088507C0(?)] // name text text (line doubled)
            {0x89b59dc,{"Kin'iro no Corda 2f",ULJM05428,0,L"ULJM05428"}},
            {0x886162c,{"Kin'iro no Corda",ULJM05054,0,L"ULJM05054"}},// dialogue: 0x886162c (x1), 0x889d5fc-0x889d520(a2) fullLine
            {0x8899e90,{"Kin'iro no Corda",ULJM05054,0,L"ULJM05054"}},// name 0x88da57c, 0x8899ca4 (x0, oneTime), 0x8899e90
            {0x8952cfc,{"Sol Trigger",simpleutf8getter<0>,NPJH50619F,L"NPJH50619"}},//dialog
            {0x884aad4,{"Sol Trigger",simpleutf8getter<0>,NPJH50619F,L"NPJH50619"}},//description
            {0x882e1b0,{"Sol Trigger",simpleutf8getter<0>,NPJH50619F,L"NPJH50619"}},//system
            {0x88bb108,{"Sol Trigger",simpleutf8getter<2>,NPJH50619F,L"NPJH50619"}},//battle tutorial
            {0x89526a0,{"Sol Trigger",simpleutf8getter<0>,NPJH50619F,L"NPJH50619"}},//battle info
            {0x88bcef8,{"Sol Trigger",simpleutf8getter<1>,NPJH50619F,L"NPJH50619"}},//battle talk
            {0x8958490,{"Fate/EXTRA CCC",simple932getter<0>,NPJH50505F,L"NPJH50505"}},
            {0x088630f8,{"Kamigami no Asobi InFinite",QNPJH50909,0,L"NPJH50909"}}, // text, choice (debounce trailing 400ms), TODO: better hook
            {0x0887813c,{"Kamigami no Asobi InFinite",simple932getter<3,4>,0,L"NPJH50909"}}, // Question YN
			
            {0x88eeba4,{"Gekka Ryouran Romance",simple932getter<0,0>,ULJM05943F,L"ULJM05943"}},// a0 - monologue text
            {0x8875e0c,{"Gekka Ryouran Romance",simple932getter<1,6>,ULJM05943F,L"ULJM05943"}},// a1 - dialogue text
    };
    
}