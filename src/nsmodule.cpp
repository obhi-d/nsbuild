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

void nsmodule::update_macros(nsbuild const& bc, std::string const& targ_name,
                             nstarget& targ)
{
  macros["framework_dir"]    = framework_path;
  macros["framework_name"]   = framework_name;
  macros["module_dir"]       = source_path;
  macros["module_name"]      = name;
  macros["module_build_dir"] = build_path;
  macros["module_gen_dir"]   = gen_path;
  macros["module_target"]    = targ_name;

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
  if (has_data(type))
  {
    // Add a step
    nsbuildstep step;
    nsbuildcmds cmd;
    step.artifacts.push_back("${data_group_output}");
    step.check_deps = true;
    step.dependencies.push_back("${data_group}");
    step.injected_config.push_back("set(data_group_output ${data_group})");
    step.injected_config.push_back(
        "list(TRANSFORM data_group_output REPLACE ${module_dir}/media "
        "${config_runtime_dir}/Media)");
    cmd.msgs.push_back(std::format("Building data files for {}", name));
    cmd.params = "${nsbuild} --copy-media ${module_target}";
    step.steps.push_back(cmd);
    prebuild.push_back(step);
  }
}

void nsmodule::write_target(std::ofstream& os, nsbuild const& bc,
                            std::string const& name) const
{
  switch (type)
  {
  case modtype::data:
    os << fmt::format("\nadd_custom_target({} ALL SOURCES ${data_group})",
                      name);
    break;
  case modtype::exe:
    os << fmt::format("\nadd_executable({} ${source_group})", name);
    break;
  case modtype::ref:
    // reference module will not add local_include, its added by main module
  case modtype::external:
    os << fmt::format("\nadd_library({} INTERFACE IMPORTED GLOBAL)", name);
    break;
  case modtype::lib:
    if (bc.config.static_libs)
      os << fmt::format("\nadd_library({} STATIC ${source_group})", name);
    else
      os << fmt::format("\nadd_library({} SHARED ${source_group})", name);
  case modtype::plugin:
    os << fmt::format("\nadd_library({} MODULE ${source_group})", name);
  default:
  }
}

void nsmodule::write_includes(std::ofstream& os, nsbuild const& bc) const
{
  switch (type)
  {
  case modtype::data:
    break;
  case modtype::exe:
    write_include(os, "${module_dir}", "include", cmake::inheritance::pub);
    write_include(os, "${module_dir}", "local_include",
                  cmake::inheritance::priv);
    write_refs_includes(os, bc);
    break;
  case modtype::ref:
    break;
  case modtype::external:
    break;
  case modtype::lib:
    write_include(os, "${module_dir}", "include", cmake::inheritance::pub);
    write_include(os, "${module_dir}", "local_include",
                  cmake::inheritance::priv);
    write_refs_includes(os, bc);
  case modtype::plugin:
    write_include(os, "${module_dir}", "include", cmake::inheritance::priv);
    write_include(os, "${module_dir}", "local_include",
                  cmake::inheritance::priv);
    write_refs_includes(os, bc);
  default:
  }
}

void nsmodule::write_refs_includes(std::ofstream& os, nsbuild const& bc) const
{
  for (auto const& r : references)
  {
    auto const& m = bc.get_module(r);
    m.write_refs_includes(os, bc);
    write_include(os, m.source_path, "include", cmake::inheritance::pub);
    write_include(os, m.source_path, "local_include", cmake::inheritance::priv);
  }
}

void nsmodule::process(nsbuild const& bc, std::string const& targ_name,
                       nstarget& targ)
{
  update_properties(bc, targ_name, targ);
  update_macros(bc, targ_name, targ);
}

void nsmodule::write(nsbuild const& bc) const
{
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

  write_prebuild_steps(ofs, bc);
  write_target(ofs, bc, target_name);
  write_postbuild_steps(ofs, bc);
  write_includes(ofs, bc);
}

void nsmodule::write_prebuild_steps(std::ofstream& ofs, const nsbuild& bc) const
{
  if (!prebuild.empty())
  {
    for (auto const& step : prebuild)
      step.print(ofs, bc, *this);

    ofs << "\n# Prebuild target"
        << fmt::format("\nadd_custom_target({}.prebuild \n\tDEPENDS",
                       target_name);
    for (auto const& step : prebuild)
    {
      for (auto const& d : step.dependencies)
        ofs << d << " ";
    }
    ofs << fmt::format(
        "\n\tCOMMAND ${{CMAKE_COMMAND}} -E echo Prebuilt step for {}\n)",
        target_name);
  }
}

void nsmodule::write_variables(std::ofstream& ofs, nsbuild const& bc) const
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
