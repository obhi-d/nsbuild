#pragma once
#include <filesystem>
#include <nscommon.h>
#include <string_view>
#include <unordered_set>

struct nsglob
{
  // std::string_view name;

  std::unordered_set<std::string>* file_filters = nullptr;
  std::string_view path_exclude_filters;

  struct file_set
  {
    std::filesystem::path              sub_path;
    std::vector<std::filesystem::path> files;
  };

  std::vector<file_set> sub_paths;

  std::string sha;
  bool        recurse = false;

  inline void add_set(std::filesystem::path p)
  {
    sub_paths.emplace_back();
    sub_paths.back().sub_path = std::filesystem::absolute(std::move(p));
  }

  void print(std::ostream&, std::string_view) const;
  void print(std::ostream&, std::string_view as, std::string_view ctx, std::filesystem::path const& relative_to) const;
  void accumulate();
  void process_entry(file_set&, std::filesystem::directory_entry const&);
  void process_directory(file_set&, std::filesystem::directory_entry const&);
};
