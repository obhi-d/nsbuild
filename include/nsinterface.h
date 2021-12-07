#pragma once
#include <nsparams.h>

struct nsinterface
{
  nsfilters                filters;
  std::vector<std::string> dependencies;
  nspath_list              libraries;
  nspath_list              binaries;
  nspath_list              includes;
  std::vector<std::string> definitions;
};

using nsinterface_list = std::vector<nsinterface>;
