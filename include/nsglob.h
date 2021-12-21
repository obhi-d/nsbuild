#pragma once
#include <nscommon.h>
#include <string_view>

struct nsglob
{
  std::string_view              name;
  std::string_view              relative_to;
  std::vector<std::string_view> sub_paths;

  bool                          recurse = false;

  void print(std::ostream&) const;
};
