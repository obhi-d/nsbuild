
#pragma once

#include <compare>
#include <filesystem>
#include <string>
#include <unordered_map>

struct nsinstallers
{
  std::unordered_map<std::string, bool> uninstallers;

  void clear() { uninstallers.clear(); }

  void save(std::filesystem::path const&) const;
  void load(std::filesystem::path const&);

  void uninstall_unused_and_save(std::filesystem::path const&) const;
  void installed(std::string entry) { uninstallers[entry] = true; }
  void uninstall(std::string const& entry) const;
};