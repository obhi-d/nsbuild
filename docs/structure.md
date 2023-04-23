
# Structure

```
+ sources
   +--CMakeLists.txt [generated in first run]
   +--Frameworks
      +--externM
         +--cmake/CMakeLists.txt
+ out
  +--CMakeLists.txt
  +--dl
  +--build [x64-debug, arm-release]
     +--CMakeLists.txt -- includes all module CMakeLists.txt
     +--cmake
     |  +--externM 
     |     +--CMakeLists.txt [has find_package]
     +--extern
     |  +--externM
     |     +--CMakeLists.txt [has add_subdir]
     +--generated
     |  +--externM [generated code for externM]
     |     +--local [Local files]
     +--bld [is build directory for external package]
     |  +--main --> preset build directory
     |  +--externM --> Module build directory
     |  +--externM.dl --> Fetch build directory
     |  +--externM.ext --> extern build directory
     +--sdk [is install dir]
     |  +--externM [is install dir for sdk]
     +--rt    [runtime output directory]
     +--cache

```