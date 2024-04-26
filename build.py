import os, sys
import subprocess

rootDir = os.path.dirname(__file__)

vcltlFile = "https://github.com/Chuyu-Team/VC-LTL5/releases/download/v5.0.9/VC-LTL-5.0.9-Binary.7z"
vcltlFileName = "VC-LTL-5.0.9-Binary.7z"
availableTargets = ["32xp", "32", "64"]


def installVCLTL():
    os.chdir(rootDir)
    if os.path.exists("temp"):
        return  # already installed
    os.makedirs(rootDir + "\\temp", exist_ok=True)
    subprocess.run(f"curl -Lo temp\\{vcltlFileName} {vcltlFile}")
    subprocess.run(f"7z x temp\\{vcltlFileName} -otemp\\VC-LTL5")
    subprocess.run("cmd /c temp\\VC-LTL5\\Install.cmd")


target = sys.argv[1]
os.chdir(os.path.join(rootDir, "scripts"))
if target in availableTargets:
    installVCLTL()
    os.chdir(os.path.join(rootDir, "scripts"))
    os.system(f"cmd /c build{target}.bat")
else:
    if target == "pack":
        os.system(f"python pack.py pack")
    else:
        os.system(f"python pack.py pack_xp")
