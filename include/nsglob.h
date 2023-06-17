#pragma once
#include <filesystem>
#include <nscommon.h>
#include <string_view>
#include <unordered_set>

struct nsglob
{
  // std::string_view name;

  std::unordered_set<std::string>* file_filters = nullptr;
  std::string_view                 path_exclude_filters;

  std::vector<std::filesystem::path> sub_paths;
  using file_set = std::vector<std::filesystem::path>;
  file_set files;

  std::string sha;
  bool        recurse = false;

  inline void add_set(std::filesystem::path p) { sub_paths.emplace_back(std::filesystem::absolute(std::move(p))); }

  void print(std::ostream&, std::string_view) const;
  void print(std::ostream&, std::string_view as, std::string_view ctx, std::filesystem::path const& relative_to) const;
  void accumulate();
  void process_entry(file_set&, std::filesystem::directory_entry const&);
  void process_directory(file_set&, std::filesystem::directory_entry const&);
};
