
#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class nsheader_map
{
public:
  nsheader_map() noexcept = default;

  void        scan_frameworks(std::filesystem::path fwdir) noexcept;
  void        scan_modules(std::filesystem::path mods) noexcept;
  std::size_t build(std::filesystem::path const& p) noexcept;
  void        write(std::filesystem::path p);

  struct node
  {
    std::string           name;
    std::filesystem::path location;
  };

  std::vector<std::filesystem::path>               header_paths;
  std::unordered_map<std::string, std::size_t>     unique_entities;
  std::vector<node>                                nodes;
  std::vector<std::pair<std::size_t, std::size_t>> edges;
};