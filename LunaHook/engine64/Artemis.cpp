#include"Artemis.h"

bool InsertArtemisHook() {
		
		/*
		* Sample games:
		* https://vndb.org/v45247
		*/
		const BYTE bytes[] = {
			0xCC,                               // int 3
			0x40, 0x57,                         // push rdi          <- hook here
			0x48, 0x83, 0xEC, 0x40,             // sub rsp,40
			0x48, 0xC7, 0x44, 0x24, 0x30, XX4,  // mov qword ptr [rsp+30],FFFFFFFFFFFFFFFE
			0x48, 0x89, 0x5C, 0x24, 0x50        // mov [rsp+50],rbx
		};
 
		for (auto addr : Util::SearchMemory(bytes, sizeof(bytes), PAGE_EXECUTE, processStartAddress, processStopAddress)) {
			HookParam hp;
			hp.address = addr + 1;
			hp.offset=get_reg(regs::rdx);
			hp.type = USING_STRING | CODEC_UTF8 | NO_CONTEXT;
			ConsoleOutput("INSERT Artemis Hook ");
			return NewHook(hp, "Artemis");
		}

		ConsoleOutput("Artemis: pattern not found");
		return false;
	}
bool Artemis64() {
    
    const BYTE BYTES[] = {
      0x48,0x89,0x5C,0x24,0x20,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x60
      //__int64 __fastcall sub_14017A760(__int64 a1, char *a2, char **a3)
      //FLIP FLOP IO
    };
    auto addrs = Util::SearchMemory(BYTES, sizeof(BYTES), PAGE_EXECUTE_READ, processStartAddress, processStopAddress);
    for (auto addr : addrs) {
      char info[1000] = {};
      ConsoleOutput("InsertArtemis64Hook %p", addr);
      HookParam hp;
      hp.address = addr;
      hp.type = CODEC_UTF8 | USING_STRING|EMBED_ABLE|EMBED_BEFORE_SIMPLE|EMBED_AFTER_NEW;
      hp.offset=get_reg(regs::rdx);//rdx 
      return NewHook(hp, "Artemis64");
    }

    ConsoleOutput("InsertArtemis64Hook failed");
    return false;
} 
bool Artemis::attach_function() {
    bool b1=Artemis64();
    b1=InsertArtemisHook()||b1;
    return b1;
} 