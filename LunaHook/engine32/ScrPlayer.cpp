#include"ScrPlayer.h"
 
bool ScrPlayer::attach_function() {
    auto func=MemDbg::findCallerAddress((ULONG)GetGlyphOutlineA,0x90909090,processStartAddress,processStopAddress);
    if(func==0)return false;
    func+=4;
    BYTE check[]={
      0x83,0xf8,0x20,
      0x74,XX,
      0x3d,0x40,0x81,0x00,0x00,
      0x74,XX
    };
    auto addr=MemDbg::findBytes(check,sizeof(check),processStartAddress,processStopAddress);
    if(addr==0)return false;
    addr=MemDbg::findEnclosingAlignedFunction(addr);
    if(addr==0)return false;
    if(addr!=func)return false;
    HookParam hp;
    hp.address=func;
    hp.offset=get_stack(5);
    //会把多行分开导致翻译不对。
    hp.type=USING_STRING;//|EMBED_ABLE|EMBED_BEFORE_SIMPLE|EMBED_AFTER_NEW|EMBED_DYNA_SJIS;
    //hp.hook_font=F_GetGlyphOutlineA;
    hp.filter_fun=[](LPVOID data, size_t* size, HookParam*) {
                static int idx=0;
                idx+=1;//这个函数总是连续被调用两次，一个绘制上层文字，一个绘制阴影。
                return bool(idx%2);
            };
    return NewHook(hp,"ScrPlayer");
} 