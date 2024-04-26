import os, sys
import subprocess

rootDir = os.path.dirname(__file__)

    
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


    
if sys.argv[1]=='pack':
    os.chdir(os.path.join(rootDir, "scripts"))
    os.system(f"python pack.py pack")
else:
    installVCLTL()
    os.chdir(os.path.join(rootDir, "scripts"))
    if sys.argv[1]=='plg32':
        os.system(f"cmd /c buildplugin32.bat")
    elif sys.argv[1]=='plg64':
        os.system(f"cmd /c buildplugin64.bat")
    elif sys.argv[1]=='en':
        os.system(f"cmd /c builden.bat")
    elif sys.argv[1]=='zh':
        os.system(f"cmd /c buildzh.bat")
    elif sys.argv[1]=='xp':
        os.system(f"cmd /c build32xp.bat")