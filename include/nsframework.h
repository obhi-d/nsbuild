#pragma once
#include <nsmodule.h>
#include <unordered_set>
#include <filesystem>

struct nsbuild;
struct nsframework
{
  std::unordered_set<std::string_view> excludes;
  std::vector<nsmodule>                modules;
  std::string                          name;
  std::string                          source_path;
  bool                                 processed = false;
  
  nsframework()                   = default;
  nsframework(nsframework&&) = default;
  nsframework& operator=(nsframework&&) = default;

  nsframework(nsframework const&) = delete;
  nsframework& operator=(nsframework const&) = delete;

  void process(nsbuild const& bc);
};
