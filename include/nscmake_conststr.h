#include <string_view>

namespace cmake
{
static inline constexpr char k_main_preamble[] = R"_(

project({})
cmake_minimum_required(VERSION 3.20)

)_";

static inline constexpr char k_target_include_directories[] =
    "target_include_directories(${{module_target}} {}  $<{}:{}>)";

static inline constexpr char k_fetch_content[] = R"_(

set(FETCHCONTENT_BASE_DIR ${{fetch_src_dir}} CACHE PATH "" FORCE)
include(FetchContent)
FetchContent_Declare({0}
  GIT_REPOSITORY {1}
  GIT_TAG {2}
)

FetchContent_GetProperties({0})
if (NOT boostorg_POPULATED)
    FetchContent_Populate({0}
      SUBBUILD_DIR sbld
      BINARY_DIR bld
    )
    add_subdirectory(${{{3}}} ${{{0}_BINARY_DIR}})
endif ()

)_";

} // namespace cmake
