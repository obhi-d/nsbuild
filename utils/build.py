

import os
import sys
import subprocess
from pathlib import Path

def main():

    sys.path.append(os.getcwd())
    os.environ["PATH"] += os.pathsep + os.getcwd()
    preset      = 'release'
    main_dir    = os.path.dirname(os.path.realpath(__file__))
    install_dir = main_dir #os.path.join(main_dir, "..", "building")
    preset_dir  = os.path.join(main_dir, preset)
    source_dir  = os.path.join(main_dir, "..", "source")

    if (len(sys.argv) <= 1) or sys.argv[1] != "--regen":
        
      Path(preset_dir).mkdir(parents=True, exist_ok=True)
      p = subprocess.call(["cmake", '-S', main_dir, '--preset', preset], stdout=subprocess.PIPE,
                           cwd=preset_dir)
      if p != 0:
          print(" -- Failed in configure ")
          print(" -- On windows run: %VSINSTALL_DIR%\VC\Auxiliary\Build\vcvarsall.bat x86_amd64")
          return -1
    
      print("Configure finished")
        
      p = subprocess.call(["cmake", '--build', '.'], stdout=subprocess.PIPE,
                           cwd=preset_dir)
    
      if p != 0:
          print(" -- Failed in build ")
          return -1

      print("Build finished")
        
    p = subprocess.call(["cmake", '--install', '.', '--prefix', install_dir], stdout=subprocess.PIPE,
                         cwd=preset_dir)
    
    
    if p != 0:
        print(" -- Failed in install ")
        return -1
        
    print("Done installing here: " + install_dir)

    binary = os.path.join(install_dir, "nsbuild")
    binary_exe = os.path.join(install_dir, "nsbuild.exe")
    if os.path.exists(binary):
        p = subprocess.call([binary, '--source', source_dir], stdout=subprocess.PIPE,
                            cwd=install_dir)
    elif os.path.exists(binary_exe):
        p = subprocess.call([binary_exe, '--source', source_dir], stdout=subprocess.PIPE,
                            cwd=install_dir)
    else:
        print(" -- Binary not found. ")
        return -1
    
    if p != 0:
        print(" -- Main file generation failed. ")
        return -1
        
    print("Done generating main file. First build will build all external modules.")
    
if __name__ == "__main__":
    main()
