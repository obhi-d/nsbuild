// Generated
#pragma once

#include "BuildConfig.hpp"
#include "@module_name@InternalConfig.hpp"

#ifdef BC_MODULE_@PROJECT_NAME@
  #define @macro_prefix@@PROJECT_NAME@API BC_LIB_EXPORT
#else
  #define @macro_prefix@@PROJECT_NAME@API BC_LIB_IMPORT
#endif


#define BC_MODULE_VERSION_MAJOR     @PROJECT_VERSION_MAJOR@
#define BC_MODULE_VERSION_MINOR     @PROJECT_VERSION_MINOR@
#define BC_MODULE_VERSION_REVISION  @PROJECT_VERSION_PATCH@
#define BC_MODULE_VERSION_STRING    "v@PROJECT_VERSION_MAJOR@.@PROJECT_VERSION_MINOR@.@PROJECT_VERSION_PATCH@"
#define BC_MODULE_NAME              "@PROJECT_NAME@ v@PROJECT_VERSION_MAJOR@.@PROJECT_VERSION_MINOR@.@PROJECT_VERSION_PATCH@"
