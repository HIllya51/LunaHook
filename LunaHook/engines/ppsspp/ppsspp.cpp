
#include"engine.h"
#include"util/util.h"
#include"psputils.hpp"
#include"specialgames.hpp"
// See: https://github.com/hrydgard/ppsspp

// Core/HLE (High Level Emulator)
// - sceCcc
//   #void sceCccSetTable(u32 jis2ucs, u32 ucs2jis)
//   int sceCccUTF8toUTF16(u32 dstAddr, u32 dstSize, u32 srcAddr)
//   int sceCccUTF8toSJIS(u32 dstAddr, u32 dstSize, u32 srcAddr)
//   int sceCccUTF16toUTF8(u32 dstAddr, u32 dstSize, u32 srcAddr)
//   int sceCccUTF16toSJIS(u32 dstAddr, u32 dstSize, u32 srcAddr)
//   int sceCccSJIStoUTF8(u32 dstAddr, u32 dstSize, u32 srcAddr)
//   int sceCccSJIStoUTF16(u32 dstAddr, u32 dstSize, u32 srcAddr)
//   int sceCccStrlenUTF8(u32 strAddr)
//   int sceCccStrlenUTF16(u32 strAddr)
//   int sceCccStrlenSJIS(u32 strAddr)
//   u32 sceCccEncodeUTF8(u32 dstAddrAddr, u32 ucs)
//   void sceCccEncodeUTF16(u32 dstAddrAddr, u32 ucs)
//   u32 sceCccEncodeSJIS(u32 dstAddrAddr, u32 jis)
//   u32 sceCccDecodeUTF8(u32 dstAddrAddr)
//   u32 sceCccDecodeUTF16(u32 dstAddrAddr)
//   u32 sceCccDecodeSJIS(u32 dstAddrAddr)
//   int sceCccIsValidUTF8(u32 c)
//   int sceCccIsValidUTF16(u32 c)
//   int sceCccIsValidSJIS(u32 c)
//   int sceCccIsValidUCS2(u32 c)
//   int sceCccIsValidUCS4(u32 c)
//   int sceCccIsValidJIS(u32 c)
//   int sceCccIsValidUnicode(u32 c)
//   #u32 sceCccSetErrorCharUTF8(u32 c)
//   #u32 sceCccSetErrorCharUTF16(u32 c)
//   #u32 sceCccSetErrorCharSJIS(u32 c)
//   u32 sceCccUCStoJIS(u32 c, u32 alt)
//   u32 sceCccJIStoUCS(u32 c, u32 alt)
// - sceFont: search charCode
//   int sceFontGetCharInfo(u32 fontHandle, u32 charCode, u32 charInfoPtr)
//   int sceFontGetShadowInfo(u32 fontHandle, u32 charCode, u32 charInfoPtr)
//   int sceFontGetCharImageRect(u32 fontHandle, u32 charCode, u32 charRectPtr)
//   int sceFontGetShadowImageRect(u32 fontHandle, u32 charCode, u32 charRectPtr)
//   int sceFontGetCharGlyphImage(u32 fontHandle, u32 charCode, u32 glyphImagePtr)
//   int sceFontGetCharGlyphImage_Clip(u32 fontHandle, u32 charCode, u32 glyphImagePtr, int clipXPos, int clipYPos, int clipWidth, int clipHeight)
//   #int sceFontSetAltCharacterCode(u32 fontLibHandle, u32 charCode)
//   int sceFontGetShadowGlyphImage(u32 fontHandle, u32 charCode, u32 glyphImagePtr)
//   int sceFontGetShadowGlyphImage_Clip(u32 fontHandle, u32 charCode, u32 glyphImagePtr, int clipXPos, int clipYPos, int clipWidth, int clipHeight)
// - sceKernelInterrupt
//   u32 sysclib_strcat(u32 dst, u32 src)
//   int sysclib_strcmp(u32 dst, u32 src)
//   u32 sysclib_strcpy(u32 dst, u32 src)
//   u32 sysclib_strlen(u32 src)
//
// Sample debug string:
//     006EFD8E   PUSH PPSSPPWi.00832188                    ASCII "sceCccEncodeSJIS(%08x, U+%04x)"
// Corresponding source code in sceCcc:
//     ERROR_LOG(HLE, "sceCccEncodeSJIS(%08x, U+%04x): invalid pointer", dstAddrAddr, jis);

struct PPSSPPFunction
{
  const char *hookName; // hook name
  int argIndex;      // argument index
  uint64_t hookType;       // hook parameter type
  int hookSplit;      // hook parameter split, positive: stack, negative: registers
  const char *pattern;  // debug string used within the function
}; 

uint64_t findleaaddr(uint64_t addr,uint64_t start,uint64_t end)
{
    for(auto _addr=start;_addr<end;_addr+=1)
    {
        auto lea=(*(WORD*)_addr);
        if(lea!=0x8d4c&&lea!=0x8d48)
            continue;
        
        auto offset=*(DWORD*)(_addr+3);
        auto refaddr=(offset)+_addr+7;
        if(refaddr==addr)
            return _addr;
    }
    
    return 0;
}

bool InsertPPSSPPHLEHooks()
{
    auto functions=std::vector<PPSSPPFunction>{
        
//https://github.com/hrydgard/ppsspp/blob/master/Core/HLE/sceCcc.cpp
            { "sceCccStrlenSJIS",  GETARG1, USING_STRING,  0, "sceCccStrlenSJIS(" } ,
            { "sceCccStrlenUTF8",  GETARG1, CODEC_UTF8|USING_STRING,    0, "sceCccStrlenUTF8(" } ,
            { "sceCccStrlenUTF16", GETARG1, CODEC_UTF16|USING_STRING, 0, "sceCccStrlenUTF16(" } ,

            { "sceCccSJIStoUTF8",  GETARG3, USING_STRING,    0, "sceCccSJIStoUTF8(" } ,
            { "sceCccSJIStoUTF16", GETARG3, USING_STRING,  0, "sceCccSJIStoUTF16(" } ,
            { "sceCccUTF8toSJIS",  GETARG3, CODEC_UTF8|USING_STRING,    0, "sceCccUTF8toSJIS(" } ,
            { "sceCccUTF8toUTF16", GETARG3, CODEC_UTF8|USING_STRING,    0, "sceCccUTF8toUTF16(" } ,
            { "sceCccUTF16toSJIS", GETARG3, CODEC_UTF16|USING_STRING, 0, "sceCccUTF16toSJIS(" } ,
            { "sceCccUTF16toUTF8", GETARG3, CODEC_UTF16|USING_STRING, 0, "sceCccUTF16toUTF8(" } ,

//https://github.com/hrydgard/ppsspp/blob/master/Core/HLE/sceFont.cpp
            { "sceFontGetCharInfo",              GETARG2, CODEC_UTF16, GETARG1, "sceFontGetCharInfo(" } ,
            { "sceFontGetShadowInfo",            GETARG2, CODEC_UTF16, GETARG1, "sceFontGetShadowInfo("} ,
            { "sceFontGetCharImageRect",         GETARG2, CODEC_UTF16, GETARG1, "sceFontGetCharImageRect(" } ,
            { "sceFontGetShadowImageRect",       GETARG2, CODEC_UTF16, GETARG1, "sceFontGetShadowImageRect(" } ,
            { "sceFontGetCharGlyphImage",        GETARG2, CODEC_UTF16, GETARG1, "sceFontGetCharGlyphImage(" } ,
            { "sceFontGetCharGlyphImage_Clip",   GETARG2, CODEC_UTF16, GETARG1, "sceFontGetCharGlyphImage_Clip(" } ,
            { "sceFontGetShadowGlyphImage",      GETARG2, CODEC_UTF16, GETARG1, "sceFontGetShadowGlyphImage(" } ,
            { "sceFontGetShadowGlyphImage_Clip", GETARG2, CODEC_UTF16, GETARG1, "sceFontGetShadowGlyphImage_Clip(" } ,

//https://github.com/hrydgard/ppsspp/blob/master/Core/HLE/sceKernelInterrupt.cpp
            { "sysclib_strcat", GETARG2, USING_STRING, 0, "Untested sysclib_strcat(" } ,
            { "sysclib_strcpy", GETARG2, USING_STRING, 0, "Untested sysclib_strcpy(" } ,
            { "sysclib_strlen", GETARG1, USING_STRING, 0, "Untested sysclib_strlen(" }

            // Disabled as I am not sure how to deal with the source string
            //, { "sceCccEncodeSJIS", 2, USING_STRING, 0, "sceCccEncodeSJIS(" }
            //, { "sceCccEncodeUTF8", 2, CODEC_UTF8,   0, "sceCccEncodeUTF8(" }
            //, { "sceCccEncodeUTF16", 2, CODEC_UTF16, 0, "sceCccEncodeUTF16(" }
            //, { "sysclib_strcmp", 2, USING_STRING, 0, "Untested sysclib_strcmp(" }
        };
    auto succ=false;
    for (auto&& function :functions) {
        auto addr = MemDbg::findBytes(function.pattern, ::strlen(function.pattern), processStartAddress, processStopAddress);
        if(!addr)continue;
        #ifndef _WIN64
        addr=MemDbg::findPushAddress(addr, processStartAddress, processStopAddress);
        if(!addr)continue;
        addr=SafeFindEnclosingAlignedFunction(addr, 0x200);
        #else
        addr=findleaaddr(addr, processStartAddress, processStopAddress);
        
        if(!addr)continue;
         
        BYTE sig1[]={
            0xCC,
            0x48,0x89,XX,0x24,XX,
        };
        
        BYTE sig2[]={
            0xC3,
            0x48,0x89,XX,0x24,XX,
        };
        BYTE sig3[]={
            0xCC,
            0x89,XX,0x24,XX,
        };
        BYTE sig4[]={
            0xC3,
            0x89,XX,0x24,XX,
        };
        int idx=0;
        uintptr_t maxaddr=0;
        for(auto sig:{sig1,sig2,sig3,sig4})
        {
            idx+=1;
            maxaddr=max(maxaddr,reverseFindBytes(sig,(idx>2)?5:6,addr-0x500,addr,1,true));
        }
        maxaddr=max(maxaddr,MemDbg::findEnclosingAlignedFunction_strict(addr,0x500));

        addr=maxaddr;
        #endif
        
        if(!addr)continue;
        HookParam hp;
        hp.address=addr;
        hp.type = function.hookType;
        hp.offset=function.argIndex;
        hp.split = function.hookSplit;
        if (hp.split)hp.type |= USING_SPLIT;
        succ|=NewHook(hp, function.hookName);
    }
    return succ;
}
#if 0
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
                #ifndef _WIN64
                // PPSSPP 1.8.0 compiles jal to sub dword ptr [ebp+0x360],??
				memcpy(spDefault.pattern, Array<BYTE>{ 0x83, 0xAD, 0x60, 0x03, 0x00, 0x00 }, spDefault.length = 6);
                #else
				// PPSSPP 1.8.0 compiles jal to sub dword ptr [r14+0x360],??
				memcpy(spDefault.pattern, Array<BYTE>{ 0x41, 0x83, 0xae, 0x60, 0x03, 0x00, 0x00 }, spDefault.length = 7);
                #endif
				spDefault.offset = 0;
				spDefault.minAddress = 0;
				spDefault.maxAddress = -1ULL;
				spDefault.padding = (uintptr_t)probe - 0x8000000;
				spDefault.hookPostProcessor = [](HookParam& hp)
				{
					hp.type |= NO_CONTEXT | USING_SPLIT | SPLIT_INDIRECT;
                    #ifndef _WIN64
                    hp.split = get_reg(regs::ebp);
					hp.split_index =get_reg(regs::eax); // this is where PPSSPP 1.8.0 stores its return address stack
                    #else
					hp.split = get_reg(regs::r14);
					hp.split_index = -8; // this is where PPSSPP 1.8.0 stores its return address stack
                    #endif
				};
			}
			probe += info.RegionSize;
		}
	}
	return found;
}
#endif
uintptr_t getDoJitAddress() {
    #ifndef _WIN64
    auto string1="Jump target too far away, needs indirect register";
    auto string2="Jump target too far away, needs force5Bytes = true";
    auto addr1 = MemDbg::findBytes(string1, ::strlen(string1), processStartAddress, processStopAddress);
    auto addr2 = MemDbg::findBytes(string2, ::strlen(string2), processStartAddress, processStopAddress);
     
    if(addr1==0||addr2==0)return 0;
    //都是被push两次，但是都是第一个
    addr1=MemDbg::findPushAddress(addr1, processStartAddress, processStopAddress);
    addr2=MemDbg::findPushAddress(addr2, processStartAddress, processStopAddress);
    if(addr1==0||addr2==0)return 0;
    addr1=MemDbg::findEnclosingAlignedFunction_strict(addr1,0x100);
    addr2=MemDbg::findEnclosingAlignedFunction_strict(addr2,0x100);
    if(addr1==0||addr2==0||addr1!=addr2)return 0;
    auto xrefs=findxref_reverse_checkcallop(addr1,processStartAddress,processStopAddress,0xe8);
    if(xrefs.size()<28)return 0;
    addr1=MemDbg::findEnclosingAlignedFunction_strict(xrefs[xrefs.size()-1-3],0x400);
    addr2=MemDbg::findEnclosingAlignedFunction_strict(xrefs[xrefs.size()-1-4],0x400);
    if(addr1==0||addr2==0||addr1!=addr2)return 0; 
    return addr1;
    #else
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
	else
	{
		 
		auto DoJitSig2 = "C7 83 ?? 0? 00 00 11 00 00 00 F6 83 ?? 0? 00 00 01 ?? ?? ?? ?? ?? ?? ?? C7 83 ?? 0? 00 00 E4 00 00 00";
    	first=find_pattern(DoJitSig2,processStartAddress,processStopAddress); 
	 	if (first) {
			first=MemDbg::findEnclosingAlignedFunction_strict(first,0x400);
			return first;
		}
	}
    #endif
    return 0;
}

namespace ppsspp{

bool checkiscurrentgame(const emfuncinfo& em){
    auto wininfos=get_proc_windows();
    for(auto&& info:wininfos){
        if(info.title.find(acastw(em._id))!=info.title.npos)return true;
    }
    return false;
}
std::unordered_set<uintptr_t>breakpoints;

bool hookPPSSPPDoJit(){
	auto DoJitPtr=getDoJitAddress();
   if(DoJitPtr==0)return false;
   HookParam hp;
   hp.address=DoJitPtr;//Jit::DoJit
   ConsoleOutput("DoJitPtr %p",DoJitPtr);
   hp.user_value=(uintptr_t)new uintptr_t;
   hp.text_fun=[](hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
        auto em_address=stack->THISCALLARG1;
        
        *(uintptr_t*)(hp->user_value)=em_address;

		HookParam hpinternal;
		hpinternal.user_value=hp->user_value;
		hpinternal.address=stack->retaddr;
		hpinternal.text_fun=[](hook_stack* stack, HookParam* hp, uintptr_t* data, uintptr_t* split, size_t* len){
            auto em_address=*(uintptr_t*)(hp->user_value);
			[&](){
                auto ret=stack->RETADDR;
                if(breakpoints.find(ret)!=breakpoints.end())return;
                breakpoints.insert(ret);

                jitaddraddr(em_address,ret,JITTYPE::PPSSPP);

                if(emfunctionhooks.find(em_address)==emfunctionhooks.end())return;
                if(!(checkiscurrentgame(emfunctionhooks.at(em_address))))return;
                
                auto op=emfunctionhooks.at(em_address);
                ConsoleOutput("jit function addr %p",ret);
                #ifndef _WIN64
                BYTE sig[]={
                    0x8b,XX2,//mov reg,[ebp-off]
                    0x8b,0xc6,//mov eax,esi
                    0x25,0xff,0xff,0xff,0x3f,//and eax,0x3fffffff
                    0x89,XX,XX4,//mov [eax+base+off],reg

                };
                auto findbase=MemDbg::findBytes(sig,sizeof(sig),ret,ret+0x20);
                if(!findbase) 
                    findbase=MemDbg::findBytes(sig,sizeof(sig),ret-0x1000,ret+0x1000);
                if(!findbase)
                    ConsoleOutput("can't find emu_baseaddr"); 
                PPSSPP::x86_baseaddr=(*(DWORD*)(findbase+12))&0xffff0000;
                ConsoleOutput("x86 base addr %p",PPSSPP::x86_baseaddr);
                #endif
                HookParam hpinternal;
                hpinternal.address=ret;
                hpinternal.emu_addr=em_address;//用于生成hcode
                hpinternal.type=USING_STRING|NO_CONTEXT|BREAK_POINT|op.type;
                hpinternal.text_fun=(decltype(hpinternal.text_fun))op.hookfunc;
                hpinternal.filter_fun=(decltype(hpinternal.filter_fun))op.filterfun;
                hpinternal.argidx=op.argidx;
                hpinternal.padding=op.padding;
                hpinternal.jittype=JITTYPE::PPSSPP;
                NewHook(hpinternal,op._id);
            }();
            delayinsertNewHook(em_address);
		};
		static auto once=NewHook(hpinternal,"DoJitPtrRet");
   };
   
  return NewHook(hp,"PPSSPPDoJit");
}
}
bool InsertPPSSPPcommonhooks()
{
  
  auto succ=InsertPPSSPPHLEHooks();
  succ|=ppsspp::hookPPSSPPDoJit();
  return succ;
}