#include <exception>
#include <fstream>
#include <nsbuild.h>
#include <nscmake.h>
#include <nslog.h>
#include <nsmodule.h>
#include <nsprocess.h>
#include <nstarget.h>

bool has_data(nsmodule_type t)
{
  switch (t)
  {
  case nsmodule_type::data:
  case nsmodule_type::exe:
  case nsmodule_type::external:
  case nsmodule_type::lib:
  case nsmodule_type::plugin:
  case nsmodule_type::ref:
  case nsmodule_type::test:
    return true;
  default:
    return false;
  }
}

bool has_sources(nsmodule_type t)
{
  switch (t)
  {
  case nsmodule_type::exe:
  case nsmodule_type::lib:
  case nsmodule_type::plugin:
  case nsmodule_type::test:
    return true;
  default:
    return false;
  }
}

void nsmodule::process(nsbuild const& bc, std::string const& targ_name,
                       nstarget& targ)
{
  update_properties(bc, targ_name, targ);
  update_macros(bc, targ_name, targ);
  update_fetch(bc);
  generate_plugin_manifest(bc);
}

void nsmodule::update_properties(nsbuild const&     bc,
                                 std::string const& targ_name, nstarget& targ)
{
  auto& fw = bc.frameworks[targ.fw_idx];

  std::filesystem::path p = fw.source_path;
  p /= name;

  framework_path = fw.source_path;
  framework_name = fw.name;
  source_path    = p.generic_string();

  if (has_data(type))
    glob_media = nsglob{.name      = "data_group",
                        .sub_paths = {"${module_dir}/media/*"},
                        .recurse   = true};
  if (has_sources(type))
    glob_sources = nsglob{
        .name      = "source_group",
        .sub_paths = {"${module_dir}/src/*.cpp",
                      "${module_dir}/src/platform/${config_platform}/*.cpp",
                      "${module_gen_dir}/*.cpp",
                      "${module_gen_dir}/local/*.cpp"},
        .recurse   = false};
  if (has_data(type))
  {
    // Add a step
    nsbuildstep step;
    nsbuildcmds cmd;
    step.artifacts.push_back("${data_group_output}");
    step.check = "data_group";
    step.dependencies.push_back("${data_group}");
    step.injected_config.push_back(cmake::k_media_commands);
    cmd.msgs.push_back(fmt::format("Building data files for {}", name));
    cmd.command = "${nsbuild}";
    cmd.params  = "--copy-media ${module_dir}/media ${config_rt_dir}/Media "
                 "${config_ignored_media}";

    step.steps.push_back(cmd);
    step.wd = bc.wd.generic_string();
    prebuild.push_back(step);
  }

  if (type == nsmodule_type::plugin)
  {
    // post build step to copy plugin manifest
    nsbuildstep step;
    nsbuildcmds cmd;
    step.artifacts.push_back(fmt::format(
        "${{config_rt_dir}}/{1}/{0}Manifest.json", name, bc.plugin_rel_dir));
    step.dependencies.push_back(
        fmt::format("${{module_gen_dir}}/{0}Manifest.json", name));
    cmd.msgs.push_back(fmt::format("Copying plugin manifest for {}", name));
    cmd.command = "${CMAKE_COMMAND}";
    cmd.params =
        fmt::format("-E copy_if_different ${{module_gen_dir}}/{0}Manifest.json "
                    "${{config_rt_dir}}/{1}",
                    name, bc.plugin_rel_dir);
    step.steps.push_back(cmd);
    step.wd = bc.wd.generic_string();
    prebuild.push_back(step);
  }
  else if (type == nsmodule_type::test)
  {
    nsbuildstep step;
    nsbuildcmds cmd;
    cmd.msgs.push_back(fmt::format("Copying test config for {}", name));
    cmd.command = "${CMAKE_COMMAND}";
    cmd.params =
        fmt::format("-E copy_if_different ${{module_dir}}/TestConfig.json "
                    "${{config_rt_dir}}/TestConfigs/{0}.json",
                    name);
    step.steps.push_back(cmd);
    step.wd = bc.wd.generic_string();
    postbuild.push_back(step);
  }
}

void nsmodule::update_macros(nsbuild const& bc, std::string const& targ_name,
                             nstarget& targ)
{
  macros["framework_dir"]    = framework_path;
  macros["framework_name"]   = framework_name;
  macros["module_dir"]       = source_path;
  macros["module_name"]      = name;
  macros["module_build_dir"] = cmake::path(get_full_bld_dir(bc));
  macros["module_gen_dir"]   = cmake::path(get_full_gen_dir(bc));
  macros["module_target"]    = targ_name;

  if (fetch)
  {
    std::filesystem::path tmp = bc.get_full_cfg_dir();
    /// - dl/module contains sources
    /// - bld/module/ts Timestamp directory
    /// - bld/module/bld  Build directory
    /// - bld/module/sdk Install directory
    macros["fetch_bulid_dir"]    = cmake::path(get_full_xpb_dir(bc));
    macros["fetch_subbulid_dir"] = cmake::path(get_full_sbld_dir(bc));
    macros["fetch_sdk_dir"]      = cmake::path(get_full_sdk_dir(bc));
    macros["fetch_src_dir"]      = cmake::path(get_full_dl_dir(bc));
    // macros["fetch_ts_dir"]       = cmake::path(get_full_ts_dir(bc));
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
}

void nsmodule::update_fetch(nsbuild const& bc)
{
  if (!fetch)
    return;
  std::error_code ec;

  if (force_rebuild)
  {
    // remove bld/module directory
    std::filesystem::remove_all(get_full_bld_dir(bc), ec);
    std::filesystem::remove_all(get_full_ts_dir(bc), ec);
    std::filesystem::remove_all(get_full_sdk_dir(bc), ec);
  }

  fetch_content(bc);
}

void nsmodule::generate_plugin_manifest(nsbuild const& bc)
{
  if (type != nsmodule_type::plugin)
    return;

  std::ofstream f{get_full_gen_dir(bc) / fmt::format("{0}Manifest.json", name)};
  f << "{" << fmt::format(k_json_val, "author", manifest.author)
    << fmt::format(k_json_val, "company", manifest.company)
    << fmt::format(k_json_val, "compatibility", manifest.compatibility)
    << fmt::format(k_json_val, "context", manifest.context)
    << fmt::format(k_json_val, "context", manifest.context)
    << fmt::format(k_json_val, "desc", manifest.desc)
    << fmt::format(k_json_val, "optional", manifest.optional)
    << fmt::format(k_json_val, "services", "{");
  for (auto const& s : manifest.services)
    f << fmt::format(k_json_val, s.first, s.second);
  f << "\n}\n}";
}

void nsmodule::write_fetch_build(nsbuild const& bc) const
{
  auto          cmlf = get_full_ext_dir(bc) / "CMakeLists.txt";
  std::ofstream ofs{cmlf};
  if (!ofs)
  {
    nslog::error(fmt::format("Failed to write to : {}", cmlf.generic_string()));
    throw std::runtime_error("Could not create CMakeLists.txt");
  }
  macros.print(ofs);
  bc.macros.print(ofs);
  write_variables(ofs, bc);

  std::filesystem::path src     = source_path;
  auto                  srccmk  = src / k_cmake_dir / "CMakeLists.txt";
  auto                  src_var = fmt::format("{}_SOURCE_DIR", fetch->name);
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

void nsmodule::fetch_content(nsbuild const& bc)
{
  if (regenerate || force_rebuild)
  {

    write_fetch_build(bc);
    build_fetched_content(bc);
  }
}

void nsmodule::write_main_build(nsbuild const& bc) const
{
  auto          cmlf = get_full_bld_dir(bc) / "CMakeLists.txt";
  std::ofstream ofs{cmlf};
  if (!ofs)
  {
    nslog::error(fmt::format("Failed to write to : {}", cmlf.generic_string()));
    throw std::runtime_error("Could not create CMakeLists.txt");
  }
  macros.print(ofs);
  write_variables(ofs, bc);

  if (has_data(type))
    glob_media.print(ofs);

  if (has_sources(type))
    glob_sources.print(ofs);

  write_target(ofs, bc, target_name);
  write_includes(ofs, bc);
  write_definitions(ofs, bc);

  write_prebuild_steps(ofs, bc);
  write_postbuild_steps(ofs, bc);
  write_find_package(ofs, bc);
  write_dependencies(ofs, bc);
  write_linklibs(ofs, bc);
  write_runtime_settings(ofs, bc);
  write_final_config(ofs, bc);
  write_install_command(ofs, bc);
  // config file
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

void nsmodule::write_target(std::ofstream& ofs, nsbuild const& bc,
                            std::string const& name) const
{
  switch (type)
  {
  case nsmodule_type::data:
    ofs << fmt::format("\nadd_custom_target({} ALL SOURCES ${{data_group}})",
                       name);
    break;
  case nsmodule_type::exe:
    ofs << fmt::format("\nadd_executable({} ${{source_group}})", name);
    break;
  case nsmodule_type::ref:
    ofs << fmt::format("\nadd_custom_target({} INTERFACE)", name);
    break;
  case nsmodule_type::external:
    ofs << fmt::format("\nadd_library({} INTERFACE IMPORTED GLOBAL)", name);
    break;
  case nsmodule_type::lib:
    if (bc.cmakeinfo.static_libs)
      ofs << fmt::format("\nadd_library({} STATIC ${{source_group}})", name);
    else
      ofs << fmt::format("\nadd_library({} SHARED ${{source_group}})", name);
    break;
  case nsmodule_type::plugin:
    ofs << fmt::format("\nadd_library({} MODULE ${{source_group}})", name);
    break;
  case nsmodule_type::test:
    ofs << fmt::format("\nadd_executable({} ${{source_group}})", name);
    break;
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
    ofs << fmt::format("\nadd_dependency({0} {0}.prebuild)", target_name);
  }
}

void nsmodule::write_postbuild_steps(std::ofstream& ofs,
                                     nsbuild const& bc) const
{
  if (!postbuild.empty())
  {
    ofs << fmt::format(
        "\nadd_custom_command(TARGET ${{module_target}} POST_BUILD ");
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

void nsmodule::write_includes(std::ofstream& ofs, nsbuild const& bc) const
{
  switch (type)
  {
  case nsmodule_type::data:
    break;
  case nsmodule_type::ref:
    break;
  case nsmodule_type::external:
    break;
  case nsmodule_type::lib:
    write_include(ofs, "${module_dir}", "include", cmake::inheritance::pub,
                  cmake::exposition::install);
    write_include(ofs, "${module_dir}", "local_include",
                  cmake::inheritance::priv);
    write_refs_includes(ofs, bc, *this);
  case nsmodule_type::test:
  case nsmodule_type::exe:
  case nsmodule_type::plugin:
    write_include(ofs, "${module_dir}", "include", cmake::inheritance::priv);
    write_include(ofs, "${module_dir}", "local_include",
                  cmake::inheritance::priv);
    write_refs_includes(ofs, bc, *this);
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
  ofs << fmt::format("\n\t\t$<BUILD_INTERFACE:\"{}/{}\">", path, subpath);
  if (expo == cmake::exposition::install)
    ofs << fmt::format("\n\t\t$<INSTALL_INTERFACE:\"{}\">", subpath);
  ofs << "\n)\nendif()";
}

void nsmodule::write_refs_includes(std::ofstream& ofs, nsbuild const& bc,
                                   nsmodule const& target) const
{
  for (auto const& r : references)
  {
    auto const& m = bc.get_module(r);
    m.write_refs_includes(ofs, bc, target);
    target.write_include(ofs, m.source_path, "include",
                         target.type == nsmodule_type::plugin
                             ? cmake::inheritance::priv
                             : cmake::inheritance::pub);
    target.write_include(ofs, m.source_path, "local_include",
                         cmake::inheritance::priv);
  }
}

void nsmodule::write_find_package(std::ofstream& ofs, nsbuild const& bc) const
{
  if (!fetch)
    return;

  if (fetch->components.empty())
    ofs << fmt::format(cmake::k_find_package, fetch->package, fetch->version);
  else
  {
    std::string s;
    for (auto c : fetch->components)
    {
      s += " ";
      s += c;
    }
    ofs << fmt::format(cmake::k_find_package_comp, fetch->package,
                       fetch->version, s);
  }

  if (fetch->link_with_ns)
  {
    ofs << "\ntarget_link_libraries(${{module_target}} "
        << cmake::to_string(cmake::inheritance::intf);
    if (fetch->components.empty())
      ofs << fmt::format(" {0}::{0}", fetch->name);
    else
    {
      for (auto const& c : fetch->components)
        ofs << fmt::format(" {}::{}", fetch->name, c);
    }
    ofs << "\n)";
  }
  else
  {
    ofs << fmt::format("\nset(__sdk_install_includes ${{{}_INCLUDE_DIRS}})",
                       fetch->package);
    ofs << "\nlist(TRANSFORM __sdk_install_includes REPLACE ${fetch_sdk_dir} "
           "\"\")";
    ofs << fmt::format("\nset(__sdk_install_libraries ${{{}_LIBRARIES}})",
                       fetch->package);
    ofs << "\nlist(TRANSFORM __sdk_install_libraries REPLACE ${fetch_sdk_dir} "
           "\"\")";

    ofs << "\ntarget_include_directories(${{module_target}} "
        << cmake::to_string(cmake::inheritance::intf)
        << fmt::format("\n\t$<BUILD_INTERFACE:\"{}_INCLUDE_DIR\">",
                       fetch->package)
        << "\n\t$<INSTALL_INTERFACE:\"${__sdk_install_includes}\">"
        << "\n)"
        << "\ntarget_link_libraries(${{module_target}} "
        << cmake::to_string(cmake::inheritance::intf)
        << fmt::format("\n\t$<BUILD_INTERFACE:\"{}_LIBRARIES\">",
                       fetch->package)
        << "\n\t$<INSTALL_INTERFACE:\"${__sdk_install_libraries}\">"
        << "\n)";
  }
}

void nsmodule::write_definitions(std::ofstream& ofs, nsbuild const& bc) const
{
  switch (type)
  {
  case nsmodule_type::data:
    break;
  case nsmodule_type::plugin:
  case nsmodule_type::lib:
  case nsmodule_type::exe:
  case nsmodule_type::test:
    write_definitions_mod(ofs, bc);
    break;
  case nsmodule_type::ref:
    break;
  case nsmodule_type::external:
    break;
  default:
    break;
  }
}

void nsmodule::write_definitions_mod(std::ofstream& ofs,
                                     nsbuild const& bc) const
{
  write_definitions(ofs, bc, 0);
  write_definitions(ofs, bc, 1);
  write_refs_definitions(ofs, bc, *this);
  write_definitions(ofs, fmt::format("CurrentModule_{}", name),
                    cmake::inheritance::priv, "");
}

void nsmodule::write_definitions(std::ofstream& ofs, nsbuild const& bc,
                                 std::uint32_t type) const
{
  for (std::size_t i = 0; i < intf[type].size(); ++i)
  {
    auto        filter = cmake::get_filter(intf[type][i].filters);
    auto const& dep    = intf[type][i].definitions;
    for (auto const& d : dep)
      write_definitions(ofs, d,
                        type == pub_intf ? cmake::inheritance::pub
                                         : cmake::inheritance::priv,
                        filter);
  }
}

void nsmodule::write_definitions(std::ofstream& ofs, std::string_view def,
                                 cmake::inheritance inherit,
                                 std::string_view   filter) const
{
  ofs << fmt::format("\ntarget_compile_definitions(${{module_target}} {} {})",
                     cmake::to_string(inherit),
                     filter.empty() ? std::string{def}
                                    : fmt::format("$<{}:{}>", filter, def));
}

void nsmodule::write_refs_definitions(std::ofstream& ofs, nsbuild const& bc,
                                      nsmodule const& target) const
{
  for (auto const& r : references)
  {
    auto const& m = bc.get_module(r);
    m.write_refs_definitions(ofs, bc, target);
    m.write_definitions_mod(ofs, bc);
  }
}

void nsmodule::write_dependencies(std::ofstream& ofs, nsbuild const& bc) const
{
  switch (type)
  {
  case nsmodule_type::data:
    break;
  case nsmodule_type::plugin:
  case nsmodule_type::lib:
  case nsmodule_type::exe:
  case nsmodule_type::test:
    write_dependencies_begin(ofs, bc);
    write_dependencies_mod(ofs, bc);
    write_dependencies_end(ofs, bc);
    ///
    break;
  case nsmodule_type::ref:
    break;
  case nsmodule_type::external:
    break;
  default:
    break;
  }
}

void nsmodule::write_dependencies_begin(std::ofstream& ofs,
                                        nsbuild const& bc) const
{
  ofs << "\nset(__module_priv_deps)";
  ofs << "\nset(__module_pub_deps)";
  ofs << "\nset(__module_ref_deps)";
}

void nsmodule::write_dependencies_mod(std::ofstream& ofs,
                                      nsbuild const& bc) const
{
  write_dependencies(ofs, bc, pub_intf);
  write_dependencies(ofs, bc, priv_intf);
  write_refs_dependencies(ofs, bc, *this);
}

void nsmodule::write_dependencies_end(std::ofstream& ofs,
                                      nsbuild const& bc) const
{
  ofs << cmake::k_write_dependency;
}

void nsmodule::write_dependencies(std::ofstream& ofs, nsbuild const& bc,
                                  std::uint32_t type) const
{
  for (std::size_t i = 0; i < intf[type].size(); ++i)
  {
    auto        filter = cmake::get_filter(intf[type][i].filters);
    auto const& dep    = intf[type][i].dependencies;
    for (auto const& d : dep)
      write_dependency(ofs, d,
                       type == pub_intf ? cmake::inheritance::pub
                                        : cmake::inheritance::priv,
                       filter);
  }
}

void nsmodule::write_dependency(std::ofstream& ofs, std::string_view target,
                                cmake::inheritance inh,
                                std::string_view   filter) const
{
  ofs << fmt::format("\nlist(APPEND {} {})",
                     inh == cmake::inheritance::pub ? "__module_pub_deps"
                                                    : "__module_pub_deps",
                     filter.empty() ? std::string{target}
                                    : fmt::format("$<{}:{}>", filter, target));
}

void nsmodule::write_refs_dependencies(std::ofstream& ofs, nsbuild const& bc,
                                       nsmodule const& target) const
{
  for (auto const& r : references)
  {
    auto const& m = bc.get_module(r);
    m.write_refs_dependencies(ofs, bc, target);
    m.write_dependencies_mod(ofs, bc);
    ofs << fmt::format("\nlist(APPEND __module_ref_deps {})", r);
  }
}

void nsmodule::write_linklibs(std::ofstream& ofs, nsbuild const& bc) const
{
  switch (type)
  {
  case nsmodule_type::data:
    break;
  case nsmodule_type::plugin:
  case nsmodule_type::lib:
  case nsmodule_type::exe:
  case nsmodule_type::test:
    write_linklibs_begin(ofs, bc);
    write_linklibs_mod(ofs, bc);
    write_linklibs_end(ofs, bc);
    ///
    break;
  case nsmodule_type::ref:
    break;
  case nsmodule_type::external:
    break;
  default:
    break;
  }
}

void nsmodule::write_linklibs_begin(std::ofstream& ofs, nsbuild const& bc) const
{
  ofs << "\nset(__module_priv_libs)";
  ofs << "\nset(__module_pub_libs)";
}

void nsmodule::write_linklibs_mod(std::ofstream& ofs, nsbuild const& bc) const
{
  write_linklibs(ofs, bc, pub_intf);
  write_linklibs(ofs, bc, priv_intf);
  write_refs_linklibs(ofs, bc, *this);
}

void nsmodule::write_linklibs_end(std::ofstream& ofs, nsbuild const& bc) const
{
  ofs << cmake::k_write_libs;
}

void nsmodule::write_linklibs(std::ofstream& ofs, nsbuild const& bc,
                              std::uint32_t type) const
{
  for (std::size_t i = 0; i < intf[type].size(); ++i)
  {
    auto        filter = cmake::get_filter(intf[type][i].filters);
    auto const& libs   = intf[priv_intf][i].sys_libraries;
    for (auto const& d : libs)
      write_linklibs(ofs, d,
                     type == pub_intf ? cmake::inheritance::pub
                                      : cmake::inheritance::priv,
                     filter);
  }
}

void nsmodule::write_linklibs(std::ofstream& ofs, std::string_view target,
                              cmake::inheritance inh,
                              std::string_view   filter) const
{
  ofs << fmt::format("\nlist(APPEND {} {})",
                     inh == cmake::inheritance::pub ? "__module_pub_libs"
                                                    : "__module_priv_libs",
                     filter.empty() ? std::string{target}
                                    : fmt::format("$<{}:{}>", filter, target));
}

void nsmodule::write_refs_linklibs(std::ofstream& ofs, nsbuild const& bc,
                                   nsmodule const& target) const
{
  for (auto const& r : references)
  {
    auto const& m = bc.get_module(r);
    m.write_refs_linklibs(ofs, bc, target);
    m.write_linklibs_mod(ofs, bc);
  }
}

void nsmodule::write_install_command(std::ofstream& ofs,
                                     nsbuild const& bc) const
{
  switch (type)
  {
  case nsmodule_type::data:
  case nsmodule_type::exe:
  case nsmodule_type::external:
  case nsmodule_type::lib:
    break;
  case nsmodule_type::plugin:
    break;
  case nsmodule_type::ref:
    break;
  case nsmodule_type::test:
    break;
  default:
    return;
  }
}

void nsmodule::write_final_config(std::ofstream& ofs, nsbuild const& bc) const
{
  ofs << cmake::k_finale;
  switch (type)
  {
  case nsmodule_type::data:
  case nsmodule_type::exe:
  case nsmodule_type::external:
  case nsmodule_type::lib:
    break;
  case nsmodule_type::plugin:
    break;
  case nsmodule_type::ref:
    break;
  case nsmodule_type::test:
    ofs << "\nadd_test(" << name << " COMMAND ${config_rt_dir}/Bin/"
        << target_name << ");";
    break;
  default:
    return;
  }
}

void nsmodule::write_runtime_settings(std::ofstream& ofs,
                                      nsbuild const& bc) const
{
  switch (type)
  {
  case nsmodule_type::data:
  case nsmodule_type::external:
  case nsmodule_type::ref:
    break;
  case nsmodule_type::exe:
  case nsmodule_type::lib:
  case nsmodule_type::test:
    ofs << "\nset_target_properties(" << target_name << " PROPERTIES "
        << cmake::k_rt_locations << "\n);";
    break;
  case nsmodule_type::plugin:
    ofs << "\nset_target_properties(" << target_name << " PROPERTIES "
        << cmake::k_plugin_locations << "\n);";
    break;
  default:
    return;
  }
}

void nsmodule::build_fetched_content(nsbuild const& bc) const
{
  auto src  = get_full_ext_dir(bc);
  auto xpb  = get_full_xpb_dir(bc);
  auto dsdk = get_full_sdk_dir(bc);

  nsprocess::cmake_config(bc, {}, cmake::path(src), xpb);
  nsprocess::cmake_build(bc, "", xpb);
  nsprocess::cmake_install(bc, cmake::path(dsdk), xpb);
}

std::filesystem::path nsmodule::get_full_bld_dir(nsbuild const& bc) const
{
  return bc.get_full_cfg_dir() / k_build_dir / name;
}

std::filesystem::path nsmodule::get_full_xpb_dir(nsbuild const& bc) const
{
  return bc.get_full_cfg_dir() / k_xpb_dir / name;
}

std::filesystem::path nsmodule::get_full_sbld_dir(nsbuild const& bc) const
{
  return bc.get_full_cfg_dir() / k_subbuild_dir / name;
}

std::filesystem::path nsmodule::get_full_sdk_dir(nsbuild const& bc) const
{
  return bc.get_full_cfg_dir() / k_sdk_dir / name;
}

std::filesystem::path nsmodule::get_full_dl_dir(nsbuild const& bc) const
{
  return bc.get_full_dl_dir() / name;
}

std::filesystem::path nsmodule::get_full_ts_dir(nsbuild const& bc) const
{
  return bc.get_full_cfg_dir() / k_ts_dir / name;
}

std::filesystem::path nsmodule::get_full_ext_dir(nsbuild const& bc) const
{
  return bc.get_full_cfg_dir() / k_ext_dir / name;
}

std::filesystem::path nsmodule::get_full_gen_dir(nsbuild const& bc) const
{
  return bc.get_full_cfg_dir() / k_gen_dir / name;
}
