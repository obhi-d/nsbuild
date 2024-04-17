// Generated
#pragma once

// clang-format off
#include "@module_name@InternalConfig.hpp"
#include "BuildConfig.hpp"

#ifdef BC_MODULE_@PROJECT_NAME@
  #define @macro_prefix@@PROJECT_NAME@API BC_LIB_EXPORT
#else
  #define @macro_prefix@@PROJECT_NAME@API BC_LIB_IMPORT
#endif

namespace BuildConfig
{
  @module_extras_hpp@

  namespace @PROJECT_NAME@
  {
    constexpr unsigned int     Major         = @PROJECT_VERSION_MAJOR@;
    constexpr unsigned int     Minor         = @PROJECT_VERSION_MINOR@;
    constexpr unsigned int     Revision      = @PROJECT_VERSION_PATCH@;
    constexpr std::string_view BaseName      = "@PROJECT_NAME@";
    constexpr std::string_view ModuleName    = "@PROJECT_NAME@ v@PROJECT_VERSION_MAJOR@.@PROJECT_VERSION_MINOR@.@PROJECT_VERSION_PATCH@";
    constexpr std::string_view VersionString = "v@PROJECT_VERSION_MAJOR@.@PROJECT_VERSION_MINOR@.@PROJECT_VERSION_PATCH@";
  }
}
// clang-format on