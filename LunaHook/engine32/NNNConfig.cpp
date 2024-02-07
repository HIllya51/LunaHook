#include"NNNConfig.h" 
bool NNNConfig::attach_function() {    
    //blackcyc
			//开头有一些究极重复的，没办法
			//夢幻廻廊
			const BYTE bytes[] = {
				0x68,0xE8,0x03,0x00,0x00,0x6a,0x00,
				
			};
			auto addr = MemDbg::findBytes(bytes, sizeof(bytes), processStartAddress, processStopAddress);
			if (addr == 0)return false;

			addr = addr + sizeof(bytes);
			for (int i = 0; i < 5; i++) {
				if (*(BYTE*)addr == 0xe8) {
					addr += 1;
					break;
				}
				addr += 1;
			}
			uintptr_t offset = *(uintptr_t*)(addr);
			uintptr_t funcaddr = offset + addr + 4; 
			const BYTE check[] = { 0x83 ,0xEC ,0x1C };
			auto checkoffset = MemDbg::findBytes(check, sizeof(check), funcaddr, funcaddr +0x20);
			
			ConsoleOutput("%p %p %p %d", addr, offset, funcaddr,checkoffset);
			if (checkoffset == 0)offset = get_stack(5);
			else offset = get_stack(6);
			HookParam hp;
			hp.address = funcaddr;

			hp.offset = offset;
			 
			hp.type = USING_STRING ;
			return NewHook(hp, "NNNhook");
} 