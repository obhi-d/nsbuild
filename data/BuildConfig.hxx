#pragma once

#define BC_TOKEN_PASTE(X, Y)              BC_TOKEN_PASTE_INTERNAL1(X, Y)
#define BC_TOKEN_PASTE2(X, Y, Z)          BC_TOKEN_PASTE_INTERNAL2(X, Y, Z)
#define BC_TOKEN_PASTE_INTERNAL1(X, Y)    X##Y
#define BC_TOKEN_PASTE_INTERNAL2(X, Y, Z) X##Y##Z

#if defined(_MSC_VER)
#define BC_EXPORT_SYM __declspec(dllexport)
#define BC_IMPORT_SYM __declspec(dllimport)
#define BC_EXPORT_TEMPLATE
#elif defined(__MINGW32__)
#define BC_EXPORT_SYM  __attribute__((visibility("default")))
#define BC_IMPORT_SYM  __attribute__((visibility("default")))
#define BC_EXPORT_TEMPLATE
#else
#define BC_EXPORT_SYM      __attribute__((visibility("default")))
#define BC_IMPORT_SYM      __attribute__((visibility("default")))
#define BC_EXPORT_TEMPLATE extern
#endif

#ifdef BC_SHARED_LIBS
#define BC_LIB_EXPORT BC_EXPORT_SYM
#define BC_LIB_IMPORT BC_IMPORT_SYM
#else
#define BC_LIB_EXPORT
#define BC_LIB_IMPORT
#endif

#include <string_view>

namespace BuildConfig
{
  namespace @PROJECT_NAME@
  {
    constexpr bool             DebugBuild    = BC_IS_DEBUG_BUILD;
    constexpr unsigned int     Major         = @PROJECT_VERSION_MAJOR@;
    constexpr unsigned int     Minor         = @PROJECT_VERSION_MINOR@;
    constexpr unsigned int     Revision      = @PROJECT_VERSION_PATCH@;
    constexpr std::string_view ProjectName   = "@PROJECT_NAME@ v@PROJECT_VERSION_MAJOR@.@PROJECT_VERSION_MINOR@.@PROJECT_VERSION_PATCH@";
    constexpr std::string_view VersionString = "v@PROJECT_VERSION_MAJOR@.@PROJECT_VERSION_MINOR@.@PROJECT_VERSION_PATCH@";
  }
}
