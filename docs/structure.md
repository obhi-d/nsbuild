
# Structure

.
+ sources
   +--CMakeLists.txt [generated in first run]
   +--Frameworks
      +--externM
         +--cmake/CMakeLists.txt
+ out
  +--dl
  +--build [x64-debug, arm-release]
     +--CMakeLists.txt [generic file with all module includes]
     +--bld
     |  +--externM [is build directory for externM]
     |     +--CMakeLists.txt [has find_package]
     +--ext
     |  +--externM
     |     +--CMakeLists.txt [has add_subdir]
     +--gen
     |  +--externM [generated code for externM]
     |     +--local [Local files]
     +--xpb [is build directory for external package]
     |  +--externM [is build directory external package for externM]
     +--sdk [is install dir]
     |  +--externM [is install dir for sdk]
     +--rt    [runtime output directory]
     +--cache

