
import os,sys
import shutil
import subprocess

rootDir = os.path.dirname(__file__)

vcltlFile = "https://github.com/Chuyu-Team/VC-LTL5/releases/download/v5.0.9/VC-LTL-5.0.9-Binary.7z"
vcltlFileName = "VC-LTL-5.0.9-Binary.7z"
def installVCLTL():
    os.makedirs(rootDir + "\\temp",exist_ok=True)
    os.chdir(rootDir + "\\temp")
    subprocess.run(f"curl -LO {vcltlFile}")
    subprocess.run(f"7z x {vcltlFileName} -oVC-LTL5")
    os.chdir("VC-LTL5")
    subprocess.run("cmd /c Install.cmd")
installVCLTL()
os.chdir(os.path.join(rootDir,'scripts'))
if int(sys.argv[1])==32:
    os.system('cmd /c build32.bat')
elif int(sys.argv[1])==64:
    os.system('cmd /c build64.bat')
else:
    os.system('cmd /c pack.bat')