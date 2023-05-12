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
  GIT_TAG "v1.0.0"
  GIT_SHALLOW ON
)

FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG "10.0.0"
  GIT_SHALLOW ON
)

FetchContent_Declare(reproc
  GIT_REPOSITORY https://github.com/DaanDeMeyer/reproc.git
  GIT_TAG v14.2.4
  GIT_SHALLOW ON
)

FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
  GIT_SHALLOW ON
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
 "src/nspreset.cpp"  )

 add_custom_command(TARGET nsbuild POST_BUILD 
  COMMAND ${CMAKE_COMMAND} -E copy_if_different  
    ${CMAKE_CURRENT_SOURCE_DIR}/data/BuildConfig.hxx 
    ${CMAKE_CURRENT_SOURCE_DIR}/data/EnumStringHash.hxx 
    ${CMAKE_CURRENT_SOURCE_DIR}/data/FlagType.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/data/ModuleConfig.hxx    ${CMAKE_CURRENT_BINARY_DIR}
 )

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
install (FILES 
  "${CMAKE_CURRENT_LIST_DIR}/data/BuildConfig.hxx"
  "${CMAKE_CURRENT_LIST_DIR}/data/FlagType.hxx"
  "${CMAKE_CURRENT_LIST_DIR}/data/ModuleConfig.hxx" 
  "${CMAKE_CURRENT_LIST_DIR}/data/EnumStringHash.hxx"
  DESTINATION ./)
