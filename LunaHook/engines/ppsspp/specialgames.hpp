#include<queue>
#include"emujitarg.hpp"

bool ULJS00403_filter(void* data, size_t* len, HookParam* hp){
     std::string result = std::string((char*)data,*len);
    std::regex newlinePattern(R"((\\n)+)");
	result = std::regex_replace(result, newlinePattern, " ");
    std::regex pattern(R"((\\d$|^\@[a-z]+|#.*?#|\$))");
    result = std::regex_replace(result, pattern, "");
	return write_string_overwrite(data,len,result);
}


void ULJS00339(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
     auto a2= PPSSPP::emu_arg(stack)[0];

	auto vm = *(DWORD*)(a2+(0x28));
	vm=*(DWORD*)PPSSPP::emu_addr(stack,vm);
	vm=*(DWORD*)PPSSPP::emu_addr(stack,vm+8);
	uintptr_t address=PPSSPP::emu_addr(stack,vm);
	auto len1=*(DWORD*)(address+4);
	auto p=address+0x20;
	if(len1>4 && *(WORD*)(p+2)==0){
		auto p1=*(DWORD*)(address+8);
		vm=*(DWORD*)PPSSPP::emu_addr(stack,vm);
		vm=*(DWORD*)PPSSPP::emu_addr(stack,vm+0xC);
		p=PPSSPP::emu_addr(stack,vm);
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
    auto address= PPSSPP::emu_arg(stack)[1];
	bool haveNamve;
	auto s=Corda::readBinaryString(address,&haveNamve);
	*split=haveNamve;
	write_string_new(data,len,s);
}

void ULJM05054(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    if (hp->emu_addr != 0x886162c) {
		auto addr=PPSSPP::emu_arg(stack)[0]+0x3c;
		*data=addr;*len=strlen((char*)addr);
        return;
    }
	auto address= PPSSPP::emu_arg(stack)[1];
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

bool FULJM05603(LPVOID data, size_t* size, HookParam*)
{
    auto text = reinterpret_cast<LPSTR>(data);
    auto len = reinterpret_cast<size_t*>(size);

    StringCharReplacer(text, len, "%N", 2, ' ');
    StringFilter(text, len, "%K", 2);
    StringFilter(text, len, "%P", 2);

    return true;
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
     uintptr_t addr = PPSSPP::emu_addr(stack,0x08975110);
	 auto adr=addr+0x20;
	 auto len1=*(DWORD*)(addr+0x14)*2;
	 
	 *data=adr+len1-2;*len=2;
	 if(0x6e87==*(WORD*)*data)*len=0;
	 if(0x000a==*(WORD*)*data)*len=0;
}
namespace ppsspp{
    std::unordered_map<uintptr_t,emfuncinfo>emfunctionhooks= {
			//Shinigami to Shoujo
            {0x883bf34,{0,1,0,0,ULJS00403_filter,"ULJS00403"}},
			//Amagami
            {0x0886775c,{0,0,0,ULJS00339,0,"ULJS00339"}},// String.length()
			//Sekai de Ichiban Dame na Koi
            {0x8814adc,{0,0,0,0,NPJH50909_filter,"ULJM05878"}},// name + dialouge
            {0x8850b2c,{0,0,0,0,NPJH50909_filter,"ULJM05878"}},// onscreen toast
			//Dunamis15
            {0x0891D72C,{CODEC_UTF8,0,0,0,ULJM06119_filter,"ULJM06119"}},
			//Princess Evangile Portable
            {0x88506d0,{CODEC_UTF16,2,0,0,ULJM06036_filter,"ULJM06036"}},// [0x88506d0(2)...0x088507C0(?)] // name text text (line doubled)
			//Kin'iro no Corda 2f
            {0x89b59dc,{0,0,0,ULJM05428,0,"ULJM05428"}},
			//Kin'iro no Corda
            {0x886162c,{0,0,0,ULJM05054,0,"ULJM05054"}},// dialogue: 0x886162c (x1), 0x889d5fc-0x889d520(a2) fullLine
            {0x8899e90,{0,0,0,ULJM05054,0,"ULJM05054"}},// name 0x88da57c, 0x8899ca4 (x0, oneTime), 0x8899e90
			//Sol Trigger
            {0x8952cfc,{CODEC_UTF8,0,0,0,NPJH50619F,"NPJH50619"}},//dialog
            {0x884aad4,{CODEC_UTF8,0,0,0,NPJH50619F,"NPJH50619"}},//description
            {0x882e1b0,{CODEC_UTF8,0,0,0,NPJH50619F,"NPJH50619"}},//system
            {0x88bb108,{CODEC_UTF8,2,0,0,NPJH50619F,"NPJH50619"}},//battle tutorial
            {0x89526a0,{CODEC_UTF8,0,0,0,NPJH50619F,"NPJH50619"}},//battle info
            {0x88bcef8,{CODEC_UTF8,1,0,0,NPJH50619F,"NPJH50619"}},//battle talk
			//Fate/EXTRA CCC
            {0x8958490,{0,0,0,0,NPJH50505F,"NPJH50505"}},
			//Kamigami no Asobi InFinite
            {0x088630f8,{0,0,0,QNPJH50909,0,"NPJH50909"}}, // text, choice (debounce trailing 400ms), TODO: better hook
            {0x0887813c,{0,3,4,0,0,"NPJH50909"}}, // Question YN
			//Gekka Ryouran Romance
            {0x88eeba4,{0,0,0,0,ULJM05943F,"ULJM05943"}},// a0 - monologue text
            {0x8875e0c,{0,1,6,0,ULJM05943F,"ULJM05943"}},// a1 - dialogue text
			//My Merry May with be
            {0x886F014,{0,3,0,0,FULJM05603,"ULJM05603"}},
			//Corpse Party -The Anthology- Sachiko no Ren'ai Yuugi ♥ Hysteric Birthday 2U - Regular Edition
            {0x88517C8,{0,1,0,0,FULJM05603,"ULJM06114"}},
    };
    
}