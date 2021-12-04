project(nsbuild)
cmake_minimum_required(VERSION 3.0)

include(ExternalProject)

ExternalProject_Add(neoscript
  GIT_REPOSITORY https://github.com/obhi-d/neoscript.git
  GIT_TAG origin/main
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/neoscript"
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/neoscript
  STEP_TARGETS build install 
  
  CMAKE_ARGS 
  	-DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/out/external/install
	-DNEOSCRIPT_BUILD_TESTS:BOOL=OFF
	-DNEOSCRIPT_HEADER_ONLY:BOOL=ON
)

ExternalProject_Add(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG origin/master
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/fmt"
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/fmt
  STEP_TARGETS build install 
  
  CMAKE_ARGS 
  	-DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/out/external/install
)

find_package(Python3 REQUIRED)


## Basic tests
add_executable(nsbuild
  "src/main.cpp"
 "src/nshelp.cpp" "src/nsbuild.cpp" "src/nscommon.cpp" "src/nsmodule.cpp" "src/nsbuild_registry.cpp" "include/nspython.h" "src/nspython.cpp" "include/nsmacros.h" "src/nsmacros.cpp" "src/nsparams.cpp" "src/nsglob.cpp")

target_include_directories(nsbuild PRIVATE 
	"${CMAKE_SOURCE_DIR}/include"
	"${CMAKE_SOURCE_DIR}/out/external/install/include"
  "${Python3_INCLUDE_DIS}"
)
target_compile_features(nsbuild PUBLIC cxx_std_20)


target_compile_options(nsbuild PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>")

target_link_libraries(nsbuild PRIVATE ${CMAKE_SOURCE_DIR}/out/external/install/lib/$<$<PLATFORM_ID:Linux>:lib>fmt$<$<CONFIG:DEBUG>:d>$<$<PLATFORM_ID:Linux>:a>$<$<PLATFORM_ID:Windows>:.lib>)
target_link_libraries(nsbuild PRIVATE ${Python3_LIBRARIES})
add_dependencies(nsbuild neoscript-install)
add_dependencies(nsbuild fmt-install)
target_compile_features(nsbuild PRIVATE cxx_std_20)
add_compile_options(nsbuild PRIVATE "/Zc:preprocessor")

