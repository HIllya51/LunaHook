import os, sys, re
import subprocess

rootDir = os.path.dirname(__file__)
if len(sys.argv) and sys.argv[1] == "loadversion":
    os.chdir(rootDir)
    with open("CMakeLists.txt", "r", encoding="utf8") as ff:
        pattern = r"set\(VERSION_MAJOR\s*(\d+)\s*\)\nset\(VERSION_MINOR\s*(\d+)\s*\)\nset\(VERSION_PATCH\s*(\d+)\s*\)"
        match = re.findall(pattern, ff.read())[0]
        version_major, version_minor, version_patch = match
        versionstring = f"v{version_major}.{version_minor}.{version_patch}"
        print("version=" + versionstring)
        exit()
vcltlFile = "https://github.com/Chuyu-Team/VC-LTL5/releases/download/v5.0.9/VC-LTL-5.0.9-Binary.7z"
vcltlFileName = "VC-LTL-5.0.9-Binary.7z"


def installVCLTL():
    os.chdir(rootDir)
    if os.path.exists("temp"):
        return  # already installed
    os.makedirs(rootDir + "\\temp", exist_ok=True)
    subprocess.run(f"curl -Lo temp\\{vcltlFileName} {vcltlFile}")
    subprocess.run(f"7z x temp\\{vcltlFileName} -otemp\\VC-LTL5")
    subprocess.run("cmd /c temp\\VC-LTL5\\Install.cmd")


def build_langx(lang):
    with open("do.bat", "w") as ff:
        ff.write(
            rf"""
cmake -DBUILD_PLUGIN=OFF -DLANGUAGE={lang} ../CMakeLists.txt -G "Visual Studio 17 2022" -A win32 -T host=x86 -B ../build/x86_{lang}
cmake --build ../build/x86_{lang} --config Release --target ALL_BUILD -j 14

cmake -DBUILD_PLUGIN=OFF -DLANGUAGE={lang} ../CMakeLists.txt -G "Visual Studio 17 2022" -A x64 -T host=x64 -B ../build/x64_{lang}
cmake --build ../build/x64_{lang} --config Release --target ALL_BUILD -j 14

"""
        )
    os.system(f"cmd /c do.bat")


def build_langx_xp(lang):
    with open("do.bat", "w") as ff:
        ff.write(
            rf"""

cmake -DBUILD_PLUGIN=OFF -DWINXP=1 -DLANGUAGE={lang} ../CMakeLists.txt -G "Visual Studio 16 2019" -A win32 -T v141_xp -B ../build/x86_{lang}_xp
cmake --build ../build/x86_{lang}_xp --config Release --target ALL_BUILD -j 14
call dobuildxp.bat
"""
        )
    os.system(f"cmd /c do.bat")


if sys.argv[1] == "pack":
    os.chdir(os.path.join(rootDir, "scripts"))
    os.system(f"python pack.py pack")
else:
    installVCLTL()
    os.chdir(os.path.join(rootDir, "scripts"))
    if sys.argv[1] == "plg32":
        os.system(f"cmd /c buildplugin32.bat")
    elif sys.argv[1] == "plg64":
        os.system(f"cmd /c buildplugin64.bat")
    elif sys.argv[1] == "Release_English":
        build_langx("English")
    elif sys.argv[1] == "Release_Chinese":
        build_langx("Chinese")
    elif sys.argv[1] == "Release_Russian":
        build_langx("Russian")
    elif sys.argv[1] == "Release_TradChinese":
        build_langx("TradChinese")
    elif sys.argv[1] == "Release_English_winxp":
        build_langx_xp("English")
    elif sys.argv[1] == "Release_Chinese_winxp":
        build_langx_xp("Chinese")
    elif sys.argv[1] == "Release_Russian_winxp":
        build_langx_xp("Russian")
    elif sys.argv[1] == "Release_TradChinese_winxp":
        build_langx_xp("TradChinese")
