
#include "nsheader_map.h"

#include <fstream>

void nsheader_map::scan_modules(std::filesystem::path mods) noexcept
{
  for (auto it : std::filesystem::directory_iterator(mods))
  {
    if (it.is_directory())
    {
      auto priv = it.path() / "private";
      if (std::filesystem::exists(priv))
        header_paths.emplace_back(std::move(priv));
      auto pub = it.path() / "public";
      if (std::filesystem::exists(pub))
        header_paths.emplace_back(std::move(pub));
    }
  }
}

void nsheader_map::scan_frameworks(std::filesystem::path source) noexcept
{
  for (auto it : std::filesystem::directory_iterator(source))
  {
    if (it.is_directory())
    {
      scan_modules(it.path());
    }
  }
}

void nsheader_map::write(std::filesystem::path p)
{
  std::ofstream outf{p};
  for (auto const& e : edges)
  {
    outf << nodes[e.first].name << "," << nodes[e.second].name << ","
         << "include\n";
  }
}

std::size_t nsheader_map::build(std::filesystem::path const& p) noexcept
{
  auto name = p.filename().string();
  auto it   = unique_entities.find(name);
  if (it != unique_entities.end())
  {
    return it->second;
  }

  auto node_id          = nodes.size();
  unique_entities[name] = node_id;
  nodes.emplace_back(node{.name = name, .location = p});

  std::ifstream filein{p};

  for (std::string line; std::getline(filein, line);)
  {
    if (line.starts_with("#include"))
    {
      auto off = line.find_first_of("\"<", 8);
      if (off == line.npos)
        continue;
      auto next = line.find_first_of("\">", off + 1);
      if (next == line.npos)
        continue;
      std::string file_name = line.substr(off + 1, next - (off + 1));
      for (auto const& hp : header_paths)
      {
        auto p = hp / file_name;
        if (std::filesystem::exists(p))
        {
          auto l = build(p);
          edges.emplace_back(node_id, l);
        }
      }
    }
  }
  return node_id;
}
