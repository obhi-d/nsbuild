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
#define BuildImportSym __attribute__((visibility("default")))
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

#define BC_PROJECT_VERSION_MAJOR      @PROJECT_VERSION_MAJOR@
#define BC_PROJECT_VERSION_MINOR      @PROJECT_VERSION_MINOR@
#define BC_PROJECT_VERSION_REVISION   @PROJECT_VERSION_PATCH@
#define BC_PROJECT_VERSION_STRING     "v@PROJECT_VERSION_MAJOR@.@PROJECT_VERSION_MINOR@.@PROJECT_VERSION_PATCH@"
#define BC_PROJECT_NAME               "@PROJECT_NAME@ v@PROJECT_VERSION_MAJOR@.@PROJECT_VERSION_MINOR@.@PROJECT_VERSION_PATCH@"

#define BC_ENDIANNESS                 @nsbuild_host_is_big_endian@