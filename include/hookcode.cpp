#include "hookcode.h"

namespace
{
	std::optional<HookParam> ParseRCode(std::wstring RCode)
	{
		std::wsmatch match;
		HookParam hp;
		hp.type |= DIRECT_READ;

		// {S|Q|V|M}
		switch (RCode[0])
		{
		case L'S':
			break;
		case L'Q':
			hp.type |= CODEC_UTF16;
			break;
		case L'U':
			hp.type |= CODEC_UTF32;
			break;
		case L'V':
			hp.type |= CODEC_UTF8;
			break;
		default:
			return {};
		}
		RCode.erase(0, 1);

		// [codepage#]
		if (std::regex_search(RCode, match, std::wregex(L"^([0-9]+)#")))
		{
			hp.codepage = std::stoi(match[1]);
			RCode.erase(0, match[0].length());
		}

		// @addr
		if (!std::regex_match(RCode, match, std::wregex(L"@([[:xdigit:]]+)"))) return {};
		hp.address = std::stoull(match[1], nullptr, 16);
		return hp;
	}

	std::optional<HookParam> ParseHCode(std::wstring HCode)
	{
		std::wsmatch match;
		HookParam hp;

		// {A|B|W|H|S|Q|V|M}
		switch (HCode[0])
		{
		case L'A':
			hp.type |= CODEC_ANSI_BE;
			break;
		case L'B':
			//ANSI LE
			break;
		case L'W':
			hp.type |= CODEC_UTF16;
			break;
		case L'I':
			hp.type |= CODEC_UTF32;
			break;
		case L'S':
			hp.type |= USING_STRING;
			break;
		case L'Q':
			hp.type |= USING_STRING | CODEC_UTF16;
			break;
		case L'U':
			hp.type |= USING_STRING | CODEC_UTF32;
			break;
		case L'V':
			hp.type |= USING_STRING | CODEC_UTF8;
			break;
		default:
			return {};
		}
		HCode.erase(0, 1);

		if (hp.type & USING_STRING)
		{
			if (HCode[0] == L'F')
			{
				hp.type |= FULL_STRING;
				HCode.erase(0, 1);
			}

		}

		// [N]
		if (HCode[0] == L'N')
		{
			hp.type |= NO_CONTEXT;
			HCode.erase(0, 1);
		}

		// [codepage#]
		if (std::regex_search(HCode, match, std::wregex(L"^([0-9]+)#")))
		{
			hp.codepage = std::stoi(match[1]);
			HCode.erase(0, match[0].length());
		}

		// [padding+]
		if (std::regex_search(HCode, match, std::wregex(L"^([[:xdigit:]]+)\\+")))
		{
			hp.padding = std::stoull(match[1], nullptr, 16);
			HCode.erase(0, match[0].length());
		}

		auto ConsumeHexInt = [&HCode]
		{
			size_t size = 0;
			int value = 0;
			try { value = std::stoi(HCode, &size, 16); } catch (std::invalid_argument) {}
			HCode.erase(0, size);
			return value;
		};

		// data_offset
		hp.offset = ConsumeHexInt();

		// [*deref_offset1]
		if (HCode[0] == L'*')
		{
			hp.type |= DATA_INDIRECT;
			HCode.erase(0, 1);
			hp.index = ConsumeHexInt();
		}

		// [:split_offset[*deref_offset2]]
		if (HCode[0] == L':')
		{
			hp.type |= USING_SPLIT;
			HCode.erase(0, 1);
			hp.split = ConsumeHexInt();

			if (HCode[0] == L'*')
			{
				hp.type |= SPLIT_INDIRECT;
				HCode.erase(0, 1);
				hp.split_index = ConsumeHexInt();
			}
		}

		// @addr[:module[:func]]
		if (!std::regex_match(HCode, match, std::wregex(L"^@([[:xdigit:]]+)(:.+?)?(:.+)?"))) return {};
		hp.address = std::stoull(match[1], nullptr, 16);
		if (match[2].matched)
		{
			hp.type |= MODULE_OFFSET;
			wcsncpy_s(hp.module, match[2].str().erase(0, 1).c_str(), MAX_MODULE_SIZE - 1);
		}
		if (match[3].matched)
		{
			hp.type |= FUNCTION_OFFSET;
			std::wstring func = match[3];
			strncpy_s(hp.function, std::string(func.begin(), func.end()).erase(0, 1).c_str(), MAX_MODULE_SIZE - 1);
		}

		// ITH has registers offset by 4 vs AGTH: need this to correct
		if (hp.offset < 0) hp.offset -= 4;
		if (hp.split < 0) hp.split -= 4;

		return hp;
	}

	std::optional<HookParam> ParseECode(std::wstring code)
	{ 
		auto idx=code.find(L'H');
		if(idx==code.npos)
		{
			idx=code.find(L'B');

			if(idx==code.npos)
			return {};
		}
		auto hpo=ParseHCode(code.substr(idx+1));
		code=code.substr(0,idx);
		if(hpo.has_value()==false)return {};
		auto hp=hpo.value();
		hp.type|=EMBED_ABLE;

		if(code[0]==L'D')
		{
			hp.type|=EMBED_DYNA_SJIS;
			code.erase(0,1);
		}
		if(code[0]==L'S')
		{
			code.erase(0,1);
			hp.type|=EMBED_BEFORE_SIMPLE;

			if(code[0]==L'N')
				hp.type|=EMBED_AFTER_NEW;
			else if(code[0]==L'O')
				hp.type|=EMBED_AFTER_OVERWRITE;
			else
				return {};
			code.erase(0,1);
		}
		if(code.size())return {};
		return hp;
			 
	}
	std::wstring HexString(int64_t num)
	{
		if (num < 0) return FormatString(L"-%I64X", -num);
		return FormatString(L"%I64X", num);
	}

	std::wstring GenerateRCode(HookParam hp)
	{
		std::wstring RCode = L"R";
		
		if(hp.type&CODEC_UTF16)
			RCode += L'Q';
		else if (hp.type&CODEC_UTF32)
			RCode += L'U';
		else if (hp.type&CODEC_UTF8)
			RCode += L'V';
		else
		{
			RCode += L'S';
			if (hp.codepage != 0) RCode += std::to_wstring(hp.codepage) + L'#';
		}

		RCode += L'@' + HexString(hp.address);

		return RCode;
	}

	std::wstring GenerateHCode(HookParam hp, DWORD processId)
	{
		std::wstring HCode;
		if(hp.type&EMBED_ABLE)
		{
			HCode +=L"E"; 
			
			if (hp.hook_before || hp.hook_after) 
				HCode += L'X'; 
			else
			{
				if(hp.type&EMBED_DYNA_SJIS)
					HCode+=L"D";
				if(hp.type&EMBED_BEFORE_SIMPLE)
					HCode+=L"S";

				if(hp.type&EMBED_AFTER_NEW)
					HCode+=L"N";
				else if(hp.type&EMBED_AFTER_OVERWRITE)
					HCode+=L"O";
			}
			 
		}
		if(hp.type&BREAK_POINT)
			HCode+=L"B";
		else
			HCode += L"H";

		if (hp.text_fun || hp.filter_fun) 
			HCode += L'X';
		else
		{
			if(hp.type & USING_STRING)
			{
				if(hp.type&CODEC_UTF16)
					HCode += L'Q';
				else if(hp.type&CODEC_UTF8)
					HCode += L'V';
				else if(hp.type&CODEC_UTF32)
					HCode += L'U';
				else 
					HCode += L'S';
			}
			else
			{
				if(hp.type&CODEC_UTF16)
					HCode += L'W';
				else if(hp.type&CODEC_UTF32)
					HCode += L'I';
				else if (hp.type & CODEC_ANSI_BE) 
					HCode += L'A';
				else 
					HCode += L'B';
			}

		}
		
		if (hp.type & FULL_STRING) HCode += L'F';

		if (hp.type & NO_CONTEXT) HCode += L'N';

		if (hp.codepage != 0 && !(hp.type & CODEC_UTF8) ) HCode += std::to_wstring(hp.codepage) + L'#';

		if (hp.padding) HCode += HexString(hp.padding) + L'+';

		if (hp.offset < 0) hp.offset += 4;
		if (hp.split < 0) hp.split += 4;

		HCode += HexString(hp.offset);
		if (hp.type & DATA_INDIRECT) HCode += L'*' + HexString(hp.index);
		if (hp.type & USING_SPLIT) HCode += L':' + HexString(hp.split);
		if (hp.type & SPLIT_INDIRECT) HCode += L'*' + HexString(hp.split_index);
	
		

		// Attempt to make the address relative
		if (processId && !(hp.type & MODULE_OFFSET))
			if (AutoHandle<> process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId))
				if (MEMORY_BASIC_INFORMATION info = {}; VirtualQueryEx(process, (LPCVOID)hp.address, &info, sizeof(info)))
					if (auto moduleName = GetModuleFilename(processId, (HMODULE)info.AllocationBase))
					{
						hp.type |= MODULE_OFFSET;
						hp.address -= (uint64_t)info.AllocationBase;
						wcsncpy_s(hp.module, moduleName->c_str() + moduleName->rfind(L'\\') + 1, MAX_MODULE_SIZE - 1);
					}

		HCode += L'@' + HexString(hp.address);
		if (hp.type & MODULE_OFFSET) HCode += L':' + std::wstring(hp.module);
		if (hp.type & FUNCTION_OFFSET) HCode += L':' + StringToWideString(hp.function);

		return HCode;
	}
}

namespace HookCode
{
	std::optional<HookParam> Parse(std::wstring code)
	{
		if (code[0] == L'/') code.erase(0, 1);
		code.erase(std::find(code.begin(), code.end(), L'/'), code.end()); // legacy/AGTH compatibility
		Trim(code);
		if (code[0] == L'R') return ParseRCode(code.erase(0, 1));
		else if (code[0] == L'B'||code[0] == L'H'){
			auto isbreakpoint=code[0] == L'B';
			auto hpo=ParseHCode(code.erase(0, 1));
			if(isbreakpoint && hpo.has_value()){
				hpo.value().type|=BREAK_POINT;
			}
			return hpo;
		} 
		else if (code[0] == L'E') return ParseECode(code.erase(0, 1));
		return {};
	}

	std::wstring Generate(HookParam hp, DWORD processId)
	{
		std::wstring HCode =L"";
		return HCode+=(hp.type & DIRECT_READ ? GenerateRCode(hp) : GenerateHCode(hp, processId));
	}

}
