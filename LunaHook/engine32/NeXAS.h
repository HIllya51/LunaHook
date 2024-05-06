

class NeXAS:public ENGINE{
    public:
    NeXAS(){
        
        check_by=CHECK_BY::FILE_ALL;
        check_by_target=check_by_list{L"*.pac",L"Thumbnail.pac"};
        // jichi 6/3/2014: AMUSE CRAFT and SOFTPAL
    // Selectively insert, so that lstrlenA can still get correct text if failed
    //if (Util::CheckFile(L"dll\\resource.dll") && Util::CheckFile(L"dll\\pal.dll") && InsertAmuseCraftHook())
    //  return true;
    }; 
     bool attach_function(); 
};
