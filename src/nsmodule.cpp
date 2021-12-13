#include <exception>
#include <fstream>
#include <nsbuild.h>
#include <nscmake.h>
#include <nslog.h>
#include <nsmodule.h>
#include <nsprocess.h>
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

  if (fetch)
  {
    std::filesystem::path tmp = build_path;
    /// - dl/module contains sources
    /// - bld/module/ts Timestamp directory
    /// - bld/module/bld  Build directory
    /// - bld/module/sdk Install directory
    macros["fetch_bulid_dir"]    = (tmp / k_build_dir).generic_string();
    macros["fetch_subbulid_dir"] = (tmp / k_subbuild_dir).generic_string();
    macros["fetch_ts_dir"]       = (tmp / k_ts_dir).generic_string();
    macros["fetch_sdk_dir"]      = (tmp / k_sdk_dir).generic_string();

    tmp                     = bc.download_dir;
    macros["fetch_src_dir"] = (tmp / name).generic_string();
  }

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

  auto cml = pwd / bc.scan_path / bc.build_dir / name;

  framework_path = fw.source_path;
  framework_name = fw.name;
  source_path    = p.generic_string();
  build_path     = cml.generic_string();
  gen_path       = (cml / k_gen_dir).generic_string();

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
    step.check = "data_group";
    step.dependencies.push_back("${data_group}");
    step.injected_config.push_back("set(data_group_output ${data_group})");
    step.injected_config.push_back(
        "list(TRANSFORM data_group_output REPLACE ${module_dir}/media "
        "${config_runtime_dir}/Media)");

    cmd.msgs.push_back(fmt::format("Building data files for {}", name));
    cmd.params = "${nsbuild} --copy-media ${module_target}";

    step.steps.push_back(cmd);
    step.wd = bc.wd.generic_string();
    prebuild.push_back(step);
  }
}

void nsmodule::write_target(std::ofstream& os, nsbuild const& bc,
                            std::string const& name) const
{
  switch (type)
  {
  case modtype::data:
    os << fmt::format("\nadd_custom_target({} ALL SOURCES ${{data_group}})",
                      name);
    break;
  case modtype::exe:
    os << fmt::format("\nadd_executable({} ${{source_group}})", name);
    break;
  case modtype::ref:
    // reference module will not add local_include, its added by main module
  case modtype::external:
    os << fmt::format("\nadd_library({} INTERFACE IMPORTED GLOBAL)", name);
    break;
  case modtype::lib:
    if (bc.config.static_libs)
      os << fmt::format("\nadd_library({} STATIC ${{source_group}})", name);
    else
      os << fmt::format("\nadd_library({} SHARED ${{source_group}})", name);
  case modtype::plugin:
    os << fmt::format("\nadd_library({} MODULE ${{source_group}})", name);
  default:
    break;
  }
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
      for (auto const& d : step.artifacts)
        ofs << d << " ";
    }
    ofs << fmt::format(
        "\n\tCOMMAND ${{CMAKE_COMMAND}} -E echo Prebuilt step for {}\n)",
        target_name);
    ofs << std::format("\nadd_dependency({0} {0}.prebuild)", target_name);
  }
}

void nsmodule::write_postbuild_steps(std::ofstream& ofs,
                                     nsbuild const& bc) const
{
  if (!postbuild.empty())
  {
    ofs << fmt::format("\nadd_custom_command(TARGET {} POST_BUILD ");
    for (auto const& step : prebuild)
    {
      for (auto const& s : step.steps)
      {
        ofs << "\n\tCOMMAND " << s.command << " " << s.params;
        for (auto const& m : s.msgs)
          ofs << "\n\tCOMMAND ${CMAKE_COMMAND} -E echo \"" << m << "\"";
      }
    }
    ofs << "\n\t)";
  }
}

void nsmodule::write_includes(std::ofstream& os, nsbuild const& bc) const
{
  switch (type)
  {
  case modtype::data:
    break;
  case modtype::exe:

    write_include(os, "${module_dir}", "include", cmake::inheritance::pub,
                  cmake::exposition::install);
    write_include(os, "${module_dir}", "local_include",
                  cmake::inheritance::priv);
    write_refs_includes(os, bc, *this);
    break;
  case modtype::ref:
    break;
  case modtype::external:
    break;
  case modtype::lib:
    write_include(os, "${module_dir}", "include", cmake::inheritance::pub,
                  cmake::exposition::install);
    write_include(os, "${module_dir}", "local_include",
                  cmake::inheritance::priv);
    write_refs_includes(os, bc, *this);
  case modtype::plugin:
    write_include(os, "${module_dir}", "include", cmake::inheritance::priv);
    write_include(os, "${module_dir}", "local_include",
                  cmake::inheritance::priv);
    write_refs_includes(os, bc, *this);
  default:
    break;
  }
}

void nsmodule::write_include(std::ofstream& ofs, std::string_view path,
                             std::string_view   subpath,
                             cmake::inheritance inherit,
                             cmake::exposition  expo) const
{
  // Assume build
  ofs << fmt::format("\nif(EXISTS \"{}/{}\")", path, subpath);
  ofs << "\n\ttarget_include_directories(${{module_target}} "
      << cmake::to_string(inherit);
  ofs << fmt::format("\n\t\t$<BUILD_INTERFACE:{}/{}>", path, subpath);
  if (expo == cmake::exposition::install)
    ofs << fmt::format("\n\t\t$<INSTALL_INTERFACE:{}>", subpath);
  ofs << "\n)\nendif()";
}

void nsmodule::write_refs_includes(std::ofstream& os, nsbuild const& bc,
                                   nsmodule const& target) const
{
  for (auto const& r : references)
  {
    auto const& m = bc.get_module(r);
    m.write_refs_includes(os, bc, target);
    target.write_include(os, m.source_path, "include",
                         target.type == modtype::plugin
                             ? cmake::inheritance::priv
                             : cmake::inheritance::pub);
    target.write_include(os, m.source_path, "local_include",
                         cmake::inheritance::priv);
  }
}

void nsmodule::write_find_package(std::ofstream& ofs, nsbuild const& bc) const
{
  if (!fetch)
    return;

  if (fetch->components.empty())
  ofs << std::format("\nfind_package({} {} REQUIRED PATHS ${{fetch_sdk_dir}} NO_DEFAULT_PATH)", fetch->package, fetch->version,
}

void nsmodule::process(nsbuild const& bc, std::string const& targ_name,
                       nstarget& targ)
{
  update_properties(bc, targ_name, targ);
  update_macros(bc, targ_name, targ);
  update_fetch(bc);
}

/// @brief Fetch content will pull commit from remote if needed
/// It will then attempt to build the content, if build fails, compilation
/// will fail.
/// - Fetch will build the project using build instructions
/// - Fetch will then install the project
/// - Directory structure is:
/// - dl/module/src contains sources
/// - bld/module/ts Timestamp directory
/// - bld/module/bld  Build directory
/// - bld/module/sdk Install directory
void nsmodule::update_fetch(nsbuild const& bc)
{
  if (!fetch)
    return;
  std::filesystem::path bld = build_path;
  std::error_code       ec;

  if (force_rebuild)
  {
    // remove bld/module directory
    std::filesystem::remove_all(bld / k_build_dir, ec);
    std::filesystem::remove_all(bld / k_ts_dir, ec);
    std::filesystem::remove_all(bld / k_sdk_dir, ec);
  }

  fetch_content(bc);
}

void nsmodule::fetch_content(nsbuild const& bc)
{
  std::filesystem::path dl      = bc.download_dir;
  auto                  dlm     = dl / name;
  bool                  refetch = false;
  auto                  dlts    = dlm / ".last_fetch";
  auto                  src     = dlm / k_src_dir;
  if (std::filesystem::exists(dlts))
  {
    // read last fetch commit
    std::ifstream ts{dlts};
    std::string   repo, commit;
    ts >> repo >> commit;

    if (repo != fetch->repo)
    {
      refetch = true;
      // delete dl/module, without throwing error
      std::error_code ec;
      std::filesystem::remove_all(src, ec);
    }
    else if (commit != fetch->commit)
      refetch = true;
  }
  else
  {
    // Forced full clone and  fetch
    refetch = true;
    // delete dl/module, without throwing error
  }

  if (refetch)
    write_fetch_build(bc);

  if (refetch || force_rebuild)
    build_fetched_content(bc);

  std::ofstream ts{dlts};
  ts << fetch->repo << " " << fetch->commit;
}

void nsmodule::write_main_build(nsbuild const& bc) const
{
  if (type == modtype::ref)
    return;

  std::filesystem::path cml = build_path;

  auto          cmlf = cml / "CMakeLists.txt";
  std::ofstream ofs{cmlf};
  if (!ofs)
  {
    nslog::error(fmt::format("Failed to write to : {}", cmlf.generic_string()));
    throw std::runtime_error("Could not create CMakeLists.txt");
  }
  write_variables(ofs, bc);

  if (has_data(type))
    glob_media.print(ofs);

  if (has_sources(type))
    glob_sources.print(ofs);

  write_target(ofs, bc, target_name);
  write_includes(ofs, bc);

  write_prebuild_steps(ofs, bc);
  write_postbuild_steps(ofs, bc);
  write_find_package(ofs, bc);
  write_dependencies(ofs, bc);
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

void nsmodule::write_fetch_build(nsbuild const& bc) const
{
  std::filesystem::path cml = build_path;

  auto          cmlf = cml / k_cmake_dir / "CMakeLists.txt";
  std::ofstream ofs{cmlf};
  if (!ofs)
  {
    nslog::error(std::format("Failed to write to : {}", cmlf.generic_string()));
    throw std::runtime_error("Could not create CMakeLists.txt");
  }
  write_variables(ofs, bc);

  std::filesystem::path src     = source_path;
  auto                  srccmk  = src / k_cmake_dir / "CMakeLists.txt";
  auto                  src_var = std::format("{}_SOURCE_DIR", fetch->name);
  if (std::filesystem::exists(srccmk))
  {
    ofs << fmt::format("\nset(module_cmk_src \"{}\")", srccmk.generic_string());
    src_var = "module_cmk_src";
  }

  for (auto const& a : fetch->args)
  {
    a.print(ofs);
  }

  ofs << fmt::format(cmake::k_fetch_content, fetch->name, fetch->repo,
                     fetch->commit, src_var);
}

void nsmodule::build_fetched_content(nsbuild const& bc) const
{
  std::filesystem::path cml  = build_path;
  auto                  dcmk = cml / k_cmake_dir;
  auto                  dbld = cml / k_build_dir;
  auto                  dsdk = cml / k_sdk_dir;

  nsprocess::cmake_config(bc, {}, dcmk.generic_string(), dbld);
  nsprocess::cmake_build(bc, "", dbld);
  nsprocess::cmake_install(bc, dsdk.generic_string(), dbld);
}
