#pragma once

// texthook/const.h
// 8/23/2013 jichi
// Branch: ITH/common.h, rev 128

enum { STRING = 12, MESSAGE_SIZE = 500, PIPE_BUFFER_SIZE = 50000, SHIFT_JIS = 932, MAX_MODULE_SIZE = 120, PATTERN_SIZE = 30, HOOK_NAME_SIZE = 60, FIXED_SPLIT_VALUE = 0x10001 ,
HOOKCODE_LEN=500};
enum WildcardByte { XX = 0x11 };

enum HostCommandType { 
  HOST_COMMAND_NEW_HOOK,
  HOST_COMMAND_REMOVE_HOOK, 
  HOST_COMMAND_FIND_HOOK, 
  HOST_COMMAND_MODIFY_HOOK, 
  HOST_COMMAND_HIJACK_PROCESS, 
  HOST_COMMAND_DETACH 
};

enum HostNotificationType { 
  HOST_NOTIFICATION_TEXT, 
  HOST_NOTIFICATION_NEWHOOK, 
  HOST_NOTIFICATION_FOUND_HOOK, 
  HOST_NOTIFICATION_RMVHOOK,
  HOST_NOTIFICATION_INSERTING_HOOK,
  HOST_SETTEXTTHREADTYPE
};

enum HookParamType : uint64_t
{
  //默认为CODEC_ANSI_LE&USING_CHAR
  //若使用了text_fun|hook_before，会改为默认USING_STRING，这时若其实是USING_CHAR，需标明USING_STRING
  CODEC_ANSI_LE = 0,
  CODEC_ANSI_BE = 0x4,
	CODEC_UTF8 = 0x100,
	CODEC_UTF16 = 0x2, 
  CODEC_UTF32=0x1000000,

  USING_CHAR =0x2000000,//text_fun!=nullptr && (CODE_ANSI_BE||CODE_UTF16)
	USING_STRING = 0x1,
  
	DATA_INDIRECT = 0x8,
	USING_SPLIT = 0x10, // use ctx2 or not
	SPLIT_INDIRECT = 0x20,
	MODULE_OFFSET = 0x40, // address is relative to module
	FUNCTION_OFFSET = 0x80, // address is relative to function
	NO_CONTEXT = 0x200,
	HOOK_EMPTY = 0x400,
	FIXING_SPLIT = 0x800,
	DIRECT_READ = 0x1000, // /R read code instead of classic /H hook code
	FULL_STRING = 0x2000,
	KNOWN_UNSTABLE = 0x20000,
	EMBED_ABLE=0x40000,
	EMBED_DYNA_SJIS=0x80000,
	EMBED_BEFORE_SIMPLE=0x200000,
	EMBED_AFTER_NEW=0x400000,  
	EMBED_AFTER_OVERWRITE=0x800000,
  EMBED_CODEC_UTF16=0x4000000,

  BREAK_POINT=0x8000000
};


enum HookFontType : unsigned
{
  F_CreateFontA=0x1,
  F_CreateFontW=0x2,
  F_CreateFontIndirectA=0x4,
  F_CreateFontIndirectW=0x8,
  F_GetGlyphOutlineA=0x10,
  F_GetGlyphOutlineW=0x20,
  F_GetTextExtentPoint32A=0x40,
  F_GetTextExtentPoint32W=0x80,
  F_GetTextExtentExPointA=0x100,
  F_GetTextExtentExPointW=0x200,
  //F_GetCharABCWidthsA=0x
  //F_GetCharABCWidthsW=0x
  F_TextOutA=0x400,
  F_TextOutW=0x800,
  F_ExtTextOutA=0x1000,
  F_ExtTextOutW=0x2000,
  F_DrawTextA=0x4000,
  F_DrawTextW=0x8000,
  F_DrawTextExA=0x10000,
  F_DrawTextExW=0x20000,
  F_CharNextA=0x40000,
  //F_CharNextW=0x
  //F_CharNextExA=0x
  //F_CharNextExW=0x
  F_CharPrevA=0x80000,
  //F_CharPrevW=0x
  F_MultiByteToWideChar=0x100000,
  F_WideCharToMultiByte=0x200000
};