#pragma once
#include "nscommon.h"

struct nscompiler
{
  nsfilters   filters;
  std::string compiler_flags;
  std::string linker_flags;
};
