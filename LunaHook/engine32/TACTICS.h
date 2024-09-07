/*
FILEVERSION    1,0,0,1
PRODUCTVERSION 1,0,0,1
FILEFLAGSMASK  0x3F
FILEFLAGS      0x0
FILEOS         VOS_NT_WINDOWS32
FILETYPE       VFT_APP
FILESUBTYPE    0x0
{
  BLOCK "StringFileInfo"
  {
    BLOCK "041104b0"
    {
      VALUE "CompanyName",       "ﾀｸﾃｨｸｽ"
      VALUE "FileDescription",   "すずがうたう日"
      VALUE "FileVersion",       "1.00"
      VALUE "InternalName",      "SUZUUTA"
      VALUE "LegalCopyright",    "Copyright (C) 1999"
      VALUE "OriginalFilename",  "SUZU.EXEC"
      VALUE "ProductName",       "SuzuGaUtauHi"
      VALUE "ProductVersion",    "1.00"
    }
  }
  BLOCK "VarFileInfo"
  {
    VALUE "Translation", 0x411, 1200
  }
}

*/
class TACTICS : public ENGINE
{
public:
    TACTICS()
    {
        check_by = CHECK_BY::RESOURCE_STR;
        check_by_target = L"ﾀｸﾃｨｸｽ";
    };
    bool attach_function();
};