
#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct nsbuild;
class nsheader_map
{
public:
  nsheader_map() noexcept = default;

  void scan_frameworks(std::filesystem::path fwdir) noexcept;
  void scan_modules(std::filesystem::path mods) noexcept;
  int  build(std::filesystem::path const& p) noexcept;
  void write(std::filesystem::path p);
  void write_json(std::filesystem::path p);
  void write_html(std::filesystem::path p, nsbuild const&);
  void write_cycle_html(std::filesystem::path p, nsbuild const&);
  void write_topo_html(std::filesystem::path p, nsbuild const&);

  struct node
  {
    std::string           name;
    std::filesystem::path location;
    std::vector<int>      parents;
    bool                  visiting;
  };

  bool                                 has_cycles = false;
  std::vector<int>                     cycle;
  std::vector<std::filesystem::path>   header_paths;
  std::unordered_map<std::string, int> unique_entities;
  std::vector<node>                    nodes;
  std::vector<std::pair<int, int>>     edges;
};