
#include "nsheader_map.h"

#include "fmt/format.h"
#include "nsbuild.h"

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

void nsheader_map::write_json(std::filesystem::path p)
{
  std::ofstream outf{p};
  outf << "{\n";
  outf << " \"nodes\": [\n";
  bool first = true;
  for (auto const& e : nodes)
  {
    if (!first)
      outf << ",\n";
    outf << fmt::format("  {{ \"id\": \"{}\" }}", e.name);
    first = false;
  }
  outf << " \n ],\n";
  outf << " \"links\": [\n";
  first = true;
  for (auto const& e : edges)
  {
    if (!first)
      outf << ",\n";
    outf << fmt::format("  {{ \"source\": \"{}\", \"target\":\"{}\" }}", nodes[e.first].name, nodes[e.second].name);
    first = false;
  }
  outf << " \n ]";
  outf << "\n}";
}

void nsheader_map::write_html(std::filesystem::path p, nsbuild const& nsb)
{
  if (has_cycles)
    write_cycle_html(p, nsb);
  else
    write_topo_html(p, nsb);
}

void nsheader_map::write_cycle_html(std::filesystem::path p, nsbuild const& nsb)
{
  std::ofstream outf{p};

  {
    std::ifstream preamble{nsb.get_data_dir() / "CyclicHeaderMapBegin.html"};
    std::string   str((std::istreambuf_iterator<char>(preamble)), std::istreambuf_iterator<char>());
    outf << str;
  }

  {
    std::ifstream preamble{nsb.get_data_dir() / "d3.v6.min.js"};
    std::string   str((std::istreambuf_iterator<char>(preamble)), std::istreambuf_iterator<char>());
    outf << str << "\n";
  }

  outf << "const graph="
          "{\n"
          " nodes: [\n";
  bool first = true;
  auto node  = cycle.back();
  int  end   = (int)cycle.size() - 1;
  int  e     = end;
  for (; e >= 0; --e)
  {
    if (cycle[e] == node && !first)
      break;
    if (!first)
      outf << ",\n";
    outf << fmt::format("  {{ id: \"{}\" }}", nodes[cycle[e]].name);
    first = false;
  }
  outf << " \n ],\n";
  outf << " links: [\n";
  first = true;
  for (; e < end; ++e)
  {
    if (!first)
      outf << ",\n";
    outf << fmt::format("  {{ source:\"{}\", target:\"{}\" }}", nodes[cycle[e]].name, nodes[cycle[e + 1]].name);
    first = false;
  }
  outf << " \n ]"
          "\n}";

  {
    std::ifstream preamble{nsb.get_data_dir() / "CyclicHeaderMapEnd.html"};
    std::string   str((std::istreambuf_iterator<char>(preamble)), std::istreambuf_iterator<char>());
    outf << str;
  }
}

void nsheader_map::write_topo_html(std::filesystem::path p, nsbuild const& nsb)
{
  std::ofstream outf{p};

  {
    std::ifstream preamble{nsb.get_data_dir() / "TopoHeaderMapBegin.html"};
    std::string   str((std::istreambuf_iterator<char>(preamble)), std::istreambuf_iterator<char>());
    outf << str;
  }

  {
    std::ifstream preamble{nsb.get_data_dir() / "d3.v6.min.js"};
    std::string   str((std::istreambuf_iterator<char>(preamble)), std::istreambuf_iterator<char>());
    outf << str << "\n";
  }

  {
    std::ifstream preamble{nsb.get_data_dir() / "d3-dag.iife.min.js"};
    std::string   str((std::istreambuf_iterator<char>(preamble)), std::istreambuf_iterator<char>());
    outf << str << "\n";
  }

  outf << "const graph="
          "[\n";
  bool first = true;
  for (auto const& n : nodes)
  {
    if (!first)
      outf << ",\n";
    std::string parents;
    bool        firstParent = true;
    for (auto p : n.parents)
    {
      if (!firstParent)
        parents += ',';
      parents += fmt::format("\"{}\"", nodes[p].name);

      firstParent = false;
    }
    outf << fmt::format("  {{ id: \"{}\", parentIds:[{}] }}", n.name, parents);
    first = false;
  }
  outf << " \n];\n";

  {
    std::ifstream preamble{nsb.get_data_dir() / "TopoHeaderMapEnd.html"};
    std::string   str((std::istreambuf_iterator<char>(preamble)), std::istreambuf_iterator<char>());
    outf << str;
  }
}

int nsheader_map::build(std::filesystem::path const& p) noexcept
{
  auto name = p.filename().string();
  auto it   = unique_entities.find(name);
  if (it != unique_entities.end())
  {
    if (nodes[it->second].visiting)
    {
      has_cycles = true;
      cycle.push_back((int)it->second);
    }
    return it->second;
  }

  auto node_id          = (int)nodes.size();
  unique_entities[name] = node_id;
  nodes.emplace_back(node{.name = name, .location = p, .visiting = true});
  if (!has_cycles)
    cycle.push_back(node_id);
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
          nodes[node_id].parents.emplace_back(l);
        }
      }
    }
  }
  nodes[(uint32_t)node_id].visiting = false;
  if (!has_cycles)
    cycle.pop_back();
  return node_id;
}
