#pragma once
#include <string_view>

namespace cmake
{
static inline constexpr char k_main_preamble[] = R"_(

cmake_minimum_required(VERSION 3.25)

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project({0} VERSION {4} LANGUAGES C CXX)
set(nsbuild "{2}")

set(__nsbuild_app_options)
set(__nsbuild_console_app_options)
set(__nsbuild_root ${{CMAKE_CURRENT_LIST_DIR}})

find_package(Python REQUIRED COMPONENTS Interpreter)

if (NOT DEFINED nsplatform)

  set(nsplatform)
  set(__main_nsbuild_dir "${{CMAKE_CURRENT_LIST_DIR}}")

  if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(nsplatform "windows")
    set(__nsbuild_app_options WIN32)
  elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(nsplatform "linux")
  elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(nsplatform "macOS")
    set(__nsbuild_app_options MACOSX_BUNDLE)
  endif()

endif()


add_custom_target(nsbuild-check ALL
  COMMAND ${{nsbuild}} --check 
                         --preset="${{__nsbuild_preset}}" 
                         --cmake="${{CMAKE_COMMAND}}" 
                         --binary-dir="${{CMAKE_BINARY_DIR}}" 
                         --build-type=${{CMAKE_BUILD_TYPE}} 
                         --cpp-compiler-id="${{CMAKE_CXX_COMPILER}}" 
                         --c-compiler-id="${{CMAKE_C_COMPILER}}" 
                         --cpp-compiler-ver="${{CMAKE_CXX_COMPILER_VERSION}}" 
                         --generator="${{CMAKE_GENERATOR}}" 
                         --generator-instance="${{CMAKE_GENERATOR_INSTANCE}}" 
                         --generator-platform="${{CMAKE_GENERATOR_PLATFORM}}" 
                         --generator-toolset="${{CMAKE_GENERATOR_TOOLSET}}" 
                         --toolchain="${{CMAKE_TOOLCHAIN}}"  
                         --is-multiconfig=${{GENERATOR_IS_MULTI_CONFIG}} 
                         --platform=${{nsplatform}}
  WORKING_DIRECTORY ${{__main_nsbuild_dir}}
)

# set_property(
#   DIRECTORY
#   APPEND
#   PROPERTY CMAKE_CONFIGURE_DEPENDS
#   {1}/CMakeLists.txt
# )

## Compile commands

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

## Tests

set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
include(CTest)

add_subdirectory(${{CMAKE_CURRENT_LIST_DIR}}/{1}/${{__nsbuild_preset}}/{3} ${{CMAKE_CURRENT_LIST_DIR}}/{1}/${{__nsbuild_preset}}/{3}/main)

if(NOT TARGET nsbuild-clean)
    add_custom_target(nsbuild-clean
        COMMAND ${{nsbuild}} --clean 
                         --preset="${{__nsbuild_preset}}" 
                         --cmake="${{CMAKE_COMMAND}}" 
                         --binary-dir="${{CMAKE_BINARY_DIR}}" 
                         --build-type=${{CMAKE_BUILD_TYPE}} 
                         --cpp-compiler-id="${{CMAKE_CXX_COMPILER}}" 
                         --c-compiler-id="${{CMAKE_C_COMPILER}}" 
                         --cpp-compiler-ver="${{CMAKE_CXX_COMPILER_VERSION}}" 
                         --generator="${{CMAKE_GENERATOR}}" 
                         --generator-instance="${{CMAKE_GENERATOR_INSTANCE}}" 
                         --generator-platform="${{CMAKE_GENERATOR_PLATFORM}}" 
                         --generator-toolset="${{CMAKE_GENERATOR_TOOLSET}}" 
                         --toolchain="${{CMAKE_TOOLCHAIN}}"  
                         --is-multiconfig=${{GENERATOR_IS_MULTI_CONFIG}} 
                         --platform=${{nsplatform}}
        WORKING_DIRECTORY ${{__main_nsbuild_dir}}
    )
endif()

)_";

static inline constexpr char k_include_mods_preamble[] = R"_(

## Setup endianness
include(TestBigEndian)
TEST_BIG_ENDIAN(nsbuild_host_is_big_endian)

## Unity build setup
set(CMAKE_UNITY_BUILD {1})

## Cppcheck setup
set(__nsbuild_use_cppcheck {0})
if (__nsbuild_use_cppcheck)
  message(" -- nsbuild: cppcheck is enabled")
  find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
  if (CMAKE_CXX_CPPCHECK)
    list(
        APPEND CMAKE_CXX_CPPCHECK 
            "--enable=all"
            "--inconclusive"
            "--inline-suppr"
            "--suppressions-list=${{CMAKE_CURRENT_LIST_DIR}}/CppCheckSuppressions.txt"
    )
  endif()
else()
  set(CMAKE_CXX_CPPCHECK "")
endif()

set(__natvis_file {2})
if (MSVC AND __natvis_file)
  set(__natvis_file "${{__nsbuild_root}}/${{__natvis_file}}")
else()
  set(__natvis_file)
endif()

set(default_namespace_name "{3}")
set(macro_prefix "{4}")
set(file_prefix "{6}")
set(__nsbuild_data "{5}")

configure_file(${{__nsbuild_data}}/BuildConfig.hxx ${{CMAKE_CURRENT_LIST_DIR}}/BuildConfig.hpp)
configure_file(${{__nsbuild_data}}/FlagType.hxx ${{CMAKE_CURRENT_LIST_DIR}}/FlagType.hpp)
configure_file(${{__nsbuild_data}}/EnumStringHash.hxx ${{CMAKE_CURRENT_LIST_DIR}}/EnumStringHash.hpp)


)_";

static inline constexpr char k_target_include_directories[] =
    "target_include_directories(${{module_target}} {}  $<{}:{}>)";

static inline constexpr char k_find_package[] = "\nfind_package({} {} REQUIRED PATHS ${{{}_sdk_dir}} NO_DEFAULT_PATH)";

static inline constexpr char k_find_package_comp_start[] = "\nfind_package({} {} REQUIRED COMPONENTS ";
static inline constexpr char k_find_package_comp_end[]   = "PATHS ${{{}_sdk_dir}} NO_DEFAULT_PATH)";

static inline constexpr char k_write_dependency[] = R"_(

if(__module_pub_deps)
  add_dependencies(${module_target} ${__module_pub_deps})
endif()
if(__module_pub_link_libs)
  target_link_libraries(${module_target} PUBLIC ${__module_pub_link_libs})
endif()
if (__module_priv_deps)
  add_dependencies(${module_target} ${__module_priv_deps})
endif()
if (__module_priv_link_libs)
  target_link_libraries(${module_target} PRIVATE ${__module_priv_link_libs})
endif()
if(__module_ref_deps)
  add_dependencies(${module_target} ${__module_ref_deps})
endif()
)_";

static inline constexpr char k_begin_prebuild_steps[] = R"_(
# Prebuild target
set(module_prebuild_artifacts)
)_";

static inline constexpr char k_finalize_prebuild_steps[] = R"_(
# If we have prebuild artifacts

if (module_prebuild_artifacts)

add_custom_target(${module_target}.prebuild 
  DEPENDS ${module_prebuild_artifacts}
  COMMAND ${CMAKE_COMMAND} -E echo "Prebuilt step for ${module_name}"
)
add_dependencies(${module_target} ${module_target}.prebuild)
if(__module_pub_deps)
  add_dependencies(${module_target}.prebuild ${__module_pub_deps})
endif()
if (__module_priv_deps)
  add_dependencies(${module_target}.prebuild ${__module_priv_deps})
endif()
if(__module_ref_deps)
  add_dependencies(${module_target}.prebuild ${__module_ref_deps})
endif()

endif()
)_";

static inline constexpr char k_write_libs[] = R"_(
if (__module_priv_libs)
  target_link_libraries(${module_target} PRIVATE ${__module_priv_libs})
endif()
if (__module_pub_libs)
  target_link_libraries(${module_target} PUBLIC ${__module_pub_libs})
endif()
)_";

static inline constexpr char k_rt_locations[] = R"_(
  ARCHIVE_OUTPUT_DIRECTORY "${config_rt_dir}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${config_rt_dir}/lib"
  RUNTIME_OUTPUT_DIRECTORY "${config_rt_dir}/bin"
)_";

static inline constexpr char k_plugin_locations[] = R"_(
  ARCHIVE_OUTPUT_DIRECTORY "${{config_rt_dir}}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${{config_rt_dir}}/{0}"
  RUNTIME_OUTPUT_DIRECTORY "${{config_rt_dir}}/{0}"
)_";

static inline constexpr char k_finale[] = R"_(

configure_file("${__nsbuild_data}/ModuleConfig.hxx" "${module_gen_dir}/${module_name}ModuleConfig.hpp")
configure_file("${__nsbuild_data}/ModuleConfig.cxx" "${module_gen_dir}/local/${module_name}ModuleConfig.cpp")
if (EXISTS "${module_dir}/private/_Config.hxx")
	configure_file("${module_dir}/private/_Config.hxx" "${module_gen_dir}/${module_name}InternalConfig.hpp")
else()
  if (NOT EXISTS "${module_gen_dir}/${module_name}InternalConfig.hpp")
    file(TOUCH "${module_gen_dir}/${module_name}InternalConfig.hpp")
  endif()
endif()
add_dependencies(${module_target} nsbuild-check)
	
)_";

static inline constexpr char k_glob_relative[] = R"_(

set(__glob_result)
foreach(__glob_file ${{{0}}})
  set(__rel_glob_file)
  file(RELATIVE_PATH __rel_glob_file "{1}" ${{__glob_file}})
  list(APPEND __glob_result ${{__rel_glob_file}})
endforeach()
set({0} ${{__glob_result}})

)_";

static inline constexpr char k_fetch_content[] = R"_(


include(FetchContent)
FetchContent_Declare(
  ${{{}_extern_name}}
  GIT_REPOSITORY ${{{}_repo}}
  GIT_TAG ${{{}_commit}}
  GIT_SHALLOW 1
  SOURCE_DIR ${{{}_src_dir}}
  SUBBUILD_DIR ${{{}_subbulid_dir}}
  BINARY_DIR ${{{}_bulid_dir}}
 )

FetchContent_MakeAvailable(${{{}_extern_name}})

)_";

static inline constexpr char k_project_name[] = R"_(

cmake_minimum_required(VERSION 3.25)

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project({0} VERSION {1} LANGUAGES C CXX)
include(ExternalProject)


)_";

static inline constexpr char k_module_install_cfg_in[] = R"_(

@PACKAGE_INIT@

include("${CMAKE_CURRENT_LIST_DIR}/@module_name@Targets.cmake")
check_required_components("@module_name@")

)_";

static inline constexpr char k_module_install[] = R"_(

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# include files are installed in sdk/include/Module directory
install(DIRECTORY ${module_src_dir}/public/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

# include files are installed in sdk/include/Module directory
install(TARGETS ${module_target}
        EXPORT ${module_name}_Targets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
       )

write_basic_package_version_file("${module_name}ConfigVersion.cmake"
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY SameMajorVersion)

configure_package_config_file(
  "${config_build_dir}/${PROJECT_NAME}Config.cmake.in"
  "${CMAKE_BINARY_DIR}/${module_name}Config.cmake"
  INSTALL_DESTINATION
    lib/cmake)

install(EXPORT ${module_name}_Targets
  FILE ${module_name}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION lib/cmake)

install(FILES "${CMAKE_BINARY_DIR}/${module_name}Config.cmake"
              "${CMAKE_BINARY_DIR}/${module_name}ConfigVersion.cmake"
        DESTINATION lib/cmake)

)_";

static inline constexpr char module_embed_biaries[] = R"_(

set(binary_sources)

if (EXISTS "${module_gen_dir}/local/EmbeddedResources.cpp")
  list(APPEND binary_sources "${module_gen_dir}/local/EmbeddedResources.hpp" "${module_gen_dir}/local/EmbeddedResources.cpp")
endif()

)_";
} // namespace cmake
