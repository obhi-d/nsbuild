#pragma once
#include <nsparams.h>

struct nsinterface
{
  nsfilters                filters;
  std::vector<std::string> dependencies;
  std::vector<std::string> sys_libraries;
  nameval_list definitions;
};

using nsinterface_list = std::vector<nsinterface>;
