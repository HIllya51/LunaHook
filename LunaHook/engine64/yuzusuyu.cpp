#include"yuzusuyu.h"
#include"mages/mages.h"
#include"hookfinder.h"
#include"emujitarg.hpp"
namespace{
    auto isFastMem = true;

    auto isVirtual = true;//Process.arch === 'x64' && Process.platform === 'windows';
    auto idxDescriptor = isVirtual == true ? 2 : 1;
    auto idxEntrypoint = idxDescriptor + 1;

uintptr_t getDoJitAddress() {
    auto RegisterBlockSig1 = "E8 ?? ?? ?? ?? 4? 8B ?? 4? 8B ?? 4? 8B ?? E8 ?? ?? ?? ?? 4? 89?? 4? 8B???? ???????? 4? 89?? ?? 4? 8B?? 4? 89";
    auto RegisterBlock=find_pattern(RegisterBlockSig1,processStartAddress,processStopAddress); 
    if (RegisterBlock) {
        auto beginSubSig1 = "CC 40 5? 5? 5?";
        auto lookbackSize = 0x400;
        auto address=RegisterBlock-lookbackSize;
        auto subs=find_pattern(beginSubSig1,address,address+lookbackSize);
        if(subs){
            return subs+1;
        }
    }

    auto PatchSig1 = "4????? 4????? 4????? FF?? ?? 4????? ?? 4????? 75 ?? 4????? ?? 4????? ?? 4?";
    auto Patch = find_pattern(PatchSig1,processStartAddress,processStopAddress);
    if (Patch) {
        auto beginSubSig1 = "4883EC ?? 48";
        auto lookbackSize = 0x80;
        auto address = Patch-lookbackSize;
        auto subs = find_pattern(beginSubSig1,address,address+lookbackSize);
        if (subs) {
            idxDescriptor = 1;
            idxEntrypoint = 2;
            return subs;
        }
    }
    return 0;
    /*
    这块不知道怎么实现。
    // DebugSymbol: RegisterBlock
    // ?RegisterBlock@EmitX64@X64@Backend@Dynarmic@@IEAA?AUBlockDescriptor@1234@AEBVLocationDescriptor@IR@4@PEBX_K@Z <- new
    // ?RegisterBlock@EmitX64@X64@Backend@Dynarmic@@IEAA?AUBlockDescriptor@1234@AEBVLocationDescriptor@IR@4@PEBX1_K@Z
    const symbols = DebugSymbol.findFunctionsMatching('Dynarmic::Backend::X64::EmitX64::RegisterBlock');
    if (symbols.length !== 0) {
        return symbols[0];
    }

    // DebugSymbol: Patch
    // ?Patch@EmitX64@X64@Backend@Dynarmic@@IEAAXAEBVLocationDescriptor@IR@4@PEBX@Z
    const patchs = DebugSymbol.findFunctionsMatching('Dynarmic::Backend::X64::EmitX64::Patch');
    if (patchs.length !== 0) {
        idxDescriptor = 1;
        idxEntrypoint = 2;
        return patchs[0];
    }
    */
}


struct emfuncinfo{
    uint64_t type;
    int argidx;int padding;
    void* hookfunc;
	void* filterfun;
    const char* _id;
    const char* _version;
};
std::unordered_map<uintptr_t,emfuncinfo>emfunctionhooks;

struct GameInfo {
    std::string name{""};
    uint64_t id{0};
    std::string version{""};
} game_info;
bool checkiscurrentgame(const emfuncinfo& em){
	auto wininfos=get_proc_windows();
	for(auto&& info:wininfos){
        if((game_info.version.size())&&game_info.name.size()&&(game_info.id!=0)){
            //判断是有效的info
            auto checkversion=(em._version==0)||(std::string(em._version)==(game_info.version));
            auto checkid=(std::stoll(em._id,0,16)==game_info.id);
            if(checkid&&checkversion)return true;
        }
		else if((em._version==0)||(info.title.find(acastw(em._version))!=info.title.npos))return true;
	}
	return false;
}
}
bool Hook_Network_RoomMember_SendGameInfo(){
    // void RoomMember::SendGameInfo(const GameInfo& game_info) {
    //     room_member_impl->current_game_info = game_info;
    //     if (!IsConnected())
    //         return;

    //     Packet packet;
    //     packet.Write(static_cast<u8>(IdSetGameInfo));
    //     packet.Write(game_info.name);
    //     packet.Write(game_info.id);
    //     packet.Write(game_info.version);
    //     room_member_impl->Send(std::move(packet));
    // }
    BYTE pattern[]={
        0x49,0x8B,XX,
        0x0F,0xB6,0x81,0x28,0x01,0x00,0x00,
        0x90,
        0x3C,0x02,
        0x74,0x1C,
        0x0F,0xB6,0x81,0x28,0x01,0x00,0x00,
        0x90,
        0x3C,0x03,
        0x74,0x10,
        0x0F,0xB6,0x81,0x28,0x01,0x00,0x00,
        0x90,
        0x3C,0x04,
        0x0F,0x85,XX4
    };
    for(auto addr:Util::SearchMemory(pattern,sizeof(pattern),PAGE_EXECUTE,processStartAddress,processStopAddress))
    {
        addr=MemDbg::findEnclosingAlignedFunction_strict(addr,0x100);
        //有两个，但另一个离起始很远
        if(addr==0)continue;
        HookParam hp;
        hp.address=addr;
        hp.text_fun=[](hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
            // void __fastcall Network::RoomMember::SendGameInfo(
            //     Network::RoomMember *this,
            //     const AnnounceMultiplayerRoom::GameInfo *game_info)
            game_info=*(GameInfo*)stack->rdx;
            ConsoleOutput("%s %llx %s",game_info.name.c_str(),game_info.id,game_info.version.c_str());
        };
        return NewHook(hp,"yuzuGameInfo");
    }
    return false;
}
bool yuzusuyu::attach_function()
{
   Hook_Network_RoomMember_SendGameInfo();
   ConsoleOutput("[Compatibility] Yuzu 1616+");
   auto DoJitPtr=getDoJitAddress();
   if(DoJitPtr==0)return false;
   spDefault.jittype=JITTYPE::YUZU;
   spDefault.minAddress = 0;
   spDefault.maxAddress = -1;
   ConsoleOutput("DoJitPtr %p",DoJitPtr);
   HookParam hp;
   hp.address=DoJitPtr;
   hp.text_fun=[](hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
        auto descriptor = *argidx(stack,idxDescriptor); // r8
        auto entrypoint = *argidx(stack,idxEntrypoint); // r9
        auto em_address = *(uintptr_t*)descriptor;
        if(!entrypoint)return;
        jitaddraddr(em_address,entrypoint,JITTYPE::YUZU);
        [&](){
            if(emfunctionhooks.find(em_address)==emfunctionhooks.end())return;
            auto op=emfunctionhooks.at(em_address);
            if(!(checkiscurrentgame(op)))return;
            
            HookParam hpinternal;
            hpinternal.address=entrypoint;
            hpinternal.emu_addr=em_address;//用于生成hcode
            hpinternal.type=USING_STRING|NO_CONTEXT|BREAK_POINT|op.type;
            hpinternal.text_fun=(decltype(hpinternal.text_fun))op.hookfunc;
            hpinternal.filter_fun=(decltype(hpinternal.filter_fun))op.filterfun;
            hpinternal.argidx=op.argidx;
            hpinternal.padding=op.padding;
            hpinternal.jittype=JITTYPE::YUZU;
            NewHook(hpinternal,op._id);
        }();
        delayinsertNewHook(em_address);
   };
  return NewHook(hp,"YuzuDoJit");
} 


namespace{
int readu8(BYTE* addr){
    int numBytes = 0;
    auto firstByte=*addr;
    if (firstByte <= 0x7F) {
        numBytes = 1;
    } else if ((firstByte & 0xE0) == 0xC0) {
        numBytes = 2;
    } else if ((firstByte & 0xF0) == 0xE0) {
        numBytes = 3;
    } else if ((firstByte & 0xF8) == 0xF0) {
        numBytes = 4;
    }
    return numBytes;
}
void T010012A017F18000(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=YUZU::emu_arg(stack)[2];
    std::string s,bottom;uint32_t c;
    while (true)
    {
        c=*(BYTE*)(address);
        if(c==0)break;
        if(c>=0x20){
            auto l=readu8((BYTE*)address);
            s+=std::string((char*)address,l);
            address+=l;
        }
        else{
            address+=1;
            if(c==1){
                bottom="";
                while(true){
                    auto l=readu8((BYTE*)address);
                    auto ss=std::string((char*)address,l);
                    address+=l;
                    if(ss[0]<0xa)break;
                    bottom+=ss;
                    s+=ss;
                }
            }
            else if(c==3){
                while (true) {
                    auto l=readu8((BYTE*)address);
                    auto ss=std::string((char*)address,l);
                    address+=l;
                    if(ss[0]<0xa)break;
                }
            }
            else if(c==7){
                address+=1;
            }
            else if(c==0xa){
                return;
            }
            else if(c==0xd){
                c = *(uint32_t*)address;
                auto count = c & 0xFF;
                c = c & 0xFFFFFF00;
                if (c == 0x0692c500) {
                    for(int _=0;_<count;_++)s+='-';
                    address +=4;
                }
            }
        }
    }
    
    write_string_new(data,len,s);
}

template<int index>
void ReadTextAndLenDW(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=YUZU::emu_arg(stack)[index];
    *len=(*(DWORD*)(address+0x10))*2;
    *data=address+0x14;
}

template<int index>
void ReadTextAndLenW(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=YUZU::emu_arg(stack)[index];
    *len=(*(WORD*)(address+0x10))*2;
    *data=address+0x14;
}
template<int idx>
void mages_readstring(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto s=mages::readString(YUZU::emu_arg(stack)[0],idx);
    write_string_new(data,len,s);
}


bool F0100A3A00CC7E000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    
    std::wregex pattern1(L"^`([^@]+).");
    s = std::regex_replace(s, pattern1, L"$1: ");

    s = std::regex_replace(s, std::wregex(L"\\$[A-Z]\\d*(,\\d*)*"), L"");

    std::wregex pattern2(L"\\$\\[([^$]+)..([^$]+)..");
    s = std::regex_replace(s, pattern2, L"$1");
	return write_string_overwrite(data,len,s);
}

bool F010045C0109F2000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("#[^\\]]*\\]"), "");
    s = std::regex_replace(s, std::regex("#[^\\n]*\\n"), "");
    s = std::regex_replace(s, std::regex("\\u3000"), "");
    s = std::regex_replace(s, std::regex(u8"Save[\\s\\S]*データ"), "");
	return write_string_overwrite(data,len,s);
}

bool F0100A1E00BFEA000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"[\\s]"), L"");
    s = std::regex_replace(s, std::wregex(L"(.+? \")"), L"");
    s = std::regex_replace(s, std::wregex(L"(\",.*)"), L"");
    s = std::regex_replace(s, std::wregex(L"(\" .*)"), L"");
	return write_string_overwrite(data,len,s);
}



bool F0100A1200CA3C000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"\\$d"), L"\n");
    s = std::regex_replace(s, std::wregex(L"＿"), L" ");
    s = std::regex_replace(s, std::wregex(L"@"), L" ");
    s = std::regex_replace(s, std::wregex(L"\\[([^\\/\\]]+)\\/[^\\/\\]]+\\]"), L"$1");
    s = std::regex_replace(s, std::wregex(L"[~^$❝.❞'?,(-)!—:;-❛ ❜]"), L"");
    s = std::regex_replace(s, std::wregex(L"[A-Za-z0-9]"), L"");
    s = std::regex_replace(s, std::wregex(L"^\\s+"), L"");
    while (std::regex_search(s, std::wregex(L"^\\s*$"))) {
        s = std::regex_replace(s, std::wregex(L"^\\s*$"), L"");
    }
	return write_string_overwrite(data,len,s);
}


bool F0100F6A00A684000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    
    std::regex regex("(?=@.)");
    std::sregex_token_iterator it(s.begin(), s.end(), regex, -1);
    std::sregex_token_iterator end;

    std::vector<std::string> parts(it, end);
    s="";
    for(auto part:parts){
        if (startWith(part,"@")==false) {
            s += part;
            continue;
        }
        std::string tag = part.substr(0, 2);
        std::string content = part.substr(2);
        if (tag == "@r") {
            if(s=="")s=content;
            else s+='\n'+content;
        } else if (tag == "@u"||tag == "@v"||tag == "@w"||tag == "@o"||tag == "@a"||tag == "@z"||tag == "@c"||tag == "@s") {
            auto splited=strSplit(content,".");
            if(splited.size()==2)
                s += splited[1];
        } else if (tag == "@b") {
        } else {
            s += content;
        }
    }
    static auto katakanaMap =std::map<std::wstring,std::wstring>{
        {L"｢",L"「"},{L"｣",L"」"},{L"ｧ",L"ぁ"},{L"ｨ",L"ぃ"},{L"ｩ",L"ぅ"},{L"ｪ",L"ぇ"},{L"ｫ",L"ぉ"},{L"ｬ",L"ゃ"},{
        L"ｭ",L"ゅ"},{L"ｮ",L"ょ"},{L"ｱ",L"あ"},{L"ｲ",L"い"},{L"ｳ",L"う"},{L"ｴ",L"え"},{L"ｵ",L"お"},{L"ｶ",L"か"},{
        L"ｷ",L"き"},{L"ｸ",L"く"},{L"ｹ",L"け"},{L"ｺ",L"こ"},{L"ｻ",L"さ"},{L"ｼ",L"し"},{L"ｽ",L"す"},{L"ｾ",L"せ"},{
        L"ｿ",L"そ"},{L"ﾀ",L"た"},{L"ﾁ",L"ち"},{L"ﾂ",L"つ"},{L"ﾃ",L"て"},{L"ﾄ",L"と"},{L"ﾅ",L"な"},{L"ﾆ",L"に"},{
        L"ﾇ",L"ぬ"},{L"ﾈ",L"ね"},{L"ﾉ",L"の"},{L"ﾊ",L"は"},{L"ﾋ",L"ひ"},{L"ﾌ",L"ふ"},{L"ﾍ",L"へ"},{L"ﾎ",L"ほ"},{
        L"ﾏ",L"ま"},{L"ﾐ",L"み"},{L"ﾑ",L"む"},{L"ﾒ",L"め"},{L"ﾓ",L"も"},{L"ﾔ",L"や"},{L"ﾕ",L"ゆ"},{L"ﾖ",L"よ"},{
        L"ﾗ",L"ら"},{L"ﾘ",L"り"},{L"ﾙ",L"る"},{L"ﾚ",L"れ"},{L"ﾛ",L"ろ"},{L"ﾜ",L"わ"},{L"ｦ",L"を"},{L"ﾝ",L"ん"},{
        L"ｰ",L"ー"},{L"ｯ",L"っ"},{L"､",L"、"},{L"ﾟ",L"？"},{L"ﾞ",L"！"},{L"･",L"…"},{L"?",L"　"},{L"｡",L"。"},{
        L"\uF8F0",L""},{L"\uFFFD",L""} // invalid (shift_jis A0 <=> EF A3 B0) | FF FD - F8 F0)
    };

    auto remap=[](std::string s) {
        std::wstring result;
        auto ws=StringToWideString(s,932).value();
        for (auto _c:ws) {
            std::wstring c;
            c.push_back(_c);
            if(katakanaMap.find(c)!=katakanaMap.end()){
                result += katakanaMap[c];
            }
            else
                result+=c;
        }
        return WideStringToString(result,932);
    };
	return write_string_overwrite(data,len,remap(s));
}
bool F01006590155AC000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    
    std::regex regex("(?=@.)");
    std::sregex_token_iterator it(s.begin(), s.end(), regex, -1);
    std::sregex_token_iterator end;

    std::vector<std::string> parts(it, end);
    s="";
    int counter = 0;
    while (counter < parts.size()) {
        std::string part = parts[counter];
        if (part[0] != '@') {
            s += part;
            counter++;
            continue;
        }
        std::string tag = part.substr(0, 2);
        std::string content = part.substr(2);
        if (tag == "@s" || tag == "@t") {
            s += content.substr(4);
            counter++;
            continue;
        } else if (tag == "@m") {
            s += content.substr(2);
            counter++;
            continue;
        } else if (tag == "@n") {
            s += '\n' + content;
            counter++;
            continue;
        } else if (tag == "@b" || tag == "@a" || tag == "@p" || tag == "@k") {
            s += content;
            counter++;
            continue;
        } else if (tag == "@v" || tag == "@h") {
            std::regex regex("[\\w_-]+");
            s += std::regex_replace(content, regex, "");
            counter++;
            continue;
        } else if (tag == "@r") {
            s += content + parts[counter + 2].substr(1);
            counter += 3;
            continue;
        } else if (tag == "@I") {
            if (content == "@" || parts[counter + 1].substr(0, 2) == "@r") {
                counter++;
                continue;
            }
            std::regex regex(u8"[\\d+─]");
            s += std::regex_replace(content, regex, "");
            counter += 3;
            continue;
        } else {
            s += content;
            counter++;
            continue;
        }
    }
	return write_string_overwrite(data,len,s);
}
bool F01000200194AE000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
        
    static std::string readString_savedSentence="";
    static bool readString_playerNameFlag=false;
    static std::string readString_playerName=u8"ラピス";
     
    std::regex regex("(?=@.)");
    std::sregex_token_iterator it(s.begin(), s.end(), regex, -1);
    std::sregex_token_iterator end;

    std::vector<std::string> parts(it, end);
    s = "";
    size_t counter = 0;
    
    while (counter < parts.size()) {
        const std::string& part = parts[counter];
        
        if (part.empty() || part[0] != '@') {
            s += part;
            counter++;
            continue;
        }
        
        std::string tag = part.substr(0, 2);
        std::string content = part.substr(2);
        
        if (tag == "@*") {
            if (content.find("name") == 0) {
                if (readString_playerName == u8"ラピス") {
                    s += content.substr(4) + readString_playerName + parts[counter + 4].substr(1);
                } else {
                    s += content.substr(4) + parts[counter + 3].substr(1) + parts[counter + 4].substr(1);
                }
                counter += 5;
                continue;
            }
        } else if (tag == "@s" || tag == "@t") {
            s += content.substr(4);
            counter++;
            continue;
        } else if (tag == "@m") {
            s += content.substr(2);
            counter++;
            continue;
        } else if (tag == "@u") {
            readString_playerNameFlag = true;
            readString_savedSentence = "";
            counter++;
            return write_string_overwrite(data,len,"");
        } else if (tag == "@n" || tag == "@b" || tag == "@a" || tag == "@p" || tag == "@k") {
            s += content;
            counter++;
            continue;
        } else if (tag == "@v" || tag == "@h") {
            std::regex regex("[\\w_-]+");
            s += std::regex_replace(content, regex, "");
            counter++;
            continue;
        } else if (tag == "@r") {
            s += content + parts[counter + 2].substr(1);
            counter += 3;
            continue;
        } else if (tag == "@I") {
            if (content == "@" || parts[counter + 1].substr(0, 2) == "@r") {
                counter++;
                continue;
            }
            std::regex regex(u8"[\\d+─]");
            s += std::regex_replace(content, regex, "");
            counter += 3;
            continue;
        } else {
            s += content;
            counter++;
            continue;
        }
    }
    
    if (!readString_playerNameFlag) {
        ;
    } else if (readString_savedSentence.empty()) {
        readString_savedSentence = s;
        s= "";
    } else {
        std::string savedSentence = readString_savedSentence;
        readString_playerNameFlag = false;
        readString_savedSentence = "";
        readString_playerName = s;
        s= s + "\n" + savedSentence;
    }
	return write_string_overwrite(data,len,s);
}
bool F0100EA001A626000(void* data, size_t* len, HookParam* hp){
    auto s=utf32_to_utf16((uint32_t*)data,*len/4);
    if (s == L"　　") {
        return false;
    }
    s = std::regex_replace(s, std::wregex(L"\n+"), L" ");

    s = std::regex_replace(s, std::wregex(L"\\$\\{FirstName\\}"), L"ナーヤ");

    if (startWith(s,L"#T")){
        s = std::regex_replace(s, std::wregex(L"#T2[^#]+"), L"");
        s = std::regex_replace(s, std::wregex(L"#T\\d"), L"");
    }
    auto u32=utf16_to_utf32(s.c_str(),s.size());
    return write_string_overwrite(data,len,u32);
}
bool F010093800DB1C000(void* data, size_t* len, HookParam* hp){
    auto s=utf32_to_utf16((uint32_t*)data,*len/4);
    s = std::regex_replace(s, std::wregex(L"\\n+"), L" ");
    s = std::regex_replace(s, std::wregex(L"\\$\\{FirstName\\}"), L"シリーン");
    if (startWith(s,L"#T")) {
        s = std::regex_replace(s, std::wregex(L"\\#T2[^#]+"), L"");
        s = std::regex_replace(s, std::wregex(L"\\#T\\d"), L"");
    }
    auto u32=utf16_to_utf32(s.c_str(),s.size());
    return write_string_overwrite(data,len,u32);
}
bool F0100F7E00DFC8000(void* data, size_t* len, HookParam* hp){
    auto s=utf32_to_utf16((uint32_t*)data,*len/4);
    s = std::regex_replace(s, std::wregex(L"[\\s]"), L" "); 
    s = std::regex_replace(s, std::wregex(L"#KW"), L"");
    s = std::regex_replace(s, std::wregex(L"#C\\(TR,0xff0000ff\\)"), L"");
    s = std::regex_replace(s, std::wregex(L"#P\\(.*\\)"), L"");
    auto u32=utf16_to_utf32(s.c_str(),s.size());
    return write_string_overwrite(data,len,u32);
}

bool F0100982015606000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"\\n+|(\\\\n)+"), L" ");
    return write_string_overwrite(data,len,s);
}
bool F0100C4E013E5E000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"\\\\n"), L" ");
    return write_string_overwrite(data,len,s);
}

bool F010001D015260000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    if(startWith(s,"#Key"))return false;
    strReplace(s,"#n","\n");
    return write_string_overwrite(data,len,s);
}
bool F0100E1E00E2AE000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("#n"), "\n");
    s = std::regex_replace(s, std::regex("[A-Za-z0-9]"), "");
    s = std::regex_replace(s, std::regex("[~^,\\-\\[\\]#]"), "");
    return write_string_overwrite(data,len,s);
}
bool F0100DE200C0DA000(void* data, size_t* len, HookParam* hp){
    StringReplacer((char*)data,len,"#n",2," ",1);
    StringReplacer((char*)data,len,"\n",1," ",1);
    return true;
}
bool F0100AEC013DDA000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    static std::string ss;
    if(ss==s)return false;
    ss=s;
    return true;
}

bool F0100F7801B5DC000(void* data, size_t* len, HookParam* hp){
    if(!all_ascii((wchar_t*)data))return false;//chaos on first load.
    StringReplacer((wchar_t*)data,len,L"<br>",4,L"\n",1);
    return true;
}

bool F0100925014864000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("(#n)+"), " ");
    s = std::regex_replace(s, std::regex("(#[A-Za-z]+\\[(\\d*[.])?\\d+\\])+"), "");
    return write_string_overwrite(data,len,s);
}

bool F0100936018EB4000(void* data, size_t* len, HookParam* hp){
    auto s=utf32_to_utf16((uint32_t*)data,*len/4);
    s = std::regex_replace(s, std::wregex(L"<[^>]+>"), L"");
    s = std::regex_replace(s, std::wregex(L"\n+"), L" ");
    auto u32=utf16_to_utf32(s.c_str(),s.size());
    return write_string_overwrite(data,len,u32);
}

template<int index,int type>
void T0100B0100E26C000(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=YUZU::emu_arg(stack)[index];
    if(type==2)
        address+=0xA;
    auto length=(*(DWORD*)(address+0x10))*2;
    *len=length;
    *data=address+0x14;
}

bool F010045C014650000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    std::regex pattern("(@(\\/)?[a-zA-Z#](\\(\\d+\\))?|)[*<>]+");
    s = std::regex_replace(s, pattern, "");
    return write_string_overwrite(data,len,s);
}

bool F01008C0016544000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"<[^>]+>"), L" ");
    return write_string_overwrite(data,len,s);
}
bool F0100FB7019ADE000(void* data, size_t* len, HookParam* hp){
    static int idx=0;
    return ((++idx)%2==1);
}
bool F01006F000B056000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"\\[.*?\\]"), L" ");
    return write_string_overwrite(data,len,s);
}
bool F0100068019996000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("%N"), "\n");
    return write_string_overwrite(data,len,s);
}
bool F0100ADC014DA0000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    std::wregex symbolRegex(L"[~^$(,)]");
    std::wregex alphanumericRegex(L"[A-Za-z0-9]");
    std::wregex atRegex(L"@");
    std::wregex leadingSpaceRegex(L"^\\s+");
    s = std::regex_replace(s, symbolRegex, L"");
    s = std::regex_replace(s, alphanumericRegex, L"");
    s = std::regex_replace(s, atRegex, L" ");
    s = std::regex_replace(s, leadingSpaceRegex, L"");
    return write_string_overwrite(data,len,s);
}
bool F0100AFA01750C000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    std::regex newlineRegex("(\\\\n)+");
    std::regex specialCharsRegex("\\\\d$|^\\@[a-z]+|#.*?#|\\$");
    s = std::regex_replace(s, newlineRegex, " ");
    s = std::regex_replace(s, specialCharsRegex, "");
    return write_string_overwrite(data,len,s);
}
bool F0100B6900A668000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("#N"), "\n");
    std::regex colorRegex("#Color\\[[\\d]+\\]");
    s = std::regex_replace(s, colorRegex, "");
    return write_string_overwrite(data,len,s);
}
bool F0100BD700E648000(void* data, size_t* len, HookParam* hp){
    StringReplacer((char*)data,len,"*",1," ",1);
    StringReplacer((char*)data,len,u8"ゞ",sizeof(u8"ゞ"),u8"！？",sizeof(u8"！？"));
    return true;
}
bool F0100D9500A0F6000(void* data, size_t* len, HookParam* hp){
    StringReplacer((char*)data,len,u8"㊤",sizeof(u8"㊤"),u8"―",sizeof(u8"―"));
    StringReplacer((char*)data,len,u8"㊥",sizeof(u8"㊥"),u8"―",sizeof(u8"―"));
    StringReplacer((char*)data,len,u8"^㌻",sizeof(u8"^㌻"),u8" ",sizeof(u8" "));// \n
    return true;
}

bool F0100DA201E0DA000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"[\\s]"), L"");
	return write_string_overwrite(data,len,s);
}
bool F01002C0008E52000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("(YUR)"), u8"ユーリ");
    s = std::regex_replace(s, std::regex("(FRE)"), u8"フレン");
    s = std::regex_replace(s, std::regex("(RAP)"), u8"ラピード");
    s = std::regex_replace(s, std::regex("(EST|ESU)"), u8"エステル");
    s = std::regex_replace(s, std::regex("(KAR)"), u8"カロル");
    s = std::regex_replace(s, std::regex("(RIT)"), u8"リタ");
    s = std::regex_replace(s, std::regex("(RAV|REI)"), u8"レイヴン");
    s = std::regex_replace(s, std::regex("(JUD)"), u8"ジュディス");
    s = std::regex_replace(s, std::regex("(PAT)"), u8"パティ");
    s = std::regex_replace(s, std::regex("(DUK|DYU)"), u8"デューク");
    s = std::regex_replace(s, std::regex("[A-Za-z0-9]"), "");
    s = std::regex_replace(s, std::regex("[,(-)_]"), "");
    s = std::regex_replace(s, std::regex("^\\s+"), "");
    while (std::regex_search(s, std::regex("^\\s*$"))) {
        s = std::regex_replace(s, std::regex("^\\s*$"), "");
    }
	return write_string_overwrite(data,len,s);
}

bool F01005940182EC000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    std::wregex whitespaceRegex(L"\\s");
    s = std::regex_replace(s, whitespaceRegex, L"");
    std::wregex colorRegex(L"<color=.*?>(.*?)<\\/color>");
    s = std::regex_replace(s, colorRegex, L"$1");
	return write_string_overwrite(data,len,s);
}
void T01005940182EC000(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=YUZU::emu_arg(stack)[0];
    *data=address+0x14;
    *len=wcslen((wchar_t*)*data)*2;
}
template<int idx>
bool F0100B0601852A000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    static std::wstring last;
    if(last==s)return false;
    last=s;
	return true;
}
bool F010027100C79A000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    static std::string last;
    if(last==s)return false;
    last=s;
	return true;
}
template<int idx>
bool F0100B0C016164000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    std::wregex htmlTagsPattern(L"<[^>]*>");
    std::wregex lettersAndNumbersPattern(L"[A-Za-z0-9]");
    s = std::regex_replace(s, htmlTagsPattern, L"");
    s = std::regex_replace(s, lettersAndNumbersPattern, L"");
    static std::wstring last;
    if(last==s)return false;
    last=s;
	return write_string_overwrite(data,len,s);
}

template<int i>
bool F010043B013C5C000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    std::wregex htmlTagsPattern(L"<[^>]*>");
    s = std::regex_replace(s, htmlTagsPattern, L"");
    static std::wstring last;
    if(last==s)return false;
    last=s;
	return write_string_overwrite(data,len,s);
}
template<int i>
bool F010055D009F78000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    std::regex pattern3("\\d+");
    s = std::regex_replace(s, pattern3, "");
	static std::string last;
    if(last==s)return false;
    last=s;
	return write_string_overwrite(data,len,s);
}

bool F010080C01AA22000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    std::regex furiganaRegex("#\\d+R.*?#");
    s = std::regex_replace(s, furiganaRegex, "");
    std::regex lettersNumbersRegex("[A-Za-z0-9]");
    s = std::regex_replace(s, lettersNumbersRegex, "");
    std::regex symbolsRegex(u8"[().%,_!#©&:?/]");
    s = std::regex_replace(s, symbolsRegex, "");
	return write_string_overwrite(data,len,s);
}
template<int i>
bool F0100CB700D438000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    std::regex furiganaRegex("<RUBY><RB>(.*?)<\\/RB><RT>(.*?)<\\/RT><\\/RUBY>");
    s = std::regex_replace(s, furiganaRegex, "$1");
    std::regex htmlTagRegex("<[^>]*>");
    s = std::regex_replace(s, htmlTagRegex, "");
    static std::string last;
    if(last==s)return false;
    last=s;
	return write_string_overwrite(data,len,s);
}
bool F01005C301AC5E000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex(".*_.*_.*"), "");//SIR_C01_016,ERU_C00_000
    s = std::regex_replace(s, std::regex("\\.mp4"), "");
    s = std::regex_replace(s, std::regex("@v"), "");
    s = std::regex_replace(s, std::regex("@n"), "\n");
	return write_string_overwrite(data,len,s);
}
bool F0100815019488000_text(void* data, size_t* len, HookParam* hp){
    //@n@vaoi_s01_0110「うんうん、そうかも！」
    auto s = std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("@.*_.*_\\d+"), "");
    s = std::regex_replace(s, std::regex("@n"), "");
	return write_string_overwrite(data,len,s);
}
bool F0100815019488000_name(void* data, size_t* len, HookParam* hp){
    //  あおい@n@vaoi_s01_0110「うんうん、そうかも！」
    auto s = std::string((char*)data,*len);
    if(s.find("@n")==s.npos)return false;
    s = std::regex_replace(s, std::regex("(.*)@n.*"), "$1");
	return write_string_overwrite(data,len,s);
}
template<int i>
bool F010072000BD32000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    std::regex lineBreakRegex("\\[~\\]");
    s = std::regex_replace(s, lineBreakRegex, "\n");
    std::regex romRegex("rom:[\\s\\S]*$");
    s = std::regex_replace(s, romRegex, "");
    std::regex furiganaRegex("\\[[\\w\\d]*\\[[\\w\\d]*\\].*?\\[\\/\\[\\w\\d]*\\]\\]");
    s = std::regex_replace(s, furiganaRegex, "");
    std::regex bracketsRegex("\\[.*?\\]");
    s = std::regex_replace(s, bracketsRegex, "");
    static std::string last;
    if(last==s)return false;
    last=s;
	return write_string_overwrite(data,len,s);
}
template<int i>
bool F01009B50139A8000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    std::wregex htmlTagRegex(L"<[^>]*>");
    s = std::regex_replace(s, htmlTagRegex, L"");
    std::wregex hoursRegex(L"\\b\\d{2}:\\d{2}\\b");
    s = std::regex_replace(s, hoursRegex, L"");
    
    auto _=L"^(?:スキップ|むしる|取り出す|話す|選ぶ|ならびかえ|閉じる|やめる|undefined|決定|ボロのクワ|拾う)$(\\r?\\n|\\r)?";
    while (std::regex_search(s, std::wregex(_))) {
        s = std::regex_replace(s, std::wregex(_), L"");
    }
    while (std::regex_search(s, std::wregex(L"^\\s*$"))) {
        s = std::regex_replace(s, std::wregex(L"^\\s*$"), L"");
    }
    static std::wstring last;
    if(last==s)return false;
    last=s;
	return write_string_overwrite(data,len,s);
}
template<int i>
bool F0100BD4014D8C000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"<[^>]*>"), L"");
    s = std::regex_replace(s, std::wregex(L".*?_"), L"");
    static std::wstring last;
    if(last==s)return false;
    last=s;
	return write_string_overwrite(data,len,s);
}
template<int i>
bool F0100C7400CFB4000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"\\d"), L"");
    s = std::regex_replace(s, std::wregex(L"<[^>]*>"), L"");
    while (std::regex_search(s, std::wregex(L"^\\s*$"))) {
        s = std::regex_replace(s, std::wregex(L"^\\s*$"), L"");
    }
    static std::wstring last;
    if(last==s)return false;
    last=s;
	return write_string_overwrite(data,len,s);
}
bool F0100CB9018F5A000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"<[^>]*>"), L"");
    s = std::regex_replace(s, std::wregex(L"\\{([^{}]+):[^{}]+\\}"), L"$1");
	return write_string_overwrite(data,len,s);
}

bool F010028D0148E6000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("{|\\/.*?}|\\[.*?]", std::regex_constants::grep), "");
    s = std::regex_replace(s, std::regex("(\\\\\\\\c|\\\\\\\\n)+"), " ");
    s = std::regex_replace(s, std::regex(",.*$"), " ");
	return write_string_overwrite(data,len,s);
}

bool F0100F4401940A000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"[\\r\\n]+"), L"");
    s = std::regex_replace(s, std::wregex(L"<[^>]+>|\\[\\[[^]]+\\]\\]"), L"");
	return write_string_overwrite(data,len,s);
}

bool F0100B5500CA0C000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    std::regex pattern1("\\\\u0000+$");
    std::regex pattern2("\\\\");
    std::regex pattern3("\\$");
    s = std::regex_replace(s, pattern1, "");
    s = std::regex_replace(s, pattern2, "");
    s = std::regex_replace(s, pattern3, "");
	return write_string_overwrite(data,len,s);
}
void T0100B5500CA0C000(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=YUZU::emu_arg(stack)[3];
    *data=address;
    *len=*(WORD*)(address-2);
}
bool F0100A8401A0A8000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    std::wregex samePageNewLineRegex(L"[\r\n]+");
    s = std::regex_replace(s, samePageNewLineRegex, L"");
    std::wregex newPageTextRegex(L"(<.+?>)+");
    s = std::regex_replace(s, newPageTextRegex, L"\r\n");
    strReplace(s,L"",L"(L)");
    strReplace(s,L"",L"(ZL)");
    strReplace(s,L"",L"(Y)");
    strReplace(s,L"",L"(X)");
    strReplace(s,L"",L"(A)");
    strReplace(s,L"",L"(B)");
    strReplace(s,L"",L"(+)");
    strReplace(s,L"",L"(-)");
    strReplace(s,L"",L"(DPAD_DOWN)");
    strReplace(s,L"",L"(DPAD_LEFT)");
    strReplace(s,L"",L"(LSTICK)");
    strReplace(s,L"",L"(L3)");
	return write_string_overwrite(data,len,s);
}
bool F0100BC0018138000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    std::wregex tagContentRegex(L"<[^>]*>([^<]*)<\\/[^>]*>");
    s = std::regex_replace(s, tagContentRegex, L"");
    s = std::regex_replace(s, std::wregex(L"<sprite name=L>"), L"L");
    s = std::regex_replace(s, std::wregex(L"<sprite name=R>"), L"R");
    s = std::regex_replace(s, std::wregex(L"<sprite name=A>"), L"A");
    s = std::regex_replace(s, std::wregex(L"<sprite name=B>"), L"B");
    s = std::regex_replace(s, std::wregex(L"<sprite name=X>"), L"X");
    s = std::regex_replace(s, std::wregex(L"<sprite name=Y>"), L"Y");
    s = std::regex_replace(s, std::wregex(L"<sprite name=PLUS>"), L"+");
    s = std::regex_replace(s, std::wregex(L"<sprite name=MINUS>"), L"-");
    s = std::regex_replace(s, std::wregex(L"<[^>]+>"), L"");
	return write_string_overwrite(data,len,s);
}
bool F0100D7800E9E0000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"[A-Za-z0-9]"), L"");
    s = std::regex_replace(s, std::wregex(L"<[^>]*>"), L"");
    s = std::regex_replace(s, std::wregex(L"^二十五字二.*(\r?\n|\r)?"), L"");
    s = std::regex_replace(s, std::wregex(L"^操作を割り当てる.*(\r?\n|\r)?"), L"");
    s = std::regex_replace(s, std::wregex(L"^上記アイコンが出.*(\r?\n|\r)?"), L"");
    s = std::regex_replace(s, std::wregex(L"[()~^,ö.!]"), L"");
    static std::wstring last;
    if(last==s)return false;
    last=s;
	return write_string_overwrite(data,len,s);
}
template<int idx>
void T0100CF400F7CE000(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=YUZU::emu_arg(stack)[idx];
    std::string s;
    int i=0;
    while(1){
        auto c=*(BYTE*)(address+i);
        if(c==0)break;
        if(c<0x20&&c>0x10){
            auto command=*(BYTE*)(address+i+1);
            if(command==0x80)
                i+=3;
            else if(command==0xb8)
                i+=4;
            else{
                auto sz=*(BYTE*)(address+i+2);
                i+=3+sz;
            }
        }
        else if(c==0xaa){
            i+=1;
        }
        else if(c==0xff){
            i+=0x30;
        }
        else{
            auto l=1+IsDBCSLeadByteEx(932,c);
            s+=std::string((char*)(address+i),l);
            i+=l;
        }
    }
    write_string_new(data,len,s);
}
void T0100DB300B996000(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=YUZU::emu_arg(stack)[8]+1;
    std::string s;
    int i=0;
    while(1){
        auto c=*(BYTE*)(address+i);
        if(c==0)break;
        if(c<0x20&&c>0x10){
            auto sz=*(BYTE*)(address+i+2);
            i+=3+sz;
        }
        else{
            auto l=1+IsDBCSLeadByteEx(932,c);
            s+=std::string((char*)(address+i),l);
            i+=l;
        }
    }
    write_string_new(data,len,s);
}
bool F0100CBA014014000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex(u8"《.*?》"), "");
    s = std::regex_replace(s, std::regex("<[^>]*>"), "");
	return write_string_overwrite(data,len,s);
}
template<int idx>
bool F0100CC401A16C000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("<[^>]*>"), "");
    s = std::regex_replace(s, std::regex("\\d+"), "");
    if(s=="")return false;
    static std::string last;
    if(last==s)return false;
    last=s;
	return write_string_overwrite(data,len,s);
}
bool F0100BDD01AAE4000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("(#Ruby\\[)([^,]+).([^\\]]+)."), "$2");
    s = std::regex_replace(s, std::regex("(#n)+"), " ");
    s = std::regex_replace(s, std::regex("(#[A-Za-z]+[(\\d*[.])?\\d+])+"), "");
	return write_string_overwrite(data,len,s);
}
bool F0100C310110B4000(void* data, size_t* len, HookParam* hp){
    auto s = std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("(#Ruby\\[)([^,]+).([^\\]]+)."), "$2");
    s = std::regex_replace(s, std::regex("#Color\\[[\\d]+\\]"), "");
    s = std::regex_replace(s, std::regex(u8"(　#n)+"), "#n");
    s = std::regex_replace(s, std::regex("#n+"), " ");
	return write_string_overwrite(data,len,s);
}
bool F010003F003A34000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"[\\s\\S]*$"), L"");
    s = std::regex_replace(s, std::wregex(L"\n+"), L" ");
    s = std::regex_replace(s, std::wregex(L"\\s"), L"");
    s = std::regex_replace(s, std::wregex(L"[＀븅]"), L"");
	return write_string_overwrite(data,len,s);
}


bool F01007B601C608000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"<[^>]*>"), L"");
    s = std::regex_replace(s, std::wregex(L"\\[.*?\\]"), L"");
    std::vector<std::wstring> lines = strSplit(s, L"\n");
    std::wstring result;
    for (const std::wstring& line : lines) {
        if(result.empty()==false)result+=L"\n";
        std::wregex commandRegex(L"^(?:メニュー|システム|Ver\\.)$(\\r?\\n|\\r)?");
        s = std::regex_replace(s, commandRegex, L"");
        std::wregex emptyLineRegex(L"^\\s*$");
        s = std::regex_replace(s, emptyLineRegex, L"");
    }
    static std::wstring last;
    if(last==s)return false;
    last=s;
	return write_string_overwrite(data,len,s);
}

bool F010046601125A000(void* data, size_t* len, HookParam* hp){
    auto s=utf32_to_utf16((uint32_t*)data,*len/4);
    s = std::regex_replace(s, std::wregex(L"<rb>(.+?)</rb><rt>.+?</rt>"), L"$1");
    s = std::regex_replace(s, std::wregex(L"\n+"), L" ");
    auto u32=utf16_to_utf32(s.c_str(),s.size());
	return write_string_overwrite(data,len,u32);
}
bool F0100771013FA8000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"<br>"), L"\n");
    s = std::regex_replace(s, std::wregex(L"^(\\s+)"), L"");
	return write_string_overwrite(data,len,s);
}
bool F0100556015CCC000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    std::regex rubiRegex("\\[[^\\]]+.");
    s = std::regex_replace(s, rubiRegex, "");
    s = std::regex_replace(s, std::regex("\\\\k|\\\\x|%C|%B|%p-1;"), "");
    std::regex colorRegex("#[0-9a-fA-F]+;([^%#]+)(%r)?");
    s = std::regex_replace(s, colorRegex, "$1");
    static std::set<std::string>dump;
    if(dump.find(s)!=dump.end())return false;
    dump.insert(s);
	return write_string_overwrite(data,len,s);
}
template<int i>
bool F0100CC80140F8000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"^(?:スキップ|メニュー|バックログ|ズームイン|ズームアウト|ガイド OFF|早送り|オート|人物情報|ユニット表示切替|カメラリセット|ガイド表示切替|ページ切替|閉じる|コマンド選択|詳細|シミュレーション|移動)$([\\r?\\n|\\r])?"), L"");

    s = std::regex_replace(s, std::wregex(L"[A-Za-z0-9]"), L"");
    s = std::regex_replace(s, std::wregex(L"[().%,_!#©&:?/]"), L"");
    while (std::regex_search(s, std::wregex(L"^\\s*$"))) {
        s = std::regex_replace(s, std::wregex(L"^\\s*$"), L"");
    }
    static std::wstring last;
    if(last==s)return false;
    last=s;
	return write_string_overwrite(data,len,s);
}

bool F0100D9A01BD86000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"[\\s]"), L"");
    s = std::regex_replace(s, std::wregex(L"\\\\n"), L"");
	return write_string_overwrite(data,len,s);
}
bool F010042300C4F6000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"[\\s]"), L"");
    s = std::regex_replace(s, std::wregex(L"(.+?/)"), L"");
    s = std::regex_replace(s, std::wregex(L"(\" .*)"), L"");
    s = std::regex_replace(s, std::wregex(L"^(.+?\")"), L"");
	return write_string_overwrite(data,len,s);
}
bool F010044800D2EC000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"\\n+"), L" ");
    s = std::regex_replace(s, std::wregex(L"\\<PL_N\\>"), L"???");
    s = std::regex_replace(s, std::wregex(L"<.+?>"), L"");
	return write_string_overwrite(data,len,s);
}
template<int i>
bool F010021300F69E000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"\\$[a-z]"), L"");
    s = std::regex_replace(s, std::wregex(L"@"), L"");
    static std::wstring last;
    if(last==s)return false;
    last=s;
	return write_string_overwrite(data,len,s);
}
bool F010050000705E000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("\\s"), "");
    s = std::regex_replace(s, std::regex("<br>"), "\n");
    s = std::regex_replace(s, std::regex("<([^:>]+):[^>]+>"), "$1");
    s = std::regex_replace(s, std::regex("<[^>]+>"), "");
	return write_string_overwrite(data,len,s);
}
bool F01001B900C0E2000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    std::regex whitespaceRegex("\\s");
    s = std::regex_replace(s, whitespaceRegex, "");
    std::regex hashRegex("#[A-Za-z]+(\\[(\\d*\\.)?\\d+\\])+");
    s = std::regex_replace(s, hashRegex, "");
    std::regex hashLetterRegex("#[a-z]");
    s = std::regex_replace(s, hashLetterRegex, "");
    std::regex lowercaseRegex("[a-z]");
    s = std::regex_replace(s, lowercaseRegex, "");
	return write_string_overwrite(data,len,s);
}

bool F0100217014266000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    std::wregex htmlTagRegex(L"<[^>]*>");
    s = std::regex_replace(s, htmlTagRegex, L"");
    std::wregex furiganaRegex(L"｛([^｛｝]+)：[^｛｝]+｝");
    s = std::regex_replace(s, furiganaRegex, L"$1");
    while (std::regex_search(s, std::wregex(L"^\\s+"))) {
        s = std::regex_replace(s, std::wregex(L"^\\s+"), L"");
    }
    return write_string_overwrite(data,len,s);
}
bool F010007500F27C000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    std::wregex htmlTagRegex(L"<[^>]*>");
    s = std::regex_replace(s, htmlTagRegex, L"");
    auto _=L"^(?:決定|進む|ページ移動|ノート全体図|閉じる|もどる|セーブ中)$(\\r?\\n|\\r)?";
    while (std::regex_search(s, std::wregex(_))) {
        s = std::regex_replace(s, std::wregex(_), L"");
    }
    while (std::regex_search(s, std::wregex(L"^\\s*$"))) {
        s = std::regex_replace(s, std::wregex(L"^\\s*$"), L"");
    }
    static std::wstring last;
    if(last==s)return false;
    last=s;
    return write_string_overwrite(data,len,s);
}
bool F0100874017BE2000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"\\n+|(\\\\n)+"), L" ");
    s = std::regex_replace(s, std::wregex(L"#n"), L"");
    return write_string_overwrite(data,len,s);
}
bool F010094601D910000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"\\<.*?\\>"), L"");
    s = std::regex_replace(s, std::wregex(L"\\[.*?\\]"), L"");
    s = std::regex_replace(s, std::wregex(L"\\s"), L"");
    return write_string_overwrite(data,len,s);
}
bool F010079201BD88000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"[\\s]"), L"");
    s = std::regex_replace(s, std::wregex(L"\\\\n"), L"");
    return write_string_overwrite(data,len,s);
}
bool F010086C00AF7C000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("\\[([^\\]]+)\\/[^\\]]+\\]"), "$1");
    s = std::regex_replace(s, std::regex("\\s+"), " ");
    s = std::regex_replace(s, std::regex("\\\\n"), " ");
    s = std::regex_replace(s, std::regex("<[^>]+>|\\[[^\\]]+\\]"), "");
    return write_string_overwrite(data,len,s);
}
bool F010079C017B98000(void* data, size_t* len, HookParam* hp){
    auto s=utf32_to_utf16((uint32_t*)data,*len/4);
    s = std::regex_replace(s, std::wregex(L"[\\s]"), L"");
    s = std::regex_replace(s, std::wregex(L"#KW"), L"");
    s = std::regex_replace(s, std::wregex(L"#C\\(TR,0xff0000ff\\)"), L"");
    s = std::regex_replace(s, std::wregex(L"【SW】"), L"");
    s = std::regex_replace(s, std::wregex(L"【SP】"), L"");
    s = std::regex_replace(s, std::wregex(L"#P\\(.*\\)"), L"");
    auto u32=utf16_to_utf32(s.c_str(),s.size());
    return write_string_overwrite(data,len,u32);
}
bool F010061A01C1CE000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"[\\s]"), L"");
    s = std::regex_replace(s, std::wregex(L"sound"), L" ");
    return write_string_overwrite(data,len,s);
}
bool F0100F7401AA74000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("[\\s]"), "");
    s = std::regex_replace(s, std::regex("@[a-z]"), "");
    s = std::regex_replace(s, std::regex("@[0-9]"), "");
    return write_string_overwrite(data,len,s);
}

bool F0100FC2019346000(void* data, size_t* len, HookParam* hp){
    StringReplacer((char*)data,len,"#n",2," ",1);
    return true;
}

bool F0100FDB00AA80000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("\\[([^\\]]+)\\/[^\\]]+\\]"), "$1");
    s = std::regex_replace(s, std::regex("<[^>]*>"), "");
    return write_string_overwrite(data,len,s);
}
bool F0100FF500E34A000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("\\[.*?\\]"), "");
    s = std::regex_replace(s, std::regex("\\n+"), " ");
    return write_string_overwrite(data,len,s);
}
bool F01005E9016BDE000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    std::regex patt("/\\/\\/ remove rubi\\n\\ss = s.replace\\(patt, ''\\);/");
    s = std::regex_replace(s, patt, "");
    s = std::regex_replace(s, std::regex("\\\\k|\\\\x|%C|%B|%p-1;"), "");
    s = std::regex_replace(s, std::regex("#[0-9a-fA-F]+;([^%#]+)(%r)?"), "$1");
    s = std::regex_replace(s, std::regex("\\\\n"), " ");
    return write_string_overwrite(data,len,s);
}

bool F010065301A2E0000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"<WaitFrame>\\d+</WaitFrame>"), L"");
    s = std::regex_replace(s, std::wregex(L"<[^>]*>"), L"");
    return write_string_overwrite(data,len,s);
}
bool F01002AE00F442000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    std::wregex pattern1(L"\\[([^\\]\\/]+)\\/[^\\]]+\\]");
    s = std::regex_replace(s, pattern1, L"$1");
    std::wregex pattern2(L"(\\S*)@");
    s = std::regex_replace(s, pattern2, L"$1");
    std::wregex pattern3(L"\\$");
    s = std::regex_replace(s, pattern3, L"");
    return write_string_overwrite(data,len,s);
}
bool F01000A400AF2A000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"@[a-zA-Z]|%[a-zA-Z]+"), L"");
    static std::wstring last;
    if(last==s)return false;
    last=s;
    return write_string_overwrite(data,len,s);
}

bool F01006B5014E2E000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("@r(.*?)@(.*?)@"), "$1");
    s = std::regex_replace(s, std::regex("@n"), "");
    s = std::regex_replace(s, std::regex("@v"), "");
    s = std::regex_replace(s, std::regex("TKY[0-9]{6}_[A-Z][0-9]{2}"), "");
    return write_string_overwrite(data,len,s);
}
bool F0100CF400F7CE000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("\\n+"), " ");
    return write_string_overwrite(data,len,s);
}
bool F01000AE01954A000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("[A-Za-z0-9]"), "");
    s = std::regex_replace(s, std::regex("[~^(-).%,!:#@$/*&;+_]"), "");
    return write_string_overwrite(data,len,s);
}
bool F01003BD013E30000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("{|\\/.*?}|\\[.*?]"), "");
    return write_string_overwrite(data,len,s);
}
bool F010074F013262000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("\\[.*?]"), "");
    return write_string_overwrite(data,len,s);
}
template<int i>
bool F010057E00AC56000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("<[^>]*>"), "");
    s = std::regex_replace(s, std::regex(u8"ズーム|回転|身長|体重"), "");
    s = std::regex_replace(s, std::regex("[A-Za-z0-9]"), "");
    s = std::regex_replace(s, std::regex("[().%,!#/]"), "");
    while (std::regex_search(s, std::regex("^\\s*$"))) {
        s = std::regex_replace(s, std::regex("^\\s*$"), "");
    }
    static std::string last;
    if(last==s)return false;
    last=s;
    return write_string_overwrite(data,len,s);
}
bool F010051D010FC2000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    s = std::regex_replace(s, std::regex("\\[([^\\]]+)\\/[^\\]]+\\]"), "$1");
    s = std::regex_replace(s, std::regex("\\s+"), " ");
    s = std::regex_replace(s, std::regex("\\\\n"), " ");
    s = std::regex_replace(s, std::regex("<[^>]+>|\\[[^\\]]+\\]"), "");
    return write_string_overwrite(data,len,s);
}

bool F01001EF017BE6000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    while (std::regex_search(s, std::wregex(L"^\\s*$"))) {
        s = std::regex_replace(s, std::wregex(L"^\\s*$"), L"");
    }
	return write_string_overwrite(data,len,s);
}
bool F01000EA00D2EE000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"\\n+"), L" ");
    s = std::regex_replace(s, std::wregex(L"\\<PL_Namae\\>"), L"???");
    s = std::regex_replace(s, std::wregex(L"\\<chiaki_washa\\>"), L"chiaki_washa");
    s = std::regex_replace(s, std::wregex(L"<.+?>"), L"");
	return write_string_overwrite(data,len,s);
}
auto _=[](){
    emfunctionhooks={
        //Memories Off
        {0x8003eeac,{CODEC_UTF16,0,0,mages_readstring<0>,0,"0100978013276000","1.0.0"}},
        {0x8003eebc,{CODEC_UTF16,0,0,mages_readstring<0>,0,"0100978013276000","1.0.1"}},
        //Memories Off ~Sorekara~
        {0x8003fb7c,{CODEC_UTF16,0,0,mages_readstring<0>,0,"0100B4A01326E000","1.0.0"}},
        {0x8003fb8c,{CODEC_UTF16,0,0,mages_readstring<0>,0,"0100B4A01326E000","1.0.1"}},
        //Famicom Tantei Club: Kieta Koukeisha
        {0x80052a10,{CODEC_UTF16,0,0,mages_readstring<3>,0,"0100B4500F7AE000","1.0.0"}},
        //Famicom Tantei Club Part: Ushiro ni Tatsu Shoujo
        {0x8004cb30,{CODEC_UTF16,0,0,mages_readstring<3>,0,"010078400F7B0000","1.0.0"}},
        //Memories Off 2nd
        {0x8003ee0c,{CODEC_UTF16,0,0,mages_readstring<0>,0,"0100D31013274000","1.0.0"}},
        {0x8003ee1c,{CODEC_UTF16,0,0,mages_readstring<0>,0,"0100D31013274000","1.0.1"}},
        //Omoide ni Kawaru Kimi ~Memories Off~
        {0x8003ef6c,{CODEC_UTF16,0,0,mages_readstring<0>,0,"0100FFA013272000","1.0.0"}},
        {0x8003ef7c,{CODEC_UTF16,0,0,mages_readstring<0>,0,"0100FFA013272000","1.0.1"}},
        //Memories Off 6 ~T-Wave~
        {0x80043d7c,{CODEC_UTF16,0,0,mages_readstring<0>,0,"010047A013268000","1.0.0"}},
        {0x80043d5c,{CODEC_UTF16,0,0,mages_readstring<0>,0,"010047A013268000","1.0.1"}},
        //Memories Off: Yubikiri no Kioku
        {0x800440ec,{CODEC_UTF16,0,0,mages_readstring<0>,0,"010079C012896000","1.0.0"}},
        //Memories Off #5 Togireta Film
        {0x8003f6ac,{CODEC_UTF16,0,0,mages_readstring<0>,0,"010073901326C000","1.0.0"}},
        {0x8003f5fc,{CODEC_UTF16,0,0,mages_readstring<0>,0,"010073901326C000","1.0.1"}},
        //SINce Memories: Hoshi no Sora no Shita de
        {0x80048cc8,{CODEC_UTF16,0,0,mages_readstring<4>,0,"0100E94014792000",0}},//line + name => join
        {0x8004f44c,{CODEC_UTF16,0,0,mages_readstring<4>,0,"0100E94014792000",0}},//fast trophy
        {0x8004f474,{CODEC_UTF16,0,0,mages_readstring<4>,0,"0100E94014792000",0}},//prompt
        {0x80039dc0,{CODEC_UTF16,0,0,mages_readstring<4>,0,"0100E94014792000",0}},//choice

        //Yahari Game demo Ore no Seishun Love Come wa Machigatteiru.
        {0x8005DFB8,{CODEC_UTF16,0,0,mages_readstring<0>,0,"0100E0D0154BC000","1.0.0"}},
        //CHAOS;HEAD NOAH
        {0x80046700,{CODEC_UTF16,0,0,mages_readstring<0>,0,"0100957016B90000","1.0.0"}},
        {0x8003A2c0,{CODEC_UTF16,0,0,mages_readstring<0>,0,"0100957016B90000","1.0.0"}},// choice
        {0x8003EAB0,{CODEC_UTF16,0,0,mages_readstring<0>,0,"0100957016B90000","1.0.0"}},// TIPS list (menu)
        {0x8004C648,{CODEC_UTF16,0,0,mages_readstring<0>,0,"0100957016B90000","1.0.0"}},// system message
        {0x80050374,{CODEC_UTF16,0,0,mages_readstring<0>,0,"0100957016B90000","1.0.0"}},// TIPS (red)
        // Shiro to Kuro no Alice
        {0x80013f20,{CODEC_UTF8,0,0,0,NewLineCharFilterW,"0100A460141B8000","1.0.0"}},
        {0x80013f94,{CODEC_UTF8,0,0,0,NewLineCharFilterW,"0100A460141B8000","1.0.0"}},
        {0x8001419c,{CODEC_UTF8,0,0,0,NewLineCharFilterW,"0100A460141B8000","1.0.0"}},
        // Shiro to Kuro no Alice -Twilight line-
        {0x80014260,{CODEC_UTF8,0,0,0,NewLineCharFilterW,"0100A460141B8000","1.0.0"}},
        {0x800142d4,{CODEC_UTF8,0,0,0,NewLineCharFilterW,"0100A460141B8000","1.0.0"}},
        {0x800144dc,{CODEC_UTF8,0,0,0,NewLineCharFilterW,"0100A460141B8000","1.0.0"}},
        //CLANNAD
        {0x80072d00,{CODEC_UTF16|FULL_STRING,1,0,0, F0100A3A00CC7E000,"0100A3A00CC7E000","1.0.0"}},
        {0x80072d30,{CODEC_UTF16|FULL_STRING,1,0,0,F0100A3A00CC7E000,"0100A3A00CC7E000","1.0.7"}},
        //VARIABLE BARRICADE NS
        {0x800e3424,{CODEC_UTF8,0,0,0,F010045C0109F2000,"010045C0109F2000","1.0.1"}},//"System Messages + Choices"), //Also includes the names of characters, 
        {0x800fb080,{CODEC_UTF8,3,0,0,F010045C0109F2000,"010045C0109F2000","1.0.1"}},//Main Text
        //AMNESIA for Nintendo Switch
        {0x805bba5c,{CODEC_UTF16,0,0,ReadTextAndLenDW<2>,F0100A1E00BFEA000,"0100A1E00BFEA000","1.0.1"}},//dialogue
        {0x805e9930,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100A1E00BFEA000,"0100A1E00BFEA000","1.0.1"}},//choice
        {0x805e7fd8,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100A1E00BFEA000,"0100A1E00BFEA000","1.0.1"}},//name

        //Chou no Doku Hana no Kusari Taishou Tsuya Koi Ibun
        {0x80095010,{CODEC_UTF16,1,0,0,F0100A1200CA3C000,"0100A1200CA3C000","2.0.1"}},//Main Text + Names
        //Live a Live
        {0x80a05170,{CODEC_UTF16,0,0,0,F0100982015606000,"0100C29017106000","1.0.0"}},
        //Sakura no Kumo * Scarlet no Koi
        {0x8049d968,{CODEC_UTF8,0,1,0,F01006590155AC000,"01006590155AC000","1.0.0"}},//name
        {0x8049d980,{CODEC_UTF8,0,0,0,F01006590155AC000,"01006590155AC000","1.0.0"}},//dialogue
        //Majestic Majolical
        {0x80557408,{CODEC_UTF8,0,0,0,F01000200194AE000,"01000200194AE000","1.0.0"}},//name
        {0x8059ee94,{CODEC_UTF8,3,0,0,F01000200194AE000,"01000200194AE000","1.0.0"}},//player name
        {0x80557420,{CODEC_UTF8,0,0,0,F01000200194AE000,"01000200194AE000","1.0.0"}},//dialogue

        //Matsurika no Kei
        {0x8017ad54,{CODEC_UTF32,1,0,0,F0100EA001A626000,"0100EA001A626000","1.0.0"}},// text
        {0x80174d4c,{CODEC_UTF32,1,0,0,F0100EA001A626000,"0100EA001A626000","1.0.0"}},// name
        //Cupid Parasite
        {0x80057910,{CODEC_UTF32,2,0,0,F0100F7E00DFC8000,"0100F7E00DFC8000","1.0.1"}},// name + text
        {0x80169df0,{CODEC_UTF32,0,0,0,F0100F7E00DFC8000,"0100F7E00DFC8000","1.0.1"}},// choice
        //Radiant Tale
        {0x80075190,{CODEC_UTF8,1,0,0,F0100925014864000,"0100925014864000","1.0.0"}},// prompt
        {0x8002fb18,{CODEC_UTF8,0,0,0,F0100925014864000,"0100925014864000","1.0.0"}},// name
        {0x8002fd7c,{CODEC_UTF8,0,0,0,F0100925014864000,"0100925014864000","1.0.0"}},// text
        //MUSICUS
        {0x80462DD4,{CODEC_UTF8,0,1,0,F01006590155AC000,"01000130150FA000","1.0.0"}},// name
        {0x80462DEC,{CODEC_UTF8,0,0,0,F01006590155AC000,"01000130150FA000","1.0.0"}},// dialogue 1 
        {0x80480d4c,{CODEC_UTF8,0,0,0,F01006590155AC000,"01000130150FA000","1.0.0"}},// dialogue 2
        {0x804798e0,{CODEC_UTF8,0,0,0,F01006590155AC000,"01000130150FA000","1.0.0"}},// choice

        
        //Story of Seasons a Wonderful Life
        {0x80ac4d88,{CODEC_UTF32,0,0,F0100936018EB4000,"0100936018EB4000","1.0.3"}},// Main text
        {0x808f7e84,{CODEC_UTF32,0,0,F0100936018EB4000,"0100936018EB4000","1.0.3"}},// Item name
        {0x80bdf804,{CODEC_UTF32,0,0,F0100936018EB4000,"0100936018EB4000","1.0.3"}},// Item description
        //Hamefura Pirates
        {0x81e75940,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100982015606000,"0100982015606000","1.0.0"}},// Hamekai.TalkPresenter$$AddMessageBacklog
        {0x81c9ae60,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100982015606000,"0100982015606000","1.0.0"}},// Hamekai.ChoicesText$$SetText
        {0x81eb7dc0,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100982015606000,"0100982015606000","1.0.0"}},// Hamekai.ShortStoryTextView$$AddText
        //Death end re;Quest 2
        {0x80225C3C,{CODEC_UTF8,8,0,0,F010001D015260000,"010001D015260000","1.0.0"}},
        //Death end re;Quest
        {0x80241088,{CODEC_UTF8,8,0,0,F0100AEC013DDA000,"0100AEC013DDA000","1.0.0"}},//english ver
        //Meta Meet Cute!!!+
        {0x81DD6010,{CODEC_UTF16,1,-32,0,0,"01009A401C1B0000","1.02"}},//english ver, only long string, short string can't find.
        //Of the Red, the Light, and the Ayakashi Tsuzuri
        {0x8176D78C,{CODEC_UTF16,3,0,0,0,"0100F7801B5DC000","1.0.0"}},
        //Tokimeki Memorial Girl's Side: 4th Heart
        {0x817e7da8,{CODEC_UTF16,0,0,T0100B0100E26C000<2,0>,0,"0100B0100E26C000","1.0.0"}},// name (x1) + dialogue (x2)
        {0x81429f54,{CODEC_UTF16,0,0,T0100B0100E26C000<0,1>,0,"0100B0100E26C000","1.0.0"}},// choice (x0)
        {0x8180633c,{CODEC_UTF16,0,0,T0100B0100E26C000<1,2>,0,"0100B0100E26C000","1.0.0"}},// help (x1)
        //13 Sentinels: Aegis Rim
        {0x80057d18,{CODEC_UTF8,0,0,0,F010045C014650000,"010045C014650000","1.0.0"}},// cutscene text
        {0x8026fec0,{CODEC_UTF8,1,0,0,F010045C014650000,"010045C014650000","1.0.0"}},// prompt
        {0x8014eab4,{CODEC_UTF8,0,0,0,F010045C014650000,"010045C014650000","1.0.0"}},// name (combat)
        {0x801528ec,{CODEC_UTF8,3,0,0,F010045C014650000,"010045C014650000","1.0.0"}},// dialogue (combat)
        {0x80055acc,{CODEC_UTF8,0,0,0,F010045C014650000,"010045C014650000","1.0.0"}},// dialogue 2 (speech bubble)
        {0x802679c8,{CODEC_UTF8,1,0,0,F010045C014650000,"010045C014650000","1.0.0"}},// notification
        {0x8025e210,{CODEC_UTF8,2,0,0,F010045C014650000,"010045C014650000","1.0.0"}},// scene context example: 数日前 咲良高校 １年Ｂ組 教室 １９８５年５月"
        {0x8005c518,{CODEC_UTF8,0,0,0,F010045C014650000,"010045C014650000","1.0.0"}},// game help
        //Sea of Stars
        {0x83e93ca0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01008C0016544000,"01008C0016544000","1.0.45861"}},// Main text
        {0x820c3fa0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01008C0016544000,"01008C0016544000","1.0.47140"}},// Main text
        //Final Fantasy I
        {0x81e88040,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01000EA014150000","1.0.1"}},// Main text
        {0x81cae54c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01000EA014150000","1.0.1"}},// Intro text
        {0x81a3e494,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01000EA014150000","1.0.1"}},// battle text
        {0x81952c28,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01000EA014150000","1.0.1"}},// Location
        //Final Fantasy II
        {0x8208f4cc,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01006B7014156000","1.0.1"}},// Main text
        {0x817e464c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01006B7014156000","1.0.1"}},// Intro text
        {0x81fb6414,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01006B7014156000","1.0.1"}},// battle text
        //Final Fantasy III
        {0x82019e84,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01002E2014158000","1.0.1"}},// Main text1
        {0x817ffcfc,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01002E2014158000","1.0.1"}},// Main text2
        {0x81b8b7e4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01002E2014158000","1.0.1"}},// battle text
        {0x8192c4a8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01002E2014158000","1.0.1"}},// Location
        //Final Fantasy IV
        {0x81e44bf4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01004B301415A000","1.0.2"}},// Main text
        {0x819f92c4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01004B301415A000","1.0.2"}},// Rolling text
        {0x81e2e798,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01004B301415A000","1.0.2"}},// Battle text
        {0x81b1e6a8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01004B301415A000","1.0.2"}},// Location
        //Final Fantasy V
        {0x81d63e24,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"0100AA201415C000","1.0.2"}},// Main text
        {0x81adfb3c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"0100AA201415C000","1.0.2"}},// Location
        {0x81a8fda8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"0100AA201415C000","1.0.2"}},// Battle text
        //Final Fantasy VI
        {0x81e6b350,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"0100AA001415E000","1.0.2"}},// Main text
        {0x81ab40ec,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"0100AA001415E000","1.0.2"}},// Location
        {0x819b8c88,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"0100AA001415E000","1.0.2"}},// Battle text
        //Final Fantasy IX
        {0x80034b90,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Main Text
        {0x802ade64,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Battle Text
        {0x801b1b84,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Descriptions
        {0x805aa0b0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Key Item Name
        {0x805a75d8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Key Item Content
        {0x8002f79c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Menu
        {0x80ca88b0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Tutorial1
        {0x80ca892c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Tutorial2
        {0x80008d88,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Location
        //Norn9 Var Commons
        {0x8003E874,{CODEC_UTF8,0,0,0,F0100068019996000,"0100068019996000","1.0.0"}},//English
        //薄桜鬼 真改 万葉ノ抄
        {0x8004E8F0,{CODEC_UTF8,1,0,0,F010001D015260000,"0100EA601A0A0000","1.0.0"}},
        //Hakuouki Shinkai: Tsukikage no Shou / 薄桜鬼 真改 月影ノ抄
        {0x8019ecd0,{CODEC_UTF8,1,0,0,F0100E1E00E2AE000,"0100E1E00E2AE000","1.0.0"}},//Text
        //Chrono Cross: The Radical Dreamers Edition
        {0x802b1254,{CODEC_UTF32,1,0,0,0,"0100AC20128AC000","1.0.2"}},//Text
        //AIR
        {0x800a6b10,{CODEC_UTF16,1,0,0,F0100ADC014DA0000,"0100ADC014DA0000","1.0.1"}},//Text + Name
        //Shinigami to Shoujo
        {0x21cb08+0x80004000,{0,1,0,0,F0100AFA01750C000,"0100AFA01750C000","1.0.2"}},//Text,sjis
        //Octopath Traveler II
        {0x8088a4d4,{CODEC_UTF16,0,0,0,0,"0100A3501946E000","1.0.0"}},//main text
        //NieR:Automata The End of YoRHa Edition
        {0x808e7068,{CODEC_UTF16,3,0,0,0,"0100B8E016F76000","1.0.2"}},//Text
        //Reine des Fleurs
        {0x80026434,{CODEC_UTF8,0,0,0,0,"0100B5800C0E4000","1.0.0"}},//Dialogue text 
        //Code : Realize - Saikou no Hanataba 
        {0x80024eac,{CODEC_UTF8,0,0,0,F0100B6900A668000,"0100B6900A668000","1.0.0"}},
        //Diabolik Lovers Grand Edition
        {0x80041080,{CODEC_UTF8,1,0,0,F0100BD700E648000,"0100BD700E648000","1.0.0"}},//name
        {0x80041080,{CODEC_UTF8,0,0,0,F0100BD700E648000,"0100BD700E648000","1.0.0"}},//dialogue
        {0x80041080,{CODEC_UTF8,2,0,0,F0100BD700E648000,"0100BD700E648000","1.0.0"}},//choice1
        //Shinobi, Koi Utsutsu
        {0x8002aca0,{CODEC_UTF8,0,0,0,F0100B6900A668000,"0100C1E0102B8000","1.0.0"}},//name
        {0x8002aea4,{CODEC_UTF8,0,0,0,F0100B6900A668000,"0100C1E0102B8000","1.0.0"}},//dialogue1
        {0x8001ca90,{CODEC_UTF8,2,0,0,F0100B6900A668000,"0100C1E0102B8000","1.0.0"}},//dialogue2
        {0x80049dbc,{CODEC_UTF8,1,0,0,F0100B6900A668000,"0100C1E0102B8000","1.0.0"}},//choice
        //Yoru, Tomosu
        {0xe2748eb0,{CODEC_UTF32,1,0,0,0,"0100C2901153C000","1.0.0"}},// text1
        //Closed Nightmare
        {0x800c0918,{CODEC_UTF8,0,0,0,F0100D9500A0F6000,"0100D9500A0F6000","1.0.0"}},// line + name
        {0x80070b98,{CODEC_UTF8,0,0,0,F0100D9500A0F6000,"0100D9500A0F6000","1.0.0"}},// fast trophy
        {0x800878fc,{CODEC_UTF8,0,0,0,F0100D9500A0F6000,"0100D9500A0F6000","1.0.0"}},// prompt
        {0x80087aa0,{CODEC_UTF8,0,0,0,F0100D9500A0F6000,"0100D9500A0F6000","1.0.0"}},// choice
        //Yuru Camp△ - Have a Nice Day!
        {0x816d03f8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100982015606000,"0100D12014FC2000","1.0.0"}},// dialog / backlog
        //Akuyaku Reijou wa Ringoku no Outaishi ni Dekiai Sareru
        {0x817b35c4,{CODEC_UTF8,1,0,0,F0100DA201E0DA000,"0100DA201E0DA000","1.0.0"}},// Dialogue
        //Yunohana Spring! ~Mellow Times~
        {0x80028178,{CODEC_UTF8,0,0,0,F0100DE200C0DA000,"0100DE200C0DA000","1.0.0"}},// name
        {0x8001b9d8,{CODEC_UTF8,2,0,0,F0100DE200C0DA000,"0100DE200C0DA000","1.0.0"}},// dialogue1
        {0x8001b9b0,{CODEC_UTF8,2,0,0,F0100DE200C0DA000,"0100DE200C0DA000","1.0.0"}},// dialogue2
        {0x8004b940,{CODEC_UTF8,2,0,0,F0100DE200C0DA000,"0100DE200C0DA000","1.0.0"}},// dialogue3
        {0x8004a8d0,{CODEC_UTF8,1,0,0,F0100DE200C0DA000,"0100DE200C0DA000","1.0.0"}},// choice
        //サマータイムレンダ Another Horizon
        {0x818ebaf0,{CODEC_UTF16,0,0,T01005940182EC000,F01005940182EC000,"01005940182EC000","1.0.0"}},//dialogue
        //Aquarium
        {0x8051a990,{CODEC_UTF8,0,1,0,F01006590155AC000,"0100D11018A7E000","1.0.0"}},//name
        {0x8051a9a8,{CODEC_UTF8,0,0,0,F01006590155AC000,"0100D11018A7E000","1.0.0"}},//dialogue
        {0x80500178,{CODEC_UTF8,0,0,0,F01006590155AC000,"0100D11018A7E000","1.0.0"}},//choice
        //AKA
        {0x8166eb80,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0601852A000<0>,"0100B0601852A000","1.0.0"}},//Main text
        {0x817d44a4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0601852A000<1>,"0100B0601852A000","1.0.0"}},//Letter
        {0x815cb0f4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0601852A000<2>,"0100B0601852A000","1.0.0"}},//Mission title
        {0x815cde30,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0601852A000<3>,"0100B0601852A000","1.0.0"}},//Mission description
        {0x8162a910,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0601852A000<4>,"0100B0601852A000","1.0.0"}},//Craft description
        {0x817fdca8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0601852A000<5>,"0100B0601852A000","1.0.0"}},//Inventory item name
        //Etrian Odyssey I HD
        {0x82d57550,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0C016164000<8>,"01008A3016162000","1.0.2"}},//Text
        {0x824ff408,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0C016164000<9>,"01008A3016162000","1.0.2"}},//Config Description
        {0x8296b4e4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0C016164000<10>,"01008A3016162000","1.0.2"}},//Class Description
        {0x81b2204c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0C016164000<11>,"01008A3016162000","1.0.2"}},//Item Description
        //Etrian Odyssey II HD
        {0x82f24c70,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0C016164000<0>,"0100B0C016164000","1.0.2"}},//Text
        {0x82cc0988,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0C016164000<1>,"0100B0C016164000","1.0.2"}},//Config Description
        {0x8249acd4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0C016164000<2>,"0100B0C016164000","1.0.2"}},//Class Description
        {0x81b27644,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0C016164000<3>,"0100B0C016164000","1.0.2"}},//Item Description
        //Etrian Odyssey III HD
        {0x83787f04,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0C016164000<4>,"0100D32015A52000","1.0.2"}},//Text
        {0x8206915c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0C016164000<5>,"0100D32015A52000","1.0.2"}},//Config Description
        {0x82e6d1d4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0C016164000<6>,"0100D32015A52000","1.0.2"}},//Class Description
        {0x82bf5d48,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100B0C016164000<7>,"0100D32015A52000","1.0.2"}},//Item Description
        //Fire Emblem Engage
        {0x8248c550,{CODEC_UTF16,0,0,ReadTextAndLenDW<2>,0,"0100A6301214E000","1.3.0"}},// App.Talk3D.TalkLog$$AddLog
        {0x820C6530,{CODEC_UTF16,0,0,ReadTextAndLenDW<2>,0,"0100A6301214E000","2.0.0"}},// App.Talk3D.TalkLog$$AddLog
        //AMNESIA LATER×CROWD for Nintendo Switch 
        {0x800ebc34,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100982015606000,"0100B5700CDFC000","1.0.0"}},// waterfall
        {0x8014dc64,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100982015606000,"0100B5700CDFC000","1.0.0"}},// name
        {0x80149b10,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100982015606000,"0100B5700CDFC000","1.0.0"}},// dialogue
        {0x803add50,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100982015606000,"0100B5700CDFC000","1.0.0"}},// choice
        //Hanayaka Nari, Waga Ichizoku Gentou Nostalgie
        {0x27ca10+0x80004000,{CODEC_UTF8,0,0,T0100B5500CA0C000,F0100B5500CA0C000,"0100B5500CA0C000","1.0.0"}},// x3 (double trigged), name+text, onscreen 
        //Natsumon! 20th Century Summer Vacation
        {0x80db5d34,{CODEC_UTF16,0,0,0,F0100A8401A0A8000,"0100A8401A0A8000","1.1.0"}},// tutorial
        {0x846fa578,{CODEC_UTF16,0,0,0,F0100A8401A0A8000,"0100A8401A0A8000","1.1.0"}},// choice
        {0x8441e800,{CODEC_UTF16,0,0,0,F0100A8401A0A8000,"0100A8401A0A8000","1.1.0"}},// examine + dialog
        //Super Mario RPG
        {0x81d78c58,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Main Text
        {0x81dc9cf8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Name
        {0x81c16b80,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Cutscene
        {0x821281f0,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Special/Item/Menu/Objective Description
        {0x81cd8148,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Special Name
        {0x81fc2820,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Item Name Battle
        {0x81d08d28,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Item Name Off-battle
        {0x82151aac,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Shop Item Name
        {0x81fcc870,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Objective Title
        {0x821bd328,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Monster List - Name
        {0x820919b8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Monster List - Description
        {0x81f56518,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Info
        {0x82134ce0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Help Category
        {0x82134f30,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Help Name
        {0x821372e4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Help Description 1
        {0x82137344,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Help Description 2
        {0x81d0ee80,{CODEC_UTF16,0,0,ReadTextAndLenDW<2>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Location
        {0x82128f64,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Album Title
        {0x81f572a0,{CODEC_UTF16,0,0,ReadTextAndLenDW<3>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Load/Save Text
        {0x81d040a8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Levelup First Part
        {0x81d043fc,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Levelup Second Part
        {0x81d04550,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Levelup New Ability Description
        {0x81fbfa18,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Yoshi Mini-Game Header
        {0x81fbfa74,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Yoshi Mini-Game Text
        {0x81cf41b4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BC0018138000,"0100BC0018138000","1.0.0"}},// Enemy Special Attacks
        //Trials of Mana
        {0x800e8abc,{CODEC_UTF16,1,0,0,F0100D7800E9E0000,"0100D7800E9E0000","1.1.1"}},// Text
        //Utsusemi no Meguri
        {0x821b452c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"0100DA101D9AA000","1.0.0"}},// text1
        {0x821b456c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"0100DA101D9AA000","1.0.0"}},// text2
        {0x821b45ac,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"0100DA101D9AA000","1.0.0"}},// text3
        //Buddy Mission BOND
        {0x80046dd0,{0,0,0,T0100DB300B996000,0,"0100DB300B996000",0}},//1.0.0, 1.0.1,sjis
        {0x80046de0,{0,0,0,T0100DB300B996000,0,"0100DB300B996000",0}},
        //Bravely Default II
        {0x80b97700,{CODEC_UTF16,0,0,0,0,"010056F00C7B4000","1.0.0"}},//Main Text
        {0x80bb8d3c,{CODEC_UTF16,0,0,0,0,"010056F00C7B4000","1.0.0"}},//Main Ptc Text
        {0x810add68,{CODEC_UTF16,0,0,0,0,"010056F00C7B4000","1.0.0"}},//Secondary Text
        //Tantei Bokumetsu / 探偵撲滅
        {0x8011c340,{CODEC_UTF8,1,0,0,F0100CBA014014000,"0100CBA014014000","1.0.0"}},//Text
        {0x80064f20,{CODEC_UTF8,1,0,0,F0100CBA014014000,"0100CBA014014000","1.0.0"}},//Choices
        //Ys X: Nordics
        {0x80817758,{CODEC_UTF8,1,0,0,F0100CC401A16C000<0>,"0100CC401A16C000","1.0.4"}},//Main Text
        {0x80981e3c,{CODEC_UTF8,0,0,0,F0100CC401A16C000<1>,"0100CC401A16C000","1.0.4"}},//Secondary Text
        //9 R.I.P
        {0x80025360,{CODEC_UTF8,2,0,0,F0100BDD01AAE4000,"0100BDD01AAE4000","1.0.0"}},//name
        {0x80023c60,{CODEC_UTF8,0,0,0,F0100BDD01AAE4000,"0100BDD01AAE4000","1.0.0"}},//text
        {0x8005388c,{CODEC_UTF8,1,0,0,F0100BDD01AAE4000,"0100BDD01AAE4000","1.0.0"}},//choice
        {0x80065010,{CODEC_UTF8,0,0,0,F0100BDD01AAE4000,"0100BDD01AAE4000","1.0.0"}},//character description
        {0x8009c780,{CODEC_UTF8,0,0,0,F0100BDD01AAE4000,"0100BDD01AAE4000","1.0.0"}},//prompt
        //Kiss Bell - Let's sound the kissing-bell of the promise / キスベル
        {0x8049d958,{CODEC_UTF8,1,0,0,F01006590155AC000,"0100BD7015E6C000","1.0.0"}},//text
        //Piofiore no Banshou -Ricordo-  CN
        {0x80015fa0,{CODEC_UTF8,2,0,0,F0100C310110B4000,"0100C310110B4000","1.0.0"}},//handlerMsg
        {0x80050d50,{CODEC_UTF8,0,0,0,F0100C310110B4000,"0100C310110B4000","1.0.0"}},//handlerName
        {0x8002F430,{CODEC_UTF8,0,0,0,F0100C310110B4000,"0100C310110B4000","1.0.0"}},//handlerPrompt
        {0x8002F4F0,{CODEC_UTF8,0,0,0,F0100C310110B4000,"0100C310110B4000","1.0.0"}},//handlerPrompt
        {0x8002F540,{CODEC_UTF8,0,0,0,F0100C310110B4000,"0100C310110B4000","1.0.0"}},//handlerPrompt
        //Piofiore no Banshou -Ricordo-
        {0x800141d0,{CODEC_UTF8,2,0,0,F0100C310110B4000,"01005F700DC56000","1.0.0"}},//handlerMsg
        {0x8004ce20,{CODEC_UTF8,0,0,0,F0100C310110B4000,"01005F700DC56000","1.0.0"}},//handlerName
        {0x8002be90,{CODEC_UTF8,0,0,0,F0100C310110B4000,"01005F700DC56000","1.0.0"}},//handlerPrompt
        {0x8002bf50,{CODEC_UTF8,0,0,0,F0100C310110B4000,"01005F700DC56000","1.0.0"}},//handlerPrompt
        {0x8002bfa0,{CODEC_UTF8,0,0,0,F0100C310110B4000,"01005F700DC56000","1.0.0"}},//handlerPrompt
        //Piofiore no Banshou -Episodio1926-
        {0x80019630,{CODEC_UTF8,2,0,0,F0100C310110B4000,"01009E30120F4000","1.0.0"}},//handlerMsg
        {0x8005B7B0,{CODEC_UTF8,0,0,0,F0100C310110B4000,"01009E30120F4000","1.0.0"}},//handlerName
        {0x80039230,{CODEC_UTF8,0,0,0,F0100C310110B4000,"01009E30120F4000","1.0.0"}},//handlerPrompt
        {0x800392F0,{CODEC_UTF8,0,0,0,F0100C310110B4000,"01009E30120F4000","1.0.0"}},//handlerPrompt
        {0x80039340,{CODEC_UTF8,0,0,0,F0100C310110B4000,"01009E30120F4000","1.0.0"}},//handlerPrompt
        //Pokémon Let’s Go, Pikachu!
        {0x8067d9fc,{CODEC_UTF16,0,0,0,F010003F003A34000,"010003F003A34000","1.0.2"}},//Text
        //Ikemen Sengoku Toki o Kakeru Koi / イケメン戦国◆時をかける恋　新たなる出逢い
        {0x813e4fb4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01008BE016CE2000","1.0.0"}},//Main Text
        {0x813e4c60,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01008BE016CE2000","1.0.0"}},//Name
        {0x813b5360,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"01008BE016CE2000","1.0.0"}},//Choices
        {0x81bab9ac,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,0,"01008BE016CE2000","1.0.0"}},//Info
        //Shin Megami Tensei V
        {0x80ce01a4,{CODEC_UTF16,0,0,0,0,"01006BD0095F4000","1.0.2"}},//Text
        //The Legend of Zelda: Link's Awakening
        {0x80f57910,{CODEC_UTF8,1,0,0,0,"01006BB00C6F0000","1.0.1"}},//Main Text
        //Cendrillon palikA
        {0x8001ab8c,{CODEC_UTF8,2,0,0,F0100DE200C0DA000,"01006B000A666000","1.0.0"}},//name
        {0x80027b30,{CODEC_UTF8,0,0,0,F0100DE200C0DA000,"01006B000A666000","1.0.0"}},//dialogue
        //Crayon Shin-chan Shiro of Coal Town
        {0x83fab4bc,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F01007B601C608000,"01007B601C608000","1.0.1"}},
        //Fuuraiki 4
        {0x80008c80,{CODEC_UTF32,1,0,0,F010046601125A000,"010046601125A000","1.0.0"}},//Main
        {0x80012b1c,{CODEC_UTF32,1,0,0,F010046601125A000,"010046601125A000","1.0.0"}},//Wordpad
        {0x80012ccc,{CODEC_UTF32,1,0,0,F010046601125A000,"010046601125A000","1.0.0"}},//Comments
        {0x80009f74,{CODEC_UTF32,1,0,0,F010046601125A000,"010046601125A000","1.0.0"}},//Choices
        {0x80023d64,{CODEC_UTF32,0,0,0,F010046601125A000,"010046601125A000","1.0.0"}},//Location
        //Ken ga Kimi for S / 剣が君 for S
        {0x81477128,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100771013FA8000,"0100771013FA8000","1.1"}},//Main Text
        {0x81470e38,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100771013FA8000,"0100771013FA8000","1.1"}},//Secondary Text
        //ANONYMOUS;CODE
        {0x80011608,{CODEC_UTF8,1,0,0,F0100556015CCC000,"0100556015CCC000","1.0.0"}},//dialouge, menu
        //Sugar * Style (シュガー＊スタイル)
        {0x800ccbc8,{0,0,0,0,0,"0100325012B70000","1.0.0"}},// ret x0 name + text (readShiftJisString), filter is to complex, quit.
        //Nightshade／百花百狼
        {0x802999c8,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F010042300C4F6000,"010042300C4F6000","1.0.1"}},//dialogue
        {0x8015b544,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010042300C4F6000,"010042300C4F6000","1.0.1"}},//name
        {0x802a2fd4,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F010042300C4F6000,"010042300C4F6000","1.0.1"}},//choice1
        {0x802b7900,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F010042300C4F6000,"010042300C4F6000","1.0.1"}},//choice2
        //Toraware no Paruma
        {0x8015b7a8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010044800D2EC000,"010044800D2EC000","1.0.0"}},//text x0
        {0x8015b46c,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F010044800D2EC000,"010044800D2EC000","1.0.0"}},//name x1
        //Brothers Conflict: Precious Baby
        {0x8016aecc,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F0100982015606000,"010037400DAAE000","1.0.0"}},//name
        {0x80126b9c,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F0100982015606000,"010037400DAAE000","1.0.0"}},//dialogue
        {0x80129160,{CODEC_UTF16,0,0,ReadTextAndLenW<2>,F0100982015606000,"010037400DAAE000","1.0.0"}},//choice
        //Zettai Kaikyu Gakuen
        {0x80067b5c,{CODEC_UTF16,1,0,0,F010021300F69E000<0>,"010021300F69E000","1.0.0"}},//name+ dialogue main(ADV)+choices
        {0x80067cd4,{CODEC_UTF16,1,0,0,F010021300F69E000<1>,"010021300F69E000","1.0.0"}},//dialogueNVL
        //Dragon Quest Builders 2
        {0x805f8900,{CODEC_UTF8,1,0,0,F010050000705E000,"010050000705E000","1.7.3"}},//Main text textbox
        {0x8068a698,{CODEC_UTF8,0,0,0,F010050000705E000,"010050000705E000","1.7.3"}},//Not press to continue text
        {0x806e4118,{CODEC_UTF8,3,0,0,F010050000705E000,"010050000705E000","1.7.3"}},//Character creation text
        {0x8067459c,{CODEC_UTF8,1,0,0,F010050000705E000,"010050000705E000","1.7.3"}},//Objective progress1
        {0x800a4f90,{CODEC_UTF8,0,0,0,F010050000705E000,"010050000705E000","1.7.3"}},//Objective progress2
        {0x8060a1c0,{CODEC_UTF8,0,0,0,F010050000705E000,"010050000705E000","1.7.3"}},//Infos1
        {0x805f6130,{CODEC_UTF8,1,0,0,F010050000705E000,"010050000705E000","1.7.3"}},//Infos2
        {0x80639b6c,{CODEC_UTF8,2,0,0,F010050000705E000,"010050000705E000","1.7.3"}},//Item description
        {0x807185ac,{CODEC_UTF8,1,0,0,F010050000705E000,"010050000705E000","1.7.3"}},//Mission1
        {0x80657e4c,{CODEC_UTF8,1,0,0,F010050000705E000,"010050000705E000","1.7.3"}},//Mission2
        {0x80713be0,{CODEC_UTF8,1,0,0,F010050000705E000,"010050000705E000","1.7.3"}},//Mission3
        {0x8076ab04,{CODEC_UTF8,1,0,0,F010050000705E000,"010050000705E000","1.7.3"}},//Tutorial header
        {0x8076ab2c,{CODEC_UTF8,1,0,0,F010050000705E000,"010050000705E000","1.7.3"}},//Tutorial explanation
        //BUSTAFELLOWS season2
        {0x819ed3e4,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F0100874017BE2000,"010037400DAAE000","1.0.0"}},//dialogue
        {0x82159cd0,{CODEC_UTF16,0,0,ReadTextAndLenW<1>,F0100874017BE2000,"010037400DAAE000","1.0.0"}},//textmessage
        {0x81e17530,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F0100874017BE2000,"010037400DAAE000","1.0.0"}},//option
        {0x81e99d64,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F0100874017BE2000,"010037400DAAE000","1.0.0"}},//choice
        {0x8186f81c,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F0100874017BE2000,"010037400DAAE000","1.0.0"}},//archives
        //5分後に意外な結末　モノクロームの図書館
        {0x81fa4890,{CODEC_UTF16,1,0X14,0,F010094601D910000,"010094601D910000","1.0.1"}},//book text
        {0x81fa5250,{CODEC_UTF16,1,0X14,0,F010094601D910000,"010094601D910000","1.0.1"}},//book text
        {0x81b1c68c,{CODEC_UTF16,0,0X14,0,F010094601D910000,"010094601D910000","1.0.1"}},//choice1
        {0x81b1c664,{CODEC_UTF16,0,0X14,0,F010094601D910000,"010094601D910000","1.0.1"}},//choice2
        {0x81b1e5b0,{CODEC_UTF16,3,0X14,0,F010094601D910000,"010094601D910000","1.0.1"}},//dialogue
        //Tokimeki Memorial Girl’s Side 2nd Season for Nintendo Switch
        {0x82058848,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010079201BD88000,"010079201BD88000","1.0.1"}},//dialogue1
        {0x82058aa0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010079201BD88000,"010079201BD88000","1.0.1"}},//dialogue2
        {0x8205a244,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010079201BD88000,"010079201BD88000","1.0.1"}},//dialogue3
        {0x826ee1d8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010079201BD88000,"010079201BD88000","1.0.1"}},//choice
        {0x8218e258,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010079201BD88000,"010079201BD88000","1.0.1"}},//news
        {0x823b61d4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010079201BD88000,"010079201BD88000","1.0.1"}},//mail
        {0x82253454,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010079201BD88000,"010079201BD88000","1.0.1"}},//luckyitem
        {0x82269240,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010079201BD88000,"010079201BD88000","1.0.1"}},//profile1
        {0x82269138,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010079201BD88000,"010079201BD88000","1.0.1"}},//profile2
        {0x822691ec,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010079201BD88000,"010079201BD88000","1.0.1"}},//profile3
        {0x82269198,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010079201BD88000,"010079201BD88000","1.0.1"}},//profile4
        //Uta no☆Prince-sama♪ Repeat Love / うたの☆プリンスさまっ♪Repeat LOVE
        {0x800374a0,{0,0,0,0,F0100068019996000,"010024200E00A000","1.0.0"}},//Main Text + Name,sjis
        {0x8002ea08,{0,0,0,0,F0100068019996000,"010024200E00A000","1.0.0"}},//Choices,sjis
        //ワンド オブ フォーチュン Ｒ２ ～時空に沈む黙示録～ for Nintendo Switch
        {0x821540c4,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F0100DA201E0DA000,"010088A01A774000","1.0.0"}},//dialogue
        {0x8353e674,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F0100DA201E0DA000,"010088A01A774000","1.0.0"}},//choice
        {0x835015e8,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F0100DA201E0DA000,"010088A01A774000","1.0.0"}},//name
        //Yo-kai Watch 4++
        {0x80a88080,{CODEC_UTF8,1,0,0,F010086C00AF7C000,"010086C00AF7C000","2.2.0"}},//All Text
        //Cupid Parasite -Sweet & Spicy Darling-
        {0x80138150,{CODEC_UTF32,2,0,0,F010079C017B98000,"010079C017B98000","1.0.0"}},//name + text
        {0x801a1bf0,{CODEC_UTF32,0,0,0,F010079C017B98000,"010079C017B98000","1.0.0"}},//choice
        //DesperaDrops
        {0x8199c95c,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F010061A01C1CE000,"010061A01C1CE000","1.0.0"}},//text1
        {0x81d5c900,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F010061A01C1CE000,"010061A01C1CE000","1.0.0"}},//text2
        {0x820d6324,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F010061A01C1CE000,"010061A01C1CE000","1.0.0"}},//choice
        //Dragon Ball Z: Kakarot
        {0x812a8e28,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Main Text
        {0x812a8c90,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Name
        {0x80bfbff0,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Ptc Text
        {0x80bfbfd4,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Ptc Name
        {0x8126a538,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Info
        {0x8106fcbc,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//More Info
        {0x80fad204,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Hint Part1
        {0x80fad2d0,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Hint Part2
        {0x80facf1c,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Loading Title
        {0x80fad018,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Loading Description
        {0x81250c50,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Tutorial h1
        {0x81250df0,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Tutorial h2
        {0x81251e80,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Tutorial Description1
        {0x81252214,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Tutorial Description2
        {0x810ae1c4,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Config Description
        {0x812a9bb8,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Menu Talk
        {0x812a9b78,{CODEC_UTF16,0,0,0,F01008C0016544000,"0100EF00134F4000","1.50"}},//Menu Name
        //Harvestella
        {0x80af7abc,{CODEC_UTF16,0,0,0,F0100B0601852A000<6>,"0100EDD018032000","1.0.1"}},//Main Text
        {0x80c0beb8,{CODEC_UTF16,0,0,0,F0100B0601852A000<7>,"0100EDD018032000","1.0.1"}},//Tutorial + News
        {0x80b87f94,{CODEC_UTF16,0,0,0,F0100B0601852A000<8>,"0100EDD018032000","1.0.1"}},//Tutorial Part 2
        {0x80e1c378,{CODEC_UTF16,0,0,0,F0100B0601852A000<9>,"0100EDD018032000","1.0.1"}},//Mission Title
        {0x80a7d7f4,{CODEC_UTF16,0,0,0,F0100B0601852A000<10>,"0100EDD018032000","1.0.1"}},//Mission Description
        {0x80e39130,{CODEC_UTF16,0,0,0,F0100B0601852A000<11>,"0100EDD018032000","1.0.1"}},//Item Name
        {0x80e38f80,{CODEC_UTF16,0,0,0,F0100B0601852A000<12>,"0100EDD018032000","1.0.1"}},//Item Description Part1
        {0x80e38ea8,{CODEC_UTF16,0,0,0,F0100B0601852A000<13>,"0100EDD018032000","1.0.1"}},//Item Description Part2
        //Sen no Hatou, Tsukisome no Kouki
        {0x8003fc90,{CODEC_UTF8,1,0,0,0,"0100F8A017BAA000","1.0.0"}},//text1
        {0x8017a740,{CODEC_UTF8,0,0,0,0,"0100F8A017BAA000","1.0.0"}},//text2
        //Olympia Soiree
        {0x8002ad04,{CODEC_UTF8,0,0,0,F0100C310110B4000,"0100F9D00C186000","1.0.0"}},
        //Getsuei no Kusari -Sakuran Paranoia-
        {0x21801c+0x80004000,{0,2,0,0,F0100F7401AA74000,"0100F7401AA74000","1.0.0"}},//text,sjis  
        {0x228fac+0x80004000,{0,1,0,0,F0100F7401AA74000,"0100F7401AA74000","1.0.0"}},//choices
        {0x267f24+0x80004000,{0,1,0,0,F0100F7401AA74000,"0100F7401AA74000","1.0.0"}},//dictionary
        //Xenoblade Chronicles 2
        {0x8010b180,{CODEC_UTF8,1,0,0,F01006F000B056000,"0100F3400332C000","2.0.2"}},//Text
        //Kanon
        {0x800dc524,{CODEC_UTF16,0,0,0,F0100FB7019ADE000,"0100FB7019ADE000","1.0.0"}},//Text
        //Princess Arthur
        {0x80066e10,{0,2,0,0,F0100FC2019346000,"0100FC2019346000","1.0.0"}},//Dialogue text ,sjis
        {0x8001f7d0,{0,0,0,0,F0100FC2019346000,"0100FC2019346000","1.0.0"}},//Name
        //Layton’s Mystery Journey: Katrielle and the Millionaires’ Conspiracy
        {0x8025d520,{0,2,0,0,F0100FDB00AA80000,"0100FDB00AA80000","1.1.0"}},//All Text ,sjis
        // Xenoblade Chronicles: Definitive Edition
        {0x808a5670,{CODEC_UTF8,1,0,0,F0100FF500E34A000,"0100FF500E34A000","1.1.2"}},//Main Text
        {0x80305968,{CODEC_UTF8,1,0,0,F0100FF500E34A000,"0100FF500E34A000","1.1.2"}},//Choices
        {0x8029edc8,{CODEC_UTF8,0,0,0,F0100FF500E34A000,"0100FF500E34A000","1.1.2"}},//Item Name
        {0x8029ede8,{CODEC_UTF8,0,0,0,F0100FF500E34A000,"0100FF500E34A000","1.1.2"}},//Item Description
        {0x8026a454,{CODEC_UTF8,0,0,0,F0100FF500E34A000,"0100FF500E34A000","1.1.2"}},//Acquired Item Name
        {0x803c725c,{CODEC_UTF8,0,0,0,F0100FF500E34A000,"0100FF500E34A000","1.1.2"}},//Acquired Item Notification
        {0x802794cc,{CODEC_UTF8,0,0,0,F0100FF500E34A000,"0100FF500E34A000","1.1.2"}},//Location Discovered
        //Unicorn Overlord
        {0x805ae1f8,{CODEC_UTF8,1,0,0,F01000AE01954A000,"01000AE01954A000","1.00"}},//Text
        //Octopath Traveler
        {0x8005ef78,{CODEC_UTF32,0,0,0,0,"01000E200DC58000","1.0.0"}},//Text
        //The World Ends with You: Final Remix
        {0x80706ab8,{CODEC_UTF16,2,0,0,F01006F000B056000,"01001C1009892000","1.0.0"}},//Text
        //JackJanne
        {0x81f02cd8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100982015606000,"01001DD010A2E800","1.0.5"}},//Text
        {0x821db028,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100982015606000,"01001DD010A2E800","1.0.5"}},//choice
        //Collar x Malice
        {0x800444c4,{CODEC_UTF8,0,0,0,0,"01002B400E9DA000","1.0.0"}},//Text
        //Kanda Alice mo Suiri Suru.
        {0x80041db0,{0,0,0,0,F01003BD013E30000,"01003BD013E30000","1.0.0"}},//sjis
        //Rune Factory 3 Special
        {0x81fb3364,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01001EF017BE6000,"01001EF017BE6000","1.0.4"}},//Main Text
        {0x826c0f20,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F01001EF017BE6000,"01001EF017BE6000","1.0.4"}},//Aproach
        {0x81fb3320,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01001EF017BE6000,"01001EF017BE6000","1.0.4"}},//Choices
        {0x821497e8,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F01001EF017BE6000,"01001EF017BE6000","1.0.4"}},//Calendar
        {0x826ba1a0,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F01001EF017BE6000,"01001EF017BE6000","1.0.4"}},//Info
        {0x823f6200,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01001EF017BE6000,"01001EF017BE6000","1.0.4"}},//More Info
        {0x826c381c,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F01001EF017BE6000,"01001EF017BE6000","1.0.4"}},//Item Select Name
        //Toraware no Paruma -Refrain-
        {0x80697300,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F01000EA00D2EE000,"01000EA00D2EE000","1.0.0"}},//text x1
        {0x806f43c0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01000EA00D2EE000,"01000EA00D2EE000","1.0.0"}},//name x0
        {0x80d2aca4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01000EA00D2EE000,"01000EA00D2EE000","1.0.0"}},//choice x0
        {0x804b04c8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01000EA00D2EE000,"01000EA00D2EE000","1.0.0"}},//alert x0
        {0x804b725c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01000EA00D2EE000,"01000EA00D2EE000","1.0.0"}},//prompt x0
        //Aiyoku no Eustia
        {0x804BEFD0,{CODEC_UTF8,0,0,0,F01006590155AC000,"01001CC017BB2000","1.0.0"}},//x0 - name
        {0x804BEFE8,{CODEC_UTF8,0,0,0,F01006590155AC000,"01001CC017BB2000","1.0.0"}},//x0 - dialogue
        {0x804d043c,{CODEC_UTF8,0,0,0,F01006590155AC000,"01001CC017BB2000","1.0.0"}},//x0 - choice
        //ワンド オブ フォーチュン Ｒ～ for Nintendo Switch
        {0x81ed0580,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100DA201E0DA000,"01000C7019E1C000","1.0.0"}},//dialogue
        {0x81f96bac,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100DA201E0DA000,"01000C7019E1C000","1.0.0"}},//name
        {0x8250ac28,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100DA201E0DA000,"01000C7019E1C000","1.0.0"}},//choice
        //Jakou no Lyla ~Trap of MUSK~
        {0x80167100,{CODEC_UTF32,1,0,0,F010093800DB1C000,"010093800DB1C000","1.0.0"}},// x1 text + name (unformated), #T1 #T2, #T0/* 1. European night */
        {0x801589a0,{CODEC_UTF32,1,0,0,F010093800DB1C000,"010093800DB1C000","1.0.0"}},// x0=x1=choice (sig=SltAdd)
        {0x801b4300,{CODEC_UTF32,1,0,0,F010093800DB1C000,"010093800DB1C000","1.0.0"}},// x1 text + name (unformated), #T1 #T2, #T0/* 2. Asian night */
        {0x802a9170,{CODEC_UTF32,1,0,0,F010093800DB1C000,"010093800DB1C000","1.0.0"}},// x0=x1=choice (sig=SltAdd)
        {0x80301e80,{CODEC_UTF32,1,0,0,F010093800DB1C000,"010093800DB1C000","1.0.0"}},// x1 text + name (unformated), #T1 #T2, #T0/* 3. Arabic night */
        {0x803f7a90,{CODEC_UTF32,1,0,0,F010093800DB1C000,"010093800DB1C000","1.0.0"}},// x0=x1=choice (sig=SltAdd)
        //Galleria no Chika Meikyuu to Majo no Ryodan ガレリアの地下迷宮と魔女ノ旅団
        {0x8002f64c,{CODEC_UTF8,0,0,0,0,"01007010157B4000","1.0.1"}},//Main Text
        //Dragon's Dogma: Dark Arisen
        {0x81023a80,{CODEC_UTF8,1,0,0,F010057E00AC56000<0>,"010057E00AC56000","1.0.1"}},//Main Text
        {0x8103e140,{CODEC_UTF8,1,0,0,F010057E00AC56000<1>,"010057E00AC56000","1.0.1"}},//Allies + Cutscene Text
        {0x8103bb10,{CODEC_UTF8,1,0,0,F010057E00AC56000<2>,"010057E00AC56000","1.0.1"}},//NPC Text
        {0x80150720,{CODEC_UTF8,0,0,0,F010057E00AC56000<3>,"010057E00AC56000","1.0.1"}},//Intro Message
        {0x80df90a8,{CODEC_UTF8,0,0,0,F010057E00AC56000<4>,"010057E00AC56000","1.0.1"}},//Info1
        {0x80ce2bb8,{CODEC_UTF8,0,0,0,F010057E00AC56000<5>,"010057E00AC56000","1.0.1"}},//Info2
        {0x80292d84,{CODEC_UTF8,0,0,0,F010057E00AC56000<6>,"010057E00AC56000","1.0.1"}},//Info Popup1
        {0x80cfac6c,{CODEC_UTF8,0,0,0,F010057E00AC56000<7>,"010057E00AC56000","1.0.1"}},//Info Popup2
        {0x8102d460,{CODEC_UTF8,1,0,0,F010057E00AC56000<8>,"010057E00AC56000","1.0.1"}},//Description
        //Yo-kai Watch Jam - Yo-kai Academy Y: Waiwai Gakuen 
        {0x80dd0cec,{CODEC_UTF8,0,0,0,F010051D010FC2000,"010051D010FC2000","4.0.0"}},//Dialogue text
        {0x80e33450,{CODEC_UTF8,3,0,0,F010051D010FC2000,"010051D010FC2000","4.0.0"}},//Other Dialogue text
        {0x80c807c0,{CODEC_UTF8,0,0,0,F010051D010FC2000,"010051D010FC2000","4.0.0"}},//Item description etc text
        {0x808d9a30,{CODEC_UTF8,0,0,0,F010051D010FC2000,"010051D010FC2000","4.0.0"}},//Tutorial Text
        {0x811b95ac,{CODEC_UTF8,3,0,0,F010051D010FC2000,"010051D010FC2000","4.0.0"}},//Menu screen
        {0x80e20290,{CODEC_UTF8,3,0,0,F010051D010FC2000,"010051D010FC2000","4.0.0"}},//Opening Song Text etc
        {0x80c43680,{CODEC_UTF8,3,0,0,F010051D010FC2000,"010051D010FC2000","4.0.0"}},//Cutscene Text
        //NEO: The World Ends With You
        {0x81581d6c,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F010043B013C5C000<0>,"010043B013C5C000","1.03"}},//Text
        {0x818eb248,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010043B013C5C000<1>,"010043B013C5C000","1.03"}},//Objective
        {0x81db84a4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010043B013C5C000<2>,"010043B013C5C000","1.03"}},//Menu: Collection Item Name
        {0x81db8660,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F010043B013C5C000<3>,"010043B013C5C000","1.03"}},//Menu: Collection Item Description
        {0x81c71a48,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010043B013C5C000<4>,"010043B013C5C000","1.03"}},//Tutorial Title
        {0x81c71b28,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F010043B013C5C000<5>,"010043B013C5C000","1.03"}},//Tutorial Description
        //Eiyuden Chronicle: Rising
        {0x82480190,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,0,"010039B015CB6000","1.02"}},//Main Text
        {0x824805d0,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,0,"010039B015CB6000","1.02"}},//Name
        {0x81f05c44,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"010039B015CB6000","1.02"}},//Intro Text
        {0x82522ac4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"010039B015CB6000","1.02"}},//Character Info
        {0x81b715f4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"010039B015CB6000","1.02"}},//Info
        {0x825274d0,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,0,"010039B015CB6000","1.02"}},//Info2
        {0x825269b0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"010039B015CB6000","1.02"}},//Tutorial Title
        {0x82526a0c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"010039B015CB6000","1.02"}},//Tutorial Description
        {0x82523e04,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"010039B015CB6000","1.02"}},//Objective Title
        {0x82524160,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"010039B015CB6000","1.02"}},//Objective Description
        {0x81f0351c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"010039B015CB6000","1.02"}},//Location Selection Title
        {0x81f0358c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"010039B015CB6000","1.02"}},//Location Selection Description
        {0x81f0d520,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"010039B015CB6000","1.02"}},//Quest Title
        {0x81f0d58c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"010039B015CB6000","1.02"}},//Quest Description
        {0x81f00318,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"010039B015CB6000","1.02"}},//Help Title
        {0x81f00368,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"010039B015CB6000","1.02"}},//Help Description
        {0x81f0866c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,0,"010039B015CB6000","1.02"}},//Config Description
        //Ghost Trick: Phantom Detective
        {0x81448898,{CODEC_UTF16,0,0,0,F010043B013C5C000<6>,"010029B018432000","1.0.0"}},//Main Text
        {0x80c540d4,{CODEC_UTF16,0,0,0,F010043B013C5C000<7>,"010029B018432000","1.0.0"}},//Secondary Text
        {0x80e50dd4,{CODEC_UTF16,0,0,0,F010043B013C5C000<8>,"010029B018432000","1.0.0"}},//Object Name
        {0x80f91c08,{CODEC_UTF16,0,0,0,F010043B013C5C000<9>,"010029B018432000","1.0.0"}},//Language Selection
        {0x805c9014,{CODEC_UTF16,0,0,0,F010043B013C5C000<10>,"010029B018432000","1.0.0"}},//Story/Character Info
        //Higurashi no Naku Koro ni Hou
        {0x800bd6c8,{0,0,0,0,F0100F6A00A684000,"0100F6A00A684000","1.0.0"}},//sjis
        {0x800c2d20,{0,0,0,0,F0100F6A00A684000,"0100F6A00A684000","1.2.0"}},//sjis
        //Umineko no Naku Koro ni Saku ~Nekobako to Musou no Koukyoukyoku~
        {0x800b4560,{CODEC_UTF8,0,0,0,0,"01006A300BA2C000","1.0.0"}},// x0 name + text (bottom, center) - whole line. filter is to complex, quit.
        {0x801049c0,{CODEC_UTF8,0,0,0,0,"01006A300BA2C000","1.0.0"}},// x0 prompt, bottomLeft
        {0x80026378,{CODEC_UTF8,0,0,0,0,"01006A300BA2C000","1.0.0"}},// x0 Yes|No
        {0x801049a8,{CODEC_UTF8,0,0,0,0,"01006A300BA2C000","1.0.0"}},// x0 topLeft (double: ♪ + text)

        //Koroshiya to Strawberry- Plus
        {0x81322cec,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F010042300C4F6000,"0100E390145C8000","1.0.0"}},//dialogue
        {0x819b1a78,{CODEC_UTF16,0,0,ReadTextAndLenW<2>,F010042300C4F6000,"0100E390145C8000","1.0.0"}},//dialogue
        {0x81314e8c,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F010042300C4F6000,"0100E390145C8000","1.0.0"}},//dialogue
        //Tokimeki Memorial Girl's Side 1st Love for Nintendo Switch
        {0x822454a4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//dialogue1
        {0x82247138,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//dialogue2
        {0x822472e0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//dialogue3
        {0x82156988,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//choice
        {0x82642200,{CODEC_UTF16,0,0,ReadTextAndLenDW<2>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//option1
        {0x81ecd758,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//option2
        {0x823185e4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//mail
        {0x823f2edc,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//roomDescript
        {0x821e3cf0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//dateDescript
        {0x81e20050,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//characterDesc1
        {0x81e1fe50,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//characterDesc2
        {0x81e1feb0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//characterDesc3
        {0x81e1ff04,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//characterDesc4
        {0x821d03b0,{CODEC_UTF16,0,0,ReadTextAndLenDW<3>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//news
        {0x82312008,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100D9A01BD86000,"0100D9A01BD86000","1.0.1"}},//luckyitem
        //Triangle Strategy
        {0x80aadebc,{CODEC_UTF16,0,0,0,F0100CC80140F8000<0>,"0100CC80140F8000","1.1.0"}},//Main Text
        {0x81358ce4,{CODEC_UTF16,3,0,0,F0100CC80140F8000<1>,"0100CC80140F8000","1.1.0"}},//Secondary Text
        {0x80a38988,{CODEC_UTF16,0,0,0,F0100CC80140F8000<2>,"0100CC80140F8000","1.1.0"}},//Info Contents
        {0x80aa4aec,{CODEC_UTF16,0,0,0,F0100CC80140F8000<3>,"0100CC80140F8000","1.1.0"}},//Info
        {0x80b1f300,{CODEC_UTF16,0,0,0,F0100CC80140F8000<4>,"0100CC80140F8000","1.1.0"}},//Difficulty Selection Part1
        {0x80b1f670,{CODEC_UTF16,0,0,0,F0100CC80140F8000<5>,"0100CC80140F8000","1.1.0"}},//Difficulty Selection Part2
        {0x80aa48f0,{CODEC_UTF16,0,0,0,F0100CC80140F8000<6>,"0100CC80140F8000","1.1.0"}},//PopUp Message
        //Xenoblade Chronicles 3
        {0x80cf6ddc,{CODEC_UTF8,0,0,0,F010074F013262000,"010074F013262000","2.2.0"}},//Main Text
        {0x80e76150,{CODEC_UTF8,0,0,0,F010074F013262000,"010074F013262000","2.2.0"}},//Secondary Text
        {0x807b4ee4,{CODEC_UTF8,1,0,0,F010074F013262000,"010074F013262000","2.2.0"}},//Tutorial Description
        {0x80850218,{CODEC_UTF8,0,0,0,F010074F013262000,"010074F013262000","2.2.0"}},//Objective
        //CLOCK ZERO ~Shuuen no Ichibyou~ Devote
        {0x8003c290,{0,0,0,0,F0100BDD01AAE4000,"01008C100C572000","1.0.0"}},//name,sjis
        {0x8003c184,{0,0,0,0,F0100BDD01AAE4000,"01008C100C572000","1.0.0"}},//dialogue
        {0x8001f6d0,{0,0,0,0,F0100BDD01AAE4000,"01008C100C572000","1.0.0"}},//prompt
        //Shuuen no Virche -ErroR:salvation
        {0x8001f594,{CODEC_UTF8,0,0x1C,0,F0100C310110B4000,"01005B9014BE0000","1.0.0"}},//dialog
        {0x8001f668,{CODEC_UTF8,0,0x1C,0,F0100C310110B4000,"01005B9014BE0000","1.0.0"}},//center
        {0x8003d540,{CODEC_UTF8,0,0,0,F0100C310110B4000,"01005B9014BE0000","1.0.0"}},//choice
        //Spade no Kuni no Alice ~Wonderful White World~ / スペードの国のアリス ～Wonderful White World～
        {0x8135d018,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F01008C0016544000,"01003FE00E2F8000","1.0.0"}},//Text + Name
        //十三支演義 偃月三国伝1・2 for Nintendo Switch
        {0x82031f20,{CODEC_UTF16,0,0,ReadTextAndLenDW<2>,F0100DA201E0DA000,"01003D2017FEA000","1.0.0"}},//name
        {0x82ef9550,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100DA201E0DA000,"01003D2017FEA000","1.0.0"}},//dialogue
        {0x83252e0c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100DA201E0DA000,"01003D2017FEA000","1.0.0"}},//choice
        //Tales of Vesperia: Definitive Edition
        {0x802de170,{CODEC_UTF8,2,0,0,F01002C0008E52000,"01002C0008E52000","1.0.2"}},//Ptc Text
        {0x802cf170,{CODEC_UTF8,3,0,0,F01002C0008E52000,"01002C0008E52000","1.0.2"}},//Cutscene
        {0x8019957c,{CODEC_UTF8,0,0,0,F01002C0008E52000,"01002C0008E52000","1.0.2"}},//Conversation
        {0x802c0600,{CODEC_UTF8,2,0,0,F01002C0008E52000,"01002C0008E52000","1.0.2"}},//Info
        {0x801135fc,{CODEC_UTF8,0,0,0,F01002C0008E52000,"01002C0008E52000","1.0.2"}},//Post Battle Text
        //Nil Adminari no Tenbin Irodori Nadeshiko
        {0x8005fd5c,{CODEC_UTF8,0,0,0,F0100BDD01AAE4000,"01002BB00A662000","1.0.0"}},//name
        {0x800db0d8,{CODEC_UTF8,0,20,0,F0100BDD01AAE4000,"01002BB00A662000","1.0.0"}},//name
        //Hanayaka Nari, Waga Ichizoku Modern Nostalgie
        {0x2509ac+0x80004000,{CODEC_UTF8,0,0,T0100B5500CA0C000,F0100B5500CA0C000,"01008DE00C022000","1.0.0"}},
        //Master Detective Archives: Rain Code
        {0x80bf2034,{CODEC_UTF16,0,0,0,F0100F4401940A000,"0100F4401940A000","1.3.3"}},//Dialogue text
        {0x80c099d4,{CODEC_UTF16,0,0,0,F0100F4401940A000,"0100F4401940A000","1.3.3"}},//Cutscene text
        {0x80cbf1f4,{CODEC_UTF16,0,0,0,F0100F4401940A000,"0100F4401940A000","1.3.3"}},//Menu
        {0x80cbc11c,{CODEC_UTF16,0,0,0,F0100DA201E0DA000,"0100F4401940A000","1.3.3"}},//Menu Item Description
        {0x80cacc14,{CODEC_UTF16,0,0,0,F0100DA201E0DA000,"0100F4401940A000","1.3.3"}},//Menu Item Description 2
        {0x80cd6410,{CODEC_UTF16,0,0,0,F0100DA201E0DA000,"0100F4401940A000","1.3.3"}},//Menu Item Description 3
        {0x80c214d4,{CODEC_UTF16,0,0,0,F0100F4401940A000,"0100F4401940A000","1.3.3"}},//Description
        {0x80cc9908,{CODEC_UTF16,0,0,0,F0100DA201E0DA000,"0100F4401940A000","1.3.3"}},//Mini game item description
        {0x80bce36c,{CODEC_UTF16,0,0,0,F0100F4401940A000,"0100F4401940A000","1.3.3"}},//Tutorial
        {0x80bcb7d4,{CODEC_UTF16,0,0,0,F0100F4401940A000,"0100F4401940A000","1.3.3"}},//Loading Screen information
        {0x80bf32d8,{CODEC_UTF16,0,0,0,F0100F4401940A000,"0100F4401940A000","1.3.3"}},//Choices
        //Fire Emblem: Three Houses
        {0x8041e6bc,{CODEC_UTF8,0,0,0,F010055D009F78000<0>,"010055D009F78000","1.2.0"}},//Main Text
        {0x805ca570,{CODEC_UTF8,0,0,0,F010055D009F78000<1>,"010055D009F78000","1.2.0"}},//Cutscene Text
        {0x8049f1e8,{CODEC_UTF8,0,0,0,F010055D009F78000<2>,"010055D009F78000","1.2.0"}},//Cutscene Text Scroll
        {0x805ee730,{CODEC_UTF8,0,0,0,F010055D009F78000<3>,"010055D009F78000","1.2.0"}},//Info
        {0x805ee810,{CODEC_UTF8,0,0,0,F010055D009F78000<4>,"010055D009F78000","1.2.0"}},//Info Choice
        {0x80467a60,{CODEC_UTF8,0,0,0,F010055D009F78000<5>,"010055D009F78000","1.2.0"}},//Location First Part
        {0x805f0340,{CODEC_UTF8,0,0,0,F010055D009F78000<6>,"010055D009F78000","1.2.0"}},//Location Second Part
        {0x801faae4,{CODEC_UTF8,0,0,0,F010055D009F78000<7>,"010055D009F78000","1.2.0"}},//Action Location
        {0x803375e8,{CODEC_UTF8,0,0,0,F010055D009F78000<8>,"010055D009F78000","1.2.0"}},//Objective
        {0x805fd870,{CODEC_UTF8,0,0,0,F010055D009F78000<9>,"010055D009F78000","1.2.0"}},//Tutorial
        {0x804022f8,{CODEC_UTF8,0,0,0,F010055D009F78000<10>,"010055D009F78000","1.2.0"}},//Request
        {0x802f7df4,{CODEC_UTF8,0,0,0,F010055D009F78000<11>,"010055D009F78000","1.2.0"}},//Quest Description
        {0x8031af0c,{CODEC_UTF8,0,0,0,F010055D009F78000<12>,"010055D009F78000","1.2.0"}},//Aproach Text
        //Sweet Clown ~Gozen San-ji no Okashi na Doukeshi~
        {0x20dbfc+0x80004000,{0,0,0x28,0,F010028D0148E6000,"010028D0148E6000","1.2.0"}},//dialog, sjis
        {0x214978+0x80004000,{0,2,0xC,0,F010028D0148E6000,"010028D0148E6000","1.2.0"}},//choices
        //Another Code: Recollection
        {0x82dcad30,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Main Text
        {0x82f2cfb0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Item Description
        {0x82dcc5fc,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Tutorial PopUp Header
        {0x82dcc61c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Tutorial PopUp Description
        {0x82f89e78,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Aproach Text
        {0x82973300,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Chapter
        {0x82dd2604,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Location
        {0x82bcb77c,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Save Message
        {0x828ccfec,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Acquired Item
        {0x83237b14,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Question Options
        {0x82dcee10,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Tutorial Header
        {0x82dcee38,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Tutorial Description
        {0x82e5cadc,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Character Info Name
        {0x82e5cc38,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Character Info Description
        {0x82871ac8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Letter Message
        {0x82e4dad4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//アナザーキー
        {0x82bd65d0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Message Title
        {0x82bd65f0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Message Content
        {0x82c1ccf0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Decision Header
        {0x82c1d218,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Decision1
        {0x82c1e43c,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100CB9018F5A000,"0100CB9018F5A000","1.0.0"}},//Decision2
        //AI: The Somnium Files
        {0x8165a9a4,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100C7400CFB4000<0>,"0100C7400CFB4000","1.0.2"}},//Main Text + Tutorial
        {0x80320dd4,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100C7400CFB4000<1>,"0100C7400CFB4000","1.0.2"}},//Menu Interface Text1
        {0x80320e20,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F0100C7400CFB4000<2>,"0100C7400CFB4000","1.0.2"}},//Menu Interface Text2
        //AI: The Somnium Files - nirvanA Initiative
        {0x8189ae64,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BD4014D8C000<0>,"0100BD4014D8C000","1.0.1"}},//Main Text + Tutorial
        {0x81813428,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BD4014D8C000<1>,"0100BD4014D8C000","1.0.1"}},//Hover Investigation Text
        {0x82e122b8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BD4014D8C000<2>,"0100BD4014D8C000","1.0.1"}},//Info
        {0x82cffff8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BD4014D8C000<3>,"0100BD4014D8C000","1.0.1"}},//Config Description
        {0x818c3cd8,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BD4014D8C000<4>,"0100BD4014D8C000","1.0.1"}},//File: Names
        {0x82ea1a38,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BD4014D8C000<5>,"0100BD4014D8C000","1.0.1"}},//File: Contents
        {0x82cbb1fc,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F0100BD4014D8C000<6>,"0100BD4014D8C000","1.0.1"}},//Investigation Choices
        //Fata morgana no Yakata ~Dreams of the Revenants Edition~ / ファタモ
        {0x8025a998,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01008C0016544000,"0100BE40138B8000","1.0.1"}},//Main Text
        {0x801d6050,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01008C0016544000,"0100BE40138B8000","1.0.1"}},//Choices
        //Ni no Kuni II: Revenant Kingdom
        {0x80ac651c,{CODEC_UTF8,0,0,0,F0100C4E013E5E000,"0100C4E013E5E000","1.0.0"}},//Main Text
        {0x80335ea0,{CODEC_UTF8,0,0,0,F0100C4E013E5E000,"0100C4E013E5E000","1.0.0"}},//Name
        //Harukanaru Toki no Naka de 7
        {0x800102bc,{0,0,0,T0100CF400F7CE000<0>,F0100CF400F7CE000,"0100CF400F7CE000","1.0.0"}},//name, sjis
        {0x80051f90,{0,0,0,T0100CF400F7CE000<1>,F0100CF400F7CE000,"0100CF400F7CE000","1.0.0"}},//text
        {0x80010b48,{0,0,0,T0100CF400F7CE000<0>,F0100CF400F7CE000,"0100CF400F7CE000","1.0.0"}},//prompt
        {0x80010c80,{0,0,0,T0100CF400F7CE000<0>,F0100CF400F7CE000,"0100CF400F7CE000","1.0.0"}},//choice
        //Angelique Luminarise
        {0x80046c04,{0,0,0,T0100CF400F7CE000<0>,F0100CF400F7CE000,"0100D11018A7E000","1.0.0"}},//ingameDialogue, sjis
        {0x80011284,{0,0,0,T0100CF400F7CE000<0>,F0100CF400F7CE000,"0100D11018A7E000","1.0.0"}},//choice
        {0x80011140,{0,0,0,T0100CF400F7CE000<0>,F0100CF400F7CE000,"0100D11018A7E000","1.0.0"}},//prompt first
        //Star Ocean The Second Story R
        {0x81d5e4d0,{0,0,0,ReadTextAndLenDW<1>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Main Text + Tutorial
        {0x81d641b4,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Intro Cutscene
        {0x824b1f00,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Character Selection Name
        {0x81d4c670,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Character Selection Lore
        {0x8203a048,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//General Description
        {0x82108cd0,{0,0,0,ReadTextAndLenDW<1>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Unique Spot Title
        {0x827a9848,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Chest Item
        {0x82756890,{0,0,0,ReadTextAndLenDW<1>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Info
        {0x82241410,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Menu Talk
        {0x81d76404,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Secondary Talk
        {0x821112e0,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Location
        {0x82111320,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Location Interior
        {0x81d6ea24,{0,0,0,ReadTextAndLenDW<1>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Special Arts/Spells Name
        {0x81d6ea68,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Special Arts/Spells Description
        {0x81d6ed48,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Special Arts/Spells Range
        {0x81d6eb3c,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Special Arts/Spells Effect
        {0x81d6f880,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Special Arts/Spells Bonus
        {0x8246d81c,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Tactics Name
        {0x8246d83c,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Tactics Description
        {0x8212101c,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Achievements Name
        {0x82121088,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Achievements Description
        {0x81d6c480,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Acquired Item1
        {0x821143f0,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Acquired Item2
        {0x81d6fb18,{0,0,0,ReadTextAndLenDW<1>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Battle Skill Name
        {0x81d6fb4c,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Battle Skill Description
        {0x81d6fb7c,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Battle Skill Bonus Description
        {0x8212775c,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Battle Item Name
        {0x82127788,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Battle Item Description
        {0x821361ac,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Battle Ability Name
        {0x821361f4,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Battle Ability Range
        {0x82136218,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Battle Ability Effect
        {0x8238451c,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Battle Strategy Name
        {0x82134610,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Battle Acquired Item
        {0x824b5eac,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Item Name
        {0x824b5f04,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Item Description
        {0x824b5f54,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Item Effect
        {0x81d71790,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Item Factor Title
        {0x824b62c0,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Item Factor Description
        {0x824c2e2c,{0,0,0,ReadTextAndLenDW<1>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//IC/Specialty Skills Name
        {0x824c2e54,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//IC/Specialty Skills Description
        {0x824c2fbc,{0,0,0,ReadTextAndLenDW<1>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//IC/Specialty Skills Level
        {0x823e7230,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//IC/Specialty Name
        {0x823e94bc,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//IC/Specialty Description
        {0x823e9980,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//IC/Specialty Talent
        {0x823ea9c4,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//IC/Specialty Support Item
        {0x82243b18,{0,0,0,ReadTextAndLenDW<1>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Enemy Info Skills
        {0x81d64540,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Guild Mission Description
        {0x823b4f6c,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Guild Mission Reward
        {0x826facd8,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Challenge Mission Description
        {0x826f98f8,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Challenge Mission Reward
        {0x8244af2c,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Formation Name
        {0x8244ae90,{0,0,0,ReadTextAndLenDW<0>,F010065301A2E0000,"010065301A2E0000","1.0.2"}},//Formation Description
        //魔法使いの夜 通常版
        {0x80086ba0,{CODEC_UTF8,0,0,T010012A017F18000,0,"010012A017F18000","1.0.0"}},
        {0x80086e70,{CODEC_UTF8,0,0,T010012A017F18000,0,"010012A017F18000","1.0.2"}},
        //月姫 -A piece of blue glass moon-
        {0x800ac290,{CODEC_UTF8,0,0,T010012A017F18000,0,"01001DC01486A000",0}},//1.0.1,1.0.2
        //The Quintessential Quintuplets the Movie: Five Memories of My Time with You (JP)
        {0x80011688,{CODEC_UTF8,1,0,0,F01005E9016BDE000,"01005E9016BDE000","1.0.0"}},//dialogue, menu, choice, name
        // Flowers: Les Quatre Saisons 
        {0x8006f940,{CODEC_UTF16,1,0,0,F01002AE00F442000,"01002AE00F442000","1.0.1"}},
        //最悪なる災厄人間に捧ぐ eSHOP [01000A400AF2A000][v0]
        {0x8034EB44,{CODEC_UTF16,8,0,0,F01000A400AF2A000,"01000A400AF2A000","1.0.0"}},//text
        //神様のような君へ
        {0x80487CD0,{CODEC_UTF8,0,0,0,F01006B5014E2E000,"01006B5014E2E000","1.0.0"}},//text
        //BUSTAFELLOWS
        {0x80191b18,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F0100874017BE2000,"010060800B7A8000","1.1.3"}},//Dialogue
        {0x80191f88,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F0100874017BE2000,"010060800B7A8000","1.1.3"}},//Choice
        {0x801921a4,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F0100874017BE2000,"010060800B7A8000","1.1.3"}},//Choice 2
        {0x801935f0,{CODEC_UTF16,0,0,ReadTextAndLenW<0>,F0100874017BE2000,"010060800B7A8000","1.1.3"}},//option
        //Moujuutsukai to Ouji-sama ~Flower ＆ Snow~ 
        {0x800a1a10,{CODEC_UTF8,1,0,0,F01001B900C0E2000,"01001B900C0E2000","1.0.0"}},//Dialogue 1
        {0x80058f80,{CODEC_UTF8,1,0,0,F01001B900C0E2000,"01001B900C0E2000","1.0.0"}},//Dialogue 2
        //Detective Pikachu Returns
        {0x81585750,{CODEC_UTF16,0,0,ReadTextAndLenDW<2>,F010007500F27C000,"010007500F27C000","1.0.0"}},//All Text
        //Dragon Quest Treasures
        {0x80bd62c4,{CODEC_UTF16,0,0,0,F0100217014266000,"0100217014266000","1.0.1"}},//Cutscene
        {0x80a74b64,{CODEC_UTF16,0,0,0,F0100217014266000,"0100217014266000","1.0.1"}},//Ptc Text
        {0x80a36d18,{CODEC_UTF16,0,0,0,F0100217014266000,"0100217014266000","1.0.1"}},//Info
        {0x80c43878,{CODEC_UTF16,0,0,0,F0100217014266000,"0100217014266000","1.0.1"}},//Tutorial Title
        {0x80c43d50,{CODEC_UTF16,0,0,0,F0100217014266000,"0100217014266000","1.0.1"}},//Tutorial Description
        {0x80a72598,{CODEC_UTF16,0,0,0,F0100217014266000,"0100217014266000","1.0.1"}},//Aproach Text
        //Rune Factory 4 Special
        {0x48b268+0x80004000,{CODEC_UTF8,3,0,0,F010027100C79A000,"010027100C79A000","1.0.1"}},//All Text
        //The Legend of Zelda: Skyward Sword HD
        {0x80dc36dc,{CODEC_UTF16|FULL_STRING,3,0,0,F01001EF017BE6000,"01002DA013484000","1.0.1"}},//All Text
        //World of Final Fantasy Maxima
        {0x8068fea0,{CODEC_UTF8,0,0,0,F010072000BD32000<0>,"010072000BD32000","1.0.0"}},//Cutscene
        {0x802c6a48,{CODEC_UTF8,0,0,0,F010072000BD32000<1>,"010072000BD32000","1.0.0"}},//Action Text
        {0x803a523c,{CODEC_UTF8,1,0,0,F010072000BD32000<2>,"010072000BD32000","1.0.0"}},//Location
        {0x8041ed64,{CODEC_UTF8,0,0,0,F010072000BD32000<3>,"010072000BD32000","1.0.0"}},//Info
        {0x802c9f1c,{CODEC_UTF8,0,0,0,F010072000BD32000<4>,"010072000BD32000","1.0.0"}},//Chapter First Part
        {0x802c9f6c,{CODEC_UTF8,0,0,0,F010072000BD32000<5>,"010072000BD32000","1.0.0"}},//Chapter Second Part
        //Tokyo Xanadu eX+
        {0x8025135c,{CODEC_UTF8,1,0,0,F010080C01AA22000,"010080C01AA22000","1.0.0"}},//Name
        {0x80251068,{CODEC_UTF8,0,0,0,F010080C01AA22000,"010080C01AA22000","1.0.0"}},//Main Text
        {0x802ac86c,{CODEC_UTF8,0,0,0,F010080C01AA22000,"010080C01AA22000","1.0.0"}},//Action Text
        {0x802b04b4,{CODEC_UTF8,0,0,0,F010080C01AA22000,"010080C01AA22000","1.0.0"}},//Choices
        {0x8013243c,{CODEC_UTF8,0,0,0,F010080C01AA22000,"010080C01AA22000","1.0.0"}},//Location
        {0x802b1f3c,{CODEC_UTF8,0,0,0,F010080C01AA22000,"010080C01AA22000","1.0.0"}},//Info
        {0x802ab46c,{CODEC_UTF8,0,0,0,F010080C01AA22000,"010080C01AA22000","1.0.0"}},//Documents
        //DORAEMON STORY OF SEASONS: Friends of the Great Kingdom
        {0x839558e4,{CODEC_UTF16,0,0,ReadTextAndLenDW<1>,F01009B50139A8000<0>,"01009B50139A8000","1.1.1"}},//Text
        {0x8202a9b0,{CODEC_UTF16,0,0,ReadTextAndLenDW<0>,F01009B50139A8000<1>,"01009B50139A8000","1.1.1"}},//Tutorial
        //Monster Hunter Stories 2: Wings of Ruin
        {0x8042fe60,{CODEC_UTF8,1,0,0,F0100CB700D438000<0>,"0100CB700D438000","1.5.2"}},//Cutscene
        {0x804326c0,{CODEC_UTF8,1,0,0,F0100CB700D438000<1>,"0100CB700D438000","1.5.2"}},//Ptc Text
        {0x804d3d44,{CODEC_UTF8,0,0,0,F0100CB700D438000<2>,"0100CB700D438000","1.5.2"}},//Info
        {0x8045e7c8,{CODEC_UTF8,0,0,0,F0100CB700D438000<3>,"0100CB700D438000","1.5.2"}},//Info Choice
        {0x805cec4c,{CODEC_UTF8,0,0,0,F0100CB700D438000<4>,"0100CB700D438000","1.5.2"}},//Config Header
        {0x8078c2d0,{CODEC_UTF8,0,0,0,F0100CB700D438000<5>,"0100CB700D438000","1.5.2"}},//Config Name+
        {0x805d0858,{CODEC_UTF8,0,0,0,F0100CB700D438000<6>,"0100CB700D438000","1.5.2"}},//Config Description
        {0x807612d4,{CODEC_UTF8,0,0,0,F0100CB700D438000<7>,"0100CB700D438000","1.5.2"}},//Notice
        {0x807194a0,{CODEC_UTF8,1,0,0,F0100CB700D438000<8>,"0100CB700D438000","1.5.2"}},//Update Content + Tutorial
        {0x804d687c,{CODEC_UTF8,0,0,0,F0100CB700D438000<9>,"0100CB700D438000","1.5.2"}},//Objective Title
        {0x804d6a7c,{CODEC_UTF8,0,0,0,F0100CB700D438000<10>,"0100CB700D438000","1.5.2"}},//Objective Description
        {0x80509900,{CODEC_UTF8,0,0,0,F0100CB700D438000<11>,"0100CB700D438000","1.5.2"}},//Aproach Text
        {0x8060ee90,{CODEC_UTF8,1,0,0,F0100CB700D438000<12>,"0100CB700D438000","1.5.2"}},//Acquired Item
        //２０４５、月より。
        {0x80016334,{CODEC_UTF8,1,0,0,F01005C301AC5E000,"01005C301AC5E000","1.0.1"}},
        //ヤマノススメ Next Summit ～あの山に、もう一度～
        {0x806E1444,{CODEC_UTF8,0,0,0,F0100815019488000_text,"0100815019488000","1.0.0"}},
        {0x80659EE0,{CODEC_UTF8,1,0,0,F0100815019488000_name,"0100815019488000","1.0.0"}},

    };
    return 1;
}();
}