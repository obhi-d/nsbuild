#include <exception>
#include <fstream>
#include <nsbuild.h>
#include <nsmodule.h>
#include <nstarget.h>

bool has_data(modtype t)
{
  switch (t)
  {
  case modtype::data:
  case modtype::exe:
  case modtype::external:
  case modtype::lib:
  case modtype::plugin:
  case modtype::ref:
    return true;
  default:
    return false;
  }
}

bool has_sources(modtype t)
{
  switch (t)
  {
  case modtype::exe:
  case modtype::lib:
  case modtype::plugin:
    return true;
  default:
    return false;
  }
}

void nsmodule::update_macros(nsbuild const& bc, nstarget& targ)
{
  auto  pwd                = std::filesystem::current_path();
  auto& fw                 = bc.frameworks[targ.fw_idx];
  auto& mod                = fw.modules[targ.mod_idx];
  auto  fwpwd              = pwd / bc.scan_path / bc.frameworks_dir / fw.name;
  macros["framework_dir"]  = fwpwd.generic_string();
  macros["framework_name"] = fw.name;
  macros["module_dir"]     = (fwpwd / mod.name).generic_string();
  macros["module_name"]    = fw.name;
  for (auto& v : vars)
  {
    if (!v.filters.any())
    {
      for (auto& p : v.params)
      {
        std::string name = "var_";
        name += p.name;
        macros[name] = cmake::value(p.params);
      }
    }
  }

  macros.fallback = &bc.macros;
}

void nsmodule::process(nsbuild const& bc, std::string const& targ_name,
                       nstarget& targ)
{
  if (type == modtype::exe || type ==)
    glob_sources = nsglob{};

  update_macros(bc, targ);
  auto pwd = std::filesystem::current_path();
  auto cml = pwd / bc.scan_path / bc.build_dir / targ_name;

  auto          cmlf = cml / "CMakeLists.txt";
  std::ofstream ofs{cmlf};
  if (ofs)
  {
    write_variables(ofs, bc);
  }
  else
  {
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::red),
               "Failed to write to : {}", cmlf.generic_string());
    throw std::runtime_error("Could not create CMakeLists.txt");
  }
}

void nsmodule::write_variables(std::ofstream& ofs, nsbuild const& bc)
{
  macros.print(ofs);
  for (auto const& e : exports)
  {
    e.print(ofs, output_fmt::set_cache);
  }
  for (auto const& e : vars)
  {
    e.print(ofs);
  }
}
