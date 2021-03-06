cmake_minimum_required(VERSION 3.0)

project(nsbuild VERSION 1.0.1 LANGUAGES C CXX)


include(FetchContent)

set(NEOSCRIPT_BUILD_TESTS OFF CACHE INTERNAL "")
set(NEOSCRIPT_HEADER_ONLY ON CACHE INTERNAL "")

set(REPROC++ ON CACHE INTERNAL "")
set(FMT_TEST OFF CACHE INTERNAL "")
set(JSON_BuildTests OFF CACHE INTERNAL "")

FetchContent_Declare(neoscript
  GIT_REPOSITORY https://github.com/obhi-d/neoscript.git
  GIT_TAG 00dbbf8945abdc437d43380f6406c14f8af49d15
)

FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG origin/master
)

FetchContent_Declare(reproc
  GIT_REPOSITORY https://github.com/DaanDeMeyer/reproc.git
  GIT_TAG 8aea3aef64c37a7ed0db759a7128a80c2540c808
)

FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG fec56a1a16c6e1c1b1f4e116a20e79398282626c
)

if(NOT neoscript_POPULATED)
  FetchContent_Populate(neoscript)
  add_subdirectory(${neoscript_SOURCE_DIR} ${neoscript_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

if(NOT fmt_POPULATED)
  FetchContent_Populate(fmt)
  add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

if(NOT reproc_POPULATED)
  FetchContent_Populate(reproc)
  add_subdirectory(${reproc_SOURCE_DIR} ${reproc_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

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
 "src/nsvars.cpp" 
 "src/nsglob.cpp" 
 "src/nsframework.cpp" 
 "src/nsprocess.cpp"
 "src/nscmake.cpp" 
 "src/nsbuildcmds.cpp" 
 "src/nsmedia.cpp" 
 "src/nsenums.cpp"  
 "src/nspreset.cpp")

target_include_directories(nsbuild PRIVATE 
	"${PROJECT_SOURCE_DIR}/include")
 
target_compile_features(nsbuild PUBLIC cxx_std_20)


target_compile_options(nsbuild PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>")
target_compile_options(nsbuild PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>")

target_link_libraries(nsbuild PRIVATE fmt)
target_link_libraries(nsbuild PRIVATE reproc++)
target_link_libraries(nsbuild PRIVATE neoscript)
target_link_libraries(nsbuild PRIVATE nlohmann_json::nlohmann_json)

install (TARGETS nsbuild RUNTIME DESTINATION ./)
