#include"yuzusuyu.h"
#include"mages/mages.h"
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

class emu_arg{
    hook_stack* stack;
public:
    emu_arg(hook_stack* stack_):stack(stack_){};
    uintptr_t operator [](int idx){
        auto base=stack->r13;
        auto args=(uintptr_t*)stack->r15;
        return base+args[idx];
    }
};
struct emfuncinfo{
    const char* hookname;
    void* hookfunc;
	void* filterfun;
    const wchar_t* _id;
    const wchar_t* _version;
};
std::unordered_map<uintptr_t,emfuncinfo>emfunctionhooks;


bool checkiscurrentgame(const emfuncinfo& em){
	auto wininfos=get_proc_windows();
	for(auto&& info:wininfos){
		if(info.title.find(em._version)!=info.title.npos)return true;
	}
	return false;
}


template<int index,int offset=0>
void simpleutf8getter(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=emu_arg(stack)[index]+offset;
    hp->type=USING_STRING|CODEC_UTF8|NO_CONTEXT|BREAK_POINT;
    *data=address;*len=strlen((char*)address);
}
template<int index,DWORD _type=0>
void simpleutf16getter(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=emu_arg(stack)[index];
    hp->type=USING_STRING|CODEC_UTF16|NO_CONTEXT|BREAK_POINT|_type;
    *data=address;*len=wcslen((wchar_t*)address)*2;
}

}
bool yuzusuyu::attach_function()
{
    ConsoleOutput("[Compatibility] Yuzu 1616+");
   auto DoJitPtr=getDoJitAddress();
   if(DoJitPtr==0)return false;
   ConsoleOutput("DoJitPtr %p",DoJitPtr);
   HookParam hp;
   hp.address=DoJitPtr;
   hp.text_fun=[](hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
        auto descriptor = *argidx(stack,idxDescriptor); // r8
        auto entrypoint = *argidx(stack,idxEntrypoint); // r9
        auto em_address = *(uintptr_t*)descriptor;
        em_address-=0x80004000;
        if(emfunctionhooks.find(em_address)==emfunctionhooks.end() || !entrypoint)return;
        auto op=emfunctionhooks.at(em_address);
        if(!(checkiscurrentgame(op)))return;

        HookParam hpinternal;
        hpinternal.address=entrypoint;
        hpinternal.type=CODEC_UTF16|USING_STRING|NO_CONTEXT|BREAK_POINT;
        hpinternal.text_fun=(decltype(hpinternal.text_fun))op.hookfunc;
		hpinternal.filter_fun=(decltype(hpinternal.filter_fun))op.filterfun;
        NewHook(hpinternal,op.hookname);
    
   };
  return NewHook(hp,"YuzuDoJit");
} 

void _0100978013276000(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto s=mages::readString(emu_arg(stack)[0],0);
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

template<int index>
void T0100A1E00BFEA000(hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
    auto address=emu_arg(stack)[index];
    *len=(*(WORD*)(address+0x10))*2;
    *data=address+0x14;
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
namespace{
auto _=[](){
    emfunctionhooks={
            {0x8003eeac - 0x80004000,{"Memories Off",_0100978013276000,0,L"0100978013276000",L"1.0.0"}},
            {0x8003eebc - 0x80004000,{"Memories Off",_0100978013276000,0,L"0100978013276000",L"1.0.1"}},
            
            // Shiro to Kuro no Alice
            {0x80013f20 - 0x80004000,{"Shiro to Kuro no Alice",simpleutf8getter<0>,NewLineCharFilterW,L"0100A460141B8000",L"1.0.0"}},
            {0x80013f94 - 0x80004000,{"Shiro to Kuro no Alice",simpleutf8getter<0>,NewLineCharFilterW,L"0100A460141B8000",L"1.0.0"}},
            {0x8001419c - 0x80004000,{"Shiro to Kuro no Alice",simpleutf8getter<0>,NewLineCharFilterW,L"0100A460141B8000",L"1.0.0"}},
            // Shiro to Kuro no Alice -Twilight line-
            {0x80014260 - 0x80004000,{"Shiro to Kuro no Alice -Twilight line-",simpleutf8getter<0>,NewLineCharFilterW,L"0100A460141B8000",L"1.0.0"}},
            {0x800142d4 - 0x80004000,{"Shiro to Kuro no Alice -Twilight line-",simpleutf8getter<0>,NewLineCharFilterW,L"0100A460141B8000",L"1.0.0"}},
            {0x800144dc - 0x80004000,{"Shiro to Kuro no Alice -Twilight line-",simpleutf8getter<0>,NewLineCharFilterW,L"0100A460141B8000",L"1.0.0"}},
            
            {0x80072d00 - 0x80004000,{"CLANNAD",simpleutf16getter<1,FULL_STRING>,F0100A3A00CC7E000,L"0100A3A00CC7E000",L"1.0.0"}},
            {0x80072d30 - 0x80004000,{"CLANNAD",simpleutf16getter<1,FULL_STRING>,F0100A3A00CC7E000,L"0100A3A00CC7E000",L"1.0.7"}},

            {0x800e3424 - 0x80004000,{"VARIABLE BARRICADE NS",simpleutf8getter<0>,F010045C0109F2000,L"010045C0109F2000",L"1.0.1"}},//"System Messages + Choices"), //Also includes the names of characters, 
            {0x800fb080 - 0x80004000,{"VARIABLE BARRICADE NS",simpleutf8getter<3>,F010045C0109F2000,L"010045C0109F2000",L"1.0.1"}},//Main Text
            
            {0x805bba5c - 0x80004000,{"AMNESIA for Nintendo Switch",T0100A1E00BFEA000<2>,F0100A1E00BFEA000,L"0100A1E00BFEA000",L"1.0.1"}},//dialogue
            {0x805e9930 - 0x80004000,{"AMNESIA for Nintendo Switch",T0100A1E00BFEA000<2>,F0100A1E00BFEA000,L"0100A1E00BFEA000",L"1.0.1"}},//choice
            {0x805e7fd8 - 0x80004000,{"AMNESIA for Nintendo Switch",T0100A1E00BFEA000<2>,F0100A1E00BFEA000,L"0100A1E00BFEA000",L"1.0.1"}},//name

            
            {0x80095010 - 0x80004000,{"Chou no Doku Hana no Kusari Taishou Tsuya Koi Ibun",simpleutf16getter<1>,F0100A1200CA3C000,L"0100A1200CA3C000",L"2.0.1"}},//Main Text + Names

            {0x80a05170 - 0x80004000,{"Live a Live",simpleutf16getter<0>,F0100C29017106000,L"0100C29017106000",L"1.0.0"}},
            
            {0x8049d968 - 0x80004000,{"Sakura no Kumo * Scarlet no Koi",simpleutf8getter<0,1>,F01006590155AC000,L"01006590155AC000",L"1.0.0"}},//name
            {0x8049d980 - 0x80004000,{"Sakura no Kumo * Scarlet no Koi",simpleutf8getter<0>,F01006590155AC000,L"01006590155AC000",L"1.0.0"}},//dialogue

            {0x80557408 - 0x80004000,{"Majestic Majolical",simpleutf8getter<0>,F01000200194AE000,L"01000200194AE000",L"1.0.0"}},//name
            {0x8059ee94 - 0x80004000,{"Majestic Majolical",simpleutf8getter<3>,F01000200194AE000,L"01000200194AE000",L"1.0.0"}},//player name
            {0x80557420 - 0x80004000,{"Majestic Majolical",simpleutf8getter<0>,F01000200194AE000,L"01000200194AE000",L"1.0.0"}},//dialogue
    };
    return 1;
}();
}