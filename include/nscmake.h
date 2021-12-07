#include <string_view>

namespace cmake
{
static inline constexpr char k_main_preamble[] = R"_(

project($project)
cmake_minimum_required(VERSION 3.20)

)_";

static inline constexpr char k_target_include_directories[] =
    "target_include_directories(${{module_target}} {}  $<{}:{}>)";

}

