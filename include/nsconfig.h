#pragma once
#include "nscommon.h"

struct nsconfig
{
  std::string              name;
  nsfilters                filters;
  std::vector<std::string> compiler_flags;
  std::vector<std::string> linker_flags;
};
