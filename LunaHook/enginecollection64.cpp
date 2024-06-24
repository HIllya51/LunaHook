#include"engine64/PPSSPP.h"
#include"engine64/Godot.h"
#include"engine64/V8.h"
#include"engine64/mono.h"
#include"engine64/AGES7.h"
#include"engine64/pchooks.h"
#include"engine64/Artemis.h"
#include"engine64/KiriKiri.h"
#include"engine64/YOX.h"
#include"engine64/Suika2.h"
#include"engine64/livecaptions.h"
#include"engine64/CMVS.h"
#include"engine64/5pb.h"
#include"engine64/lucasystem.h"
#include"engine64/ENTERGRAM.h"
#include"engine64/TYPEMOON.h"
#include"engine64/Kincaid.h"
#include"engine64/LightVN.h"
#include"engine64/yuzusuyu.h"
#include"engine64/vita3k.h"
#include"engine64/rpcs3.h"
#include"engines/lua/lua51.h"
#include"engines/python/Renpy.h"
std::vector<ENGINE*> ignore_engines(){ return{ }; }
std::vector<ENGINE*> unsafe_check_atlast(){   return{ }; }

std::vector<ENGINE*> check_engines(){ 
    return  { 
        new Godot,
        new V8,
        new Renpy,
        new mono,
        new pchooks,
        new Artemis,
        new KiriKiri,
        new YOX,
        new lucasystem,
        new LightVN,
        new CMVS,
        new Suika2,
        new AGES7,
        new _5pb,
        new TYPEMOON,
        new ENTERGRAM,
        new yuzusuyu,
        new PPSSPPengine,
        new vita3k,
        new rpcs3,
        new Kincaid,
        new livecaptions,
        new lua51,
    };
     
}
