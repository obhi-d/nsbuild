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
  macros["framework_dir"]    = framework_path;
  macros["framework_name"]   = framework_name;
  macros["module_dir"]       = source_path;
  macros["module_name"]      = name;
  macros["module_build_dir"] = build_path;
  macros["module_gen_dir"]   = gen_path;

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

void nsmodule::update_properties(nsbuild const&     bc,
                                 std::string const& targ_name, nstarget& targ)
{
  auto  pwd = std::filesystem::current_path();
  auto& fw  = bc.frameworks[targ.fw_idx];

  std::filesystem::path p = fw.source_path;
  p /= name;

  auto cml = pwd / bc.scan_path / bc.build_dir / targ_name;

  framework_path = fw.source_path;
  framework_name = fw.name;
  source_path    = p.generic_string();
  build_path     = cml.generic_string();
  gen_path       = (cml / k_gen_folder).generic_string();

  if (has_data(type))
    glob_media =
        nsglob{.name = "data_group", .sub_paths = {"${module_dir}/media/*"}};
  if (has_sources(type))
    glob_sources =
        nsglob{.name      = "source_group",
               .sub_paths = {
                   "${module_dir}/src/*.cpp",
                   "${module_dir}/src/platform/${config_platform}/*.cpp",
                   "${module_gen_dir}/*.cpp", "${module_gen_dir}/local/*.cpp"}};
}

void nsmodule::process(nsbuild const& bc, std::string const& targ_name,
                       nstarget& targ)
{
  update_properties(bc, targ_name, targ);
  update_macros(bc, targ);

  std::filesystem::path cml = build_path;

  auto          cmlf = cml / "CMakeLists.txt";
  std::ofstream ofs{cmlf};
  if (!ofs)
  {
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::red),
               "Failed to write to : {}", cmlf.generic_string());
    throw std::runtime_error("Could not create CMakeLists.txt");
  }
  
  write_variables(ofs, bc);
  if (has_data(type))
    glob_media.print(ofs);
  if (has_sources(type))
    glob_sources.print(ofs);
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
