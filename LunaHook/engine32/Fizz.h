

class Fizz : public ENGINE
{
public:
    Fizz()
    {
        check_by = CHECK_BY::CUSTOM;
        check_by_target = [&]()
        {
            auto _ = Util::CheckFile(L"data.gsp") && Util::CheckFile(L"Image*.gsp") && Util::CheckFile(L"bgm*.gsp") && Util::CheckFile(L"se.gsp");
            if (Util::CheckFile(L"voice/*.gsp"))
            {
                typex = 1;
                return true;
            }
            if (wcscmp(processName_lower, L"gsd.exe") == 0)
            {
                typex = 2;
                return true;
            }
            return false;
        };
    };
    bool attach_function();
    int typex = 0;
};