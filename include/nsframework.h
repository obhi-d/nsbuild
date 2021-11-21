#pragma once
#include <nsmodule.h>
#include <unordered_set>

struct nsframework
{
  std::unordered_set<std::string_view> excludes;
  std::vector<nsmodule>                modules;
  std::string                          name;
  
  nsframework()                   = default;
  nsframework(nsframework&&) = default;
  nsframework& operator=(nsframework&&) = default;

  nsframework(nsframework const&) = delete;
  nsframework& operator=(nsframework const&) = delete;
};
