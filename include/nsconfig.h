#pragma once 

#include <nscommon.h>

struct nsconfig
{
  bool is_accepted(std::string_view v)
  {
    for (auto const& e : accepted)
      if(e == v) return true;
    return false;
  }

  std::vector<std::string> accepted;
};