project(nsbuild)
cmake_minimum_required(VERSION 3.0)

set   (NSBUILD_BISON            "bison"                                               CACHE STRING "Bison execuatable")
set   (NSBUILD_FLEX             "flex"                                                CACHE STRING "Flex executable"  )

include(ExternalProject)

ExternalProject_Add(neoscript
  GIT_REPOSITORY https://github.com/obhi-d/neoscript.git
  GIT_TAG origin/main
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/neoscript"
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/neoscript
  STEP_TARGETS install 
  
  CMAKE_ARGS 
  	-DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/out/external/install
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DCMAKE_CONFIGURATION_TYPES=${CMAKE_CONFIGURATION_TYPES}
	-DNEOSCRIPT_BUILD_TESTS:BOOL=OFF
	-DNEOSCRIPT_HEADER_ONLY:BOOL=ON
)



## Basic tests
add_executable(nsbuild
  "src/main.cpp"
 "src/nshelp.cpp" "src/nsbuild.cpp" "src/nscommon.cpp" "src/nsmodule.cpp" "src/nsbuild_registry.cpp")

target_include_directories(nsbuild PRIVATE 
	"${CMAKE_SOURCE_DIR}/include"
	"${CMAKE_SOURCE_DIR}/out/external/install/include"
)
add_dependencies(nsbuild neoscript-install)
target_compile_features(nsbuild PRIVATE cxx_std_20)
add_compile_options(nsbuild PRIVATE "/Zc:preprocessor")

