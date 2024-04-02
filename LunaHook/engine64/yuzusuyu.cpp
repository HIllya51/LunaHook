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

uintptr_t* argidx(hook_stack* stack,int idx){
    auto offset=0;
    switch (idx)
    {
    case 0:offset=get_reg(regs::rcx);break;
    case 1:offset=get_reg(regs::rdx);break;
    case 2:offset=get_reg(regs::r8);break;
    case 3:offset=get_reg(regs::r9);break;
    }
    return (uintptr_t*)((uintptr_t)stack+sizeof(hook_stack)-sizeof(uintptr_t)+offset);
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
        if((game_info.version.size())&&(info.title.find(acastw(game_info.version))!=info.title.npos)&&(game_info.id!=0)){
            //判断是有效的info
            auto checkversion=(std::string(em._version)==(game_info.version));
            auto checkid=(std::stoll(em._id,0,16)==game_info.id);
            if(checkid&&checkversion)return true;
        }
		else if(info.title.find(acastw(em._version))!=info.title.npos)return true;
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


template<int index>
void ReadTextAndLen(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=YUZU::emu_arg(stack)[index];
    *len=(*(DWORD*)(address+0x10))*2;
    *data=address+0x14;
}


void _0100978013276000(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto s=mages::readString(YUZU::emu_arg(stack)[0],0);
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
    s = std::regex_replace(s, std::regex("Save[\\s\\S]*データ"), "");
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
bool F0100C29017106000(void* data, size_t* len, HookParam* hp){
    auto s = std::wstring((wchar_t*)data,*len/2);
    std::wregex pattern(L"\\n+|(\\n)+");
    s = std::regex_replace(s, pattern, L" ");
	return write_string_overwrite(data,len,s);
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
            std::regex regex("[\\d+─]");
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
    static std::string readString_playerName="ラピス";
     
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
                if (readString_playerName == "ラピス") {
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
            std::regex regex("[\\d+─]");
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

bool F010001D015260000(void* data, size_t* len, HookParam* hp){
    auto s=std::string((char*)data,*len);
    if(startWith(s,"#Key"))return false;
    strReplace(s,"#n","\n");
    return write_string_overwrite(data,len,s);
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

bool F01006F000B056000(void* data, size_t* len, HookParam* hp){
    auto s=std::wstring((wchar_t*)data,*len/2);
    s = std::regex_replace(s, std::wregex(L"\\[.*?\\]"), L" ");
    return write_string_overwrite(data,len,s);
}

namespace{
auto _=[](){
    emfunctionhooks={
            //Memories Off
            {0x8003eeac,{CODEC_UTF16,0,0,_0100978013276000,0,"0100978013276000","1.0.0"}},
            {0x8003eebc,{CODEC_UTF16,0,0,_0100978013276000,0,"0100978013276000","1.0.1"}},
            
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
            {0x805bba5c,{CODEC_UTF16,0,0,ReadTextAndLen<2>,F0100A1E00BFEA000,"0100A1E00BFEA000","1.0.1"}},//dialogue
            {0x805e9930,{CODEC_UTF16,0,0,ReadTextAndLen<1>,F0100A1E00BFEA000,"0100A1E00BFEA000","1.0.1"}},//choice
            {0x805e7fd8,{CODEC_UTF16,0,0,ReadTextAndLen<1>,F0100A1E00BFEA000,"0100A1E00BFEA000","1.0.1"}},//name

            //Chou no Doku Hana no Kusari Taishou Tsuya Koi Ibun
            {0x80095010,{CODEC_UTF16,1,0,0,F0100A1200CA3C000,"0100A1200CA3C000","2.0.1"}},//Main Text + Names
            //Live a Live
            {0x80a05170,{CODEC_UTF16,0,0,0,F0100C29017106000,"0100C29017106000","1.0.0"}},
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

            //CHAOS;HEAD NOAH
            {0x80046700,{CODEC_UTF16,0,0,_0100978013276000,0,"0100957016B90000","1.0.0"}},
            {0x8003A2c0,{CODEC_UTF16,0,0,_0100978013276000,0,"0100957016B90000","1.0.0"}},// choice
            {0x8003EAB0,{CODEC_UTF16,0,0,_0100978013276000,0,"0100957016B90000","1.0.0"}},// TIPS list (menu)
            {0x8004C648,{CODEC_UTF16,0,0,_0100978013276000,0,"0100957016B90000","1.0.0"}},// system message
            {0x80050374,{CODEC_UTF16,0,0,_0100978013276000,0,"0100957016B90000","1.0.0"}},// TIPS (red)
            
            //Story of Seasons a Wonderful Life
            {0x80ac4d88,{CODEC_UTF32,0,0,F0100936018EB4000,"0100936018EB4000","1.0.3"}},// Main text
            {0x808f7e84,{CODEC_UTF32,0,0,F0100936018EB4000,"0100936018EB4000","1.0.3"}},// Item name
            {0x80bdf804,{CODEC_UTF32,0,0,F0100936018EB4000,"0100936018EB4000","1.0.3"}},// Item description
            //Hamefura Pirates
            {0x81e75940,{CODEC_UTF16,0,0,ReadTextAndLen<1>,F0100982015606000,"0100982015606000","1.0.0"}},// Hamekai.TalkPresenter$$AddMessageBacklog
            {0x81c9ae60,{CODEC_UTF16,0,0,ReadTextAndLen<1>,F0100982015606000,"0100982015606000","1.0.0"}},// Hamekai.ChoicesText$$SetText
            {0x81eb7dc0,{CODEC_UTF16,0,0,ReadTextAndLen<1>,F0100982015606000,"0100982015606000","1.0.0"}},// Hamekai.ShortStoryTextView$$AddText
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
            {0x83e93ca0,{CODEC_UTF16,0,0,ReadTextAndLen<0>,F01008C0016544000,"01008C0016544000","1.0.45861"}},// Main text
            {0x820c3fa0,{CODEC_UTF16,0,0,ReadTextAndLen<0>,F01008C0016544000,"01008C0016544000","1.0.47140"}},// Main text
            //Final Fantasy I
            {0x81e88040,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01000EA014150000","1.0.1"}},// Main text
            {0x81cae54c,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01000EA014150000","1.0.1"}},// Intro text
            {0x81a3e494,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01000EA014150000","1.0.1"}},// battle text
            {0x81952c28,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01000EA014150000","1.0.1"}},// Location
            //Final Fantasy II
            {0x8208f4cc,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01006B7014156000","1.0.1"}},// Main text
            {0x817e464c,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01006B7014156000","1.0.1"}},// Intro text
            {0x81fb6414,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01006B7014156000","1.0.1"}},// battle text
            //Final Fantasy III
            {0x82019e84,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01002E2014158000","1.0.1"}},// Main text1
            {0x817ffcfc,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01002E2014158000","1.0.1"}},// Main text2
            {0x81b8b7e4,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01002E2014158000","1.0.1"}},// battle text
            {0x8192c4a8,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01002E2014158000","1.0.1"}},// Location
            //Final Fantasy IV
            {0x81e44bf4,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01004B301415A000","1.0.2"}},// Main text
            {0x819f92c4,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01004B301415A000","1.0.2"}},// Rolling text
            {0x81e2e798,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01004B301415A000","1.0.2"}},// Battle text
            {0x81b1e6a8,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"01004B301415A000","1.0.2"}},// Location
            //Final Fantasy V
            {0x81d63e24,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"0100AA201415C000","1.0.2"}},// Main text
            {0x81adfb3c,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"0100AA201415C000","1.0.2"}},// Location
            {0x81a8fda8,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"0100AA201415C000","1.0.2"}},// Battle text
            //Final Fantasy VI
            {0x81e6b350,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"0100AA001415E000","1.0.2"}},// Main text
            {0x81ab40ec,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"0100AA001415E000","1.0.2"}},// Location
            {0x819b8c88,{CODEC_UTF16,0,0,ReadTextAndLen<0>,0,"0100AA001415E000","1.0.2"}},// Battle text
            //Final Fantasy IX
            {0x80034b90,{CODEC_UTF16,0,0,ReadTextAndLen<1>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Main Text
            {0x802ade64,{CODEC_UTF16,0,0,ReadTextAndLen<0>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Battle Text
            {0x801b1b84,{CODEC_UTF16,0,0,ReadTextAndLen<0>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Descriptions
            {0x805aa0b0,{CODEC_UTF16,0,0,ReadTextAndLen<0>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Key Item Name
            {0x805a75d8,{CODEC_UTF16,0,0,ReadTextAndLen<0>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Key Item Content
            {0x8002f79c,{CODEC_UTF16,0,0,ReadTextAndLen<0>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Menu
            {0x80ca88b0,{CODEC_UTF16,0,0,ReadTextAndLen<0>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Tutorial1
            {0x80ca892c,{CODEC_UTF16,0,0,ReadTextAndLen<0>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Tutorial2
            {0x80008d88,{CODEC_UTF16,0,0,ReadTextAndLen<1>,F01006F000B056000,"01006F000B056000","1.0.1"}},// Location

    };
    return 1;
}();
}