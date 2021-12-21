#pragma once
#include <string_view>

namespace cmake
{
static inline constexpr char k_main_preamble[] = R"_(

project({0})
cmake_minimum_required(VERSION 3.20)
set(nsbuild "{2}")

if (NOT DEFINED nsplatform)

  set(nsplatform)
  set(__main_nsbuild_dir "${{CMAKE_CURRENT_LIST_DIR}}")

  if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(nsplatform "windows")
  elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(nsplatform "linux")
  elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(nsplatform "macOS")
  endif()

endif()

# file(GENERATE OUTPUT ${{CMAKE_BINARY_DIR}}/FetchBuild.txt CONTENT "$<...>")
add_custom_target(nsbuild-check ALL
  COMMAND ${{nsbuild}} --check "${{CMAKE_COMMAND}}" "${{CMAKE_BINARY_DIR}}" -B=${{CMAKE_BUILD_TYPE}} -X="${{CMAKE_CXX_COMPILER}}" -C="${{CMAKE_C_COMPILER}}" -V="${{CMAKE_CXX_COMPILER_VERSION}}" -G="${{CMAKE_GENERATOR}}" -I="${{CMAKE_GENERATOR_INSTANCE}}" -U="${{CMAKE_GENERATOR_PLATFORM}}" -H="${{CMAKE_GENERATOR_TOOLSET}}" -T="${{CMAKE_TOOLCHAIN}}"  -N=${{GENERATOR_IS_MULTI_CONFIG}} -P=${{nsplatform}}
  WORKING_DIRECTORY ${{__main_nsbuild_dir}}
)

# set_property(
#   DIRECTORY
#   APPEND
#   PROPERTY CMAKE_CONFIGURE_DEPENDS
#   {1}/CMakeLists.txt
# )

include(CTest)
include(${{CMAKE_CURRENT_LIST_DIR}}/{1}/${{__nsbuild_preset}}/CMakeLists.txt)

)_";

static inline constexpr char k_target_include_directories[] =
    "target_include_directories(${{module_target}} {}  $<{}:{}>)";

static inline constexpr char k_fetch_content_start[] = R"_(

project({0}_install)
cmake_minimum_required(VERSION 3.20)

)_";

static inline constexpr char k_fetch_content[] = R"_(


include(FetchContent)
FetchContent_Declare(
  {0}
  GIT_REPOSITORY {1}
  GIT_TAG {2}
  SOURCE_DIR ${{fetch_src_dir}}
  SUBBUILD_DIR ${{fetch_subbulid_dir}}
  BINARY_DIR ${{fetch_bulid_dir}}
 )

FetchContent_MakeAvailable({0})

)_";

static inline constexpr char k_find_package[] =
    "\nfind_package({} {} REQUIRED PATHS ${{fetch_sdk_dir}} "
    "NO_DEFAULT_PATH)";

static inline constexpr char k_find_package_comp[] =
    "\nfind_package({} {} REQUIRED COMPONENTS {} PATHS ${{fetch_sdk_dir}} "
    "NO_DEFAULT_PATH)";

static inline constexpr char k_write_dependency[] = R"_(

if(__module_pub_deps)
  add_dependencies(${module_target} ${__module_pub_deps})
  target_link_libraries(${module_target} PUBLIC ${__module_pub_deps})
endif()
if (__module_priv_deps)
  add_dependencies(${module_target} ${__module_priv_deps})
  target_link_libraries(${module_target} PRIVATE ${__module_priv_deps})
endif()
if(__module_ref_deps)
  add_dependencies(${module_target} ${__module_ref_deps})
endif()
)_";

static inline constexpr char k_write_libs[] = R"_(

target_link_libraries(${module_target} PUBLIC ${__module_priv_libs})
target_link_libraries(${module_target} PRIVATE ${__module_pub_libs})

)_";

static inline constexpr char k_rt_locations[] = R"_(
  ARCHIVE_OUTPUT_DIRECTORY "${config_rt_dir}/Lib"
  LIBRARY_OUTPUT_DIRECTORY "${config_rt_dir}/Lib"
  RUNTIME_OUTPUT_DIRECTORY "${config_rt_dir}/Bin"
)_";

static inline constexpr char k_plugin_locations[] = R"_(
  RUNTIME_OUTPUT_DIRECTORY "${config_rt_dir}/Media/Plugins"
)_";

static inline constexpr char k_media_commands[] = R"_(
  set(__module_data)
  foreach(l ${data_group})
    if (NOT l MATCHES "^${module_dir}/media/Internal/")
      list(APPEND __module_data ${l})
    endif()
  endforeach()
  
  set(__module_data_output ${__module_data})
  list(TRANSFORM __module_data_output REPLACE ${module_dir}/media ${config_runtime_dir}/Media)
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

} // namespace cmake
