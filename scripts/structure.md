
# Structure

- sources
   |--CMakeLists.txt [generated in first run]
   |--Frameworks
      |--externM
         |--cmk/CMakeLists.txt
- out
  |-- `build` [x64-debug, arm-release]
  |-- persistent 
      |--CMakeLists.txt
      |--module1/CMakeLists.txt
      |--moduleN/CMakeLists.txt
      |--externM/CMakeLists.txt
      |--externM/cmk/CMakeLists.txt
      |--externM/bld

## build
Main build directory
### CMakeLists.txt
- Includes all module builds
### externM/cmk/CMakeLists.txt
- This will contain the build instructions (FetchContent + add_subdirectory)
- Build will be driven in config time of main CMakeLists.txt
## source/Frameworks/External
External module framework
### externM/cmk/CMakeLists.txt
Optional CMakeLists.txt with build and install instructions to be used
to build a fetch module. This will be used instead of the main CMake file (if) present
in the source directory of the fetched code to build the fetched code. The CMake file
in the source directory can be included in this file if required.
