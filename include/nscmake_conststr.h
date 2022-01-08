#pragma once
#include <string_view>

namespace cmake
{
static inline constexpr char k_main_preamble[] = R"_(

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project({0} VERSION {4} LANGUAGES C CXX)
cmake_minimum_required(VERSION 3.20)
set(nsbuild "{2}")

set(__nsbuild_app_options)
set(__nsbuild_console_app_options)

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

## Compile commands

include(TestBigEndian)
TEST_BIG_ENDIAN(nsbuild_host_is_big_endian)

# file(GENERATE OUTPUT ${{CMAKE_BINARY_DIR}}/FetchBuild.txt CONTENT "$<...>")
add_custom_target(nsbuild-check ALL
  COMMAND ${{nsbuild}} --check "${{__nsbuild_preset}}" "${{CMAKE_COMMAND}}" "${{CMAKE_BINARY_DIR}}" -B=${{CMAKE_BUILD_TYPE}} -X="${{CMAKE_CXX_COMPILER}}" -C="${{CMAKE_C_COMPILER}}" -V="${{CMAKE_CXX_COMPILER_VERSION}}" -G="${{CMAKE_GENERATOR}}" -I="${{CMAKE_GENERATOR_INSTANCE}}" -U="${{CMAKE_GENERATOR_PLATFORM}}" -H="${{CMAKE_GENERATOR_TOOLSET}}" -T="${{CMAKE_TOOLCHAIN}}"  -N=${{GENERATOR_IS_MULTI_CONFIG}} -P=${{nsplatform}}
  WORKING_DIRECTORY ${{__main_nsbuild_dir}}
)


# set_property(
#   DIRECTORY
#   APPEND
#   PROPERTY CMAKE_CONFIGURE_DEPENDS
#   {1}/CMakeLists.txt
# )

set(CMAKE_UNITY_BUILD ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
include(CTest)
add_subdirectory(${{CMAKE_CURRENT_LIST_DIR}}/{1}/${{__nsbuild_preset}}/{3} ${{CMAKE_CURRENT_LIST_DIR}}/{1}/${{__nsbuild_preset}}/{3}/main)

add_custom_target(nsbuild-copy-ccjson ALL
  COMMAND ${{CMAKE_COMMAND}} -E create_symlink ${{CMAKE_BINARY_DIR}}/compile_commands.json ${{__main_nsbuild_dir}}/compile_commands.json 
  DEPENDS nsbuild-check
)


)_";

static inline constexpr char k_target_include_directories[] =
    "target_include_directories(${{module_target}} {}  $<{}:{}>)";

static inline constexpr char k_find_package[] =
    "\nfind_package({} {} REQUIRED PATHS ${{fetch_sdk_dir}} NO_DEFAULT_PATH)";

static inline constexpr char k_find_package_comp_start[] = "\nfind_package({} {} REQUIRED COMPONENTS ";
static inline constexpr char k_find_package_comp_end[] = "PATHS ${fetch_sdk_dir} NO_DEFAULT_PATH)";

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

static inline constexpr char k_enums_json[] = R"_(

set(enums_json)
set(enums_json_output)
set(has_enums_json FALSE)

if (EXISTS "${module_dir}/include/Enums.json")
  list(APPEND enums_json "${module_dir}/include/Enums.json")
  list(APPEND enums_json_output "${module_gen_dir}/Lum${module_name}Enums.h")
  set(has_enums_json TRUE)
endif()
if (EXISTS "${module_dir}/local_include/Enums.json")
  list(APPEND enums_json "${module_dir}/local_include/Enums.json")
  list(APPEND enums_json_output "${module_gen_dir}/local/Lum${module_name}LocalEnums.h")
  set(has_enums_json TRUE)
endif()
if(has_enums_json)
  list(APPEND enums_json_output "${module_gen_dir}/local/Lum${module_name}Enums.cpp")
endif()

)_";

static inline constexpr char k_media_commands[] = R"_(
  set(__module_data)
  foreach(l ${data_group})
    if (NOT l MATCHES "^${module_dir}/media/Internal/")
      list(APPEND __module_data ${l})
    endif()
  endforeach()
  
  set(data_group_output ${__module_data})
  list(TRANSFORM data_group_output REPLACE ${module_dir}/media ${config_rt_dir}/media)
)_";

static inline constexpr char k_finale[] = R"_(

if (EXISTS "${module_dir}/local_include/_Config.hxx")
	configure_file("${module_dir}/local_include/_Config.hxx" "${module_gen_dir}/${module_name}ModuleConfig.h")
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
  ${fetch_extern_name}
  GIT_REPOSITORY ${fetch_repo}
  GIT_TAG ${fetch_commit}
  SOURCE_DIR ${fetch_src_dir}
  SUBBUILD_DIR ${fetch_subbulid_dir}
  BINARY_DIR ${fetch_bulid_dir}
 )

FetchContent_MakeAvailable(${fetch_extern_name})

)_";

static inline constexpr char k_ext_cmake_proj_start[] = R"_(


ExternalProject_Add(${fetch_extern_name}
  GIT_REPOSITORY ${fetch_repo}
  GIT_TAG  ${fetch_commit}
  SOURCE_DIR ${fetch_src_dir}
  BINARY_DIR ${fetch_bulid_dir}
  INSTALL_DIR ${fetch_sdk_dir}
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
    -DCMAKE_GENERATOR_INSTANCE=${CMAKE_GENERATOR_INSTANCE}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_INSTALL_PREFIX=${fetch_sdk_dir})_";


static inline constexpr char k_ext_proj_custom[] = R"_(

if (NOT __fetch_configure_cmd)
  set(__fetch_configure_cmd ${CMAKE_COMMAND} -E echo " -- Fake Configuring ${module_name}")
endif()
if (NOT __fetch_build_cmd)
  set(__fetch_build_cmd ${CMAKE_COMMAND} -E echo " -- Fake Building ${module_name}")
endif()
if (NOT __fetch_install_cmd)
  set(__fetch_install_cmd ${CMAKE_COMMAND} -E echo " -- Fake Installing ${module_name}")
endif()

if (NOT __fetch_is_custom_build)
  ExternalProject_Add(${fetch_extern_name}
    GIT_REPOSITORY ${fetch_repo}
    GIT_TAG  ${fetch_commit}
    SOURCE_DIR ${fetch_src_dir}
    BINARY_DIR ${fetch_bulid_dir}
    INSTALL_DIR ${fetch_sdk_dir}
    CONFIGURE_COMMAND ${__fetch_configure_cmd}
    BUILD_COMMAND ${__fetch_configure_cmd}
    INSTALL_COMMAND ${__fetch_install_cmd}
  )
endif()

)_";

static inline constexpr char k_project_name[] = R"_(

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project({0} VERSION {1} LANGUAGES C CXX)
cmake_minimum_required(VERSION 3.20)
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
install(DIRECTORY ${fetch_src_dir}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

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
  "${module_build_dir}/${module_name}Config.cmake"
  INSTALL_DESTINATION
    lib/cmake)

install(EXPORT ${module_name}_Targets
  FILE ${module_name}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION lib/cmake)

install(FILES "${module_build_dir}/${module_name}Config.cmake"
              "${module_build_dir}/${module_name}ConfigVersion.cmake"
        DESTINATION lib/cmake)

)_";

} // namespace cmake
