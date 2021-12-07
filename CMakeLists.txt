cmake_minimum_required(VERSION 3.0)

project(nsbuild VERSION 0.0.1 LANGUAGES C CXX)


include(FetchContent)

set(NEOSCRIPT_BUILD_TESTS OFF)
set(NEOSCRIPT_HEADER_ONLY ON)
set(FETCHCONTENT_BASE_DIR ${PROJECT_SOURCE_DIR}/external)

FetchContent_Declare(neoscript
  GIT_REPOSITORY https://github.com/obhi-d/neoscript.git
  GIT_TAG origin/main
)

FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG origin/master
)

FetchContent_Declare(reproc
  GIT_REPOSITORY https://github.com/DaanDeMeyer/reproc.git
  GIT_TAG 8aea3aef64c37a7ed0db759a7128a80c2540c808
)

FetchContent_MakeAvailable(neoscript fmt reproc)
find_package(Python3 REQUIRED)

## Basic tests
add_executable(nsbuild
  "src/main.cpp"
 "src/nshelp.cpp" 
 "src/nsbuild.cpp" 
 "src/nscommon.cpp" 
 "src/nsmodule.cpp" 
 "src/nsbuild_registry.cpp" 
 "include/nspython.h" 
 "src/nspython.cpp" 
 "include/nsmacros.h" 
 "src/nsmacros.cpp" 
 "src/nsparams.cpp" 
 "src/nsglob.cpp" 
 "src/nsframework.cpp")

target_include_directories(nsbuild PRIVATE 
	"${CMAKE_SOURCE_DIR}/include"
  "${Python3_INCLUDE_DIS}"
)
target_compile_features(nsbuild PUBLIC cxx_std_20)


target_compile_options(nsbuild PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>")
target_compile_options(nsbuild PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>")

target_link_libraries(nsbuild fmt)
target_link_libraries(nsbuild reproc)
target_link_libraries(nsbuild neoscript)

target_link_libraries(nsbuild PRIVATE ${Python3_LIBRARIES})
target_compile_features(nsbuild PRIVATE cxx_std_20)
