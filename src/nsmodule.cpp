#include "nsmodule.h"

#include "nsbuild.h"
#include "nscmake.h"
#include "nscmake_conststr.h"
#include "nslog.h"
#include "nspreset.h"
#include "nsprocess.h"
#include "nstarget.h"
#include "picosha2.h"

#include <exception>
#include <fstream>
#include <sstream>

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

bool has_runtime(nsmodule_type t)
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

bool has_headers(nsmodule_type t)
{
  switch (t)
  {
  case nsmodule_type::exe:
  case nsmodule_type::lib:
  case nsmodule_type::plugin:
  case nsmodule_type::ref:
  case nsmodule_type::test:
    return true;
  default:
    return false;
  }
}

bool is_executable(nsmodule_type t) 
{ return t == nsmodule_type::exe || t == nsmodule_type::test; }

void nsmodule::process(nsbuild const& bc, std::string const& targ_name, nstarget& targ)
{
  update_properties(bc, targ_name, targ);
  if (disabled)
    return;
  update_macros(bc, targ_name, targ);
  update_fetch(bc);
  generate_plugin_manifest(bc);
}

void nsmodule::update_properties(nsbuild const& bc, std::string const& targ_name, nstarget& targ)
{
  target_name = targ_name;
  auto& fw    = bc.frameworks[targ.fw_idx];

  std::filesystem::path p = fw.source_path;
  p /= name;

  force_rebuild  = bc.state.delete_builds;
  framework_path = fw.source_path;
  framework_name = fw.name;
  source_path    = p.generic_string();
  gen_path       = cmake::path(get_full_gen_dir(bc));

  if (has_data(type))
    glob_media = nsglob{.name        = "data_group",
                        .relative_to = {}, // "${CMAKE_CURRENT_LIST_DIR}",
                        .sub_paths   = {"${module_dir}/media/*"},
                        .recurse     = true};
  if (has_data(type))
  {
    // Add a step
    nsbuildstep step;
    nsbuildcmds cmd;
    step.name = "data group";
    step.artifacts.push_back("${data_group_output}");
    step.check = "data_group";
    step.dependencies.push_back("${data_group}");
    step.injected_config.push_back(cmake::k_media_commands);
    unset.emplace_back("data_group");
    unset.emplace_back("data_group_output");
    cmd.msgs.push_back(fmt::format("Building data files for {}", name));
    cmd.command = "${nsbuild}";
    cmd.params  = "--copy-media ${module_dir}/media ${config_rt_dir}/media "
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
    step.name = "plugin manifest";
    step.artifacts.push_back(fmt::format("${{config_rt_dir}}/{1}/{0}Manifest.json", name, bc.manifests_dir));
    step.dependencies.push_back(fmt::format("${{module_gen_dir}}/{0}Manifest.json", name));
    cmd.msgs.push_back(fmt::format("Copying plugin manifest for {}", name));
    cmd.command = "${CMAKE_COMMAND}";
    cmd.params  = fmt::format("-E copy_if_different ${{module_gen_dir}}/{0}Manifest.json "
                             "${{config_rt_dir}}/{1}",
                             name, bc.manifests_dir);
    step.steps.push_back(cmd);
    step.wd = bc.wd.generic_string();
    prebuild.push_back(step);
  }
  else if (type == nsmodule_type::test)
  {    
  }

  if (has_headers(type))
  {
    nsbuildstep step;
    nsbuildcmds cmd;
    step.name = "enums generation";
    step.artifacts.push_back("${enums_json_output}");
    step.check = "has_enums_json";
    step.dependencies.push_back("${enums_json}");
    cmd.msgs.push_back(fmt::format("Generating enum for {}", name));
    cmd.command = "${nsbuild}";
    cmd.params  = "--gen-enum ${module_dir} ${config_preset_name}";
    step.steps.push_back(cmd);
    step.wd = bc.wd.generic_string();
    unset.emplace_back("has_enums_json");
    unset.emplace_back("enums_json_output");
    unset.emplace_back("enums_json");
    prebuild.push_back(step);
  }

  if (is_executable(type))
  {
    if (intf[nsmodule::priv_intf].empty())
      intf[nsmodule::priv_intf].emplace_back();
    intf[nsmodule::priv_intf].back().definitions.emplace_back("L_APP_NAME", fmt::format("\"{}\"", name));
    if (type == nsmodule_type::test)
      intf[nsmodule::priv_intf].back().definitions.emplace_back("L_TEST_APP", fmt::format("\"{}\"", name));
  }
  
  if (!bc.s_current_preset->static_libs)
  {
    if (intf[nsmodule::priv_intf].empty())
      intf[nsmodule::priv_intf].emplace_back();
    intf[nsmodule::priv_intf].back().definitions.emplace_back("LUMIERE_EXPORT_AS_DYNAMIC_LIB", "1");
  }

  if (bc.s_current_preset->build_type == "debug")
  {
    if (intf[nsmodule::priv_intf].empty())
      intf[nsmodule::priv_intf].emplace_back();
    intf[nsmodule::priv_intf].back().definitions.emplace_back("L_DEBUG", "1");
  }

  if (fetch)
  {
    if (fetch->extern_name.empty())
      fetch->extern_name = fetch->package;
    if (fetch->targets.empty())
      fetch->targets = fetch->components;
  }

  std::filesystem::create_directories(get_full_bld_dir(bc));
  std::filesystem::create_directories(get_full_gen_dir(bc));
  std::filesystem::create_directories(get_full_gen_dir(bc) / "local");
}

void nsmodule::update_macros(nsbuild const& bc, std::string const& targ_name, nstarget& targ)
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
    std::string components;
    bool        first = true;
    for (auto c : fetch->components)
    {
      if (!first)
        components += ";";
      first = false;
      components += c;
    }

    std::filesystem::path tmp = bc.get_full_cfg_dir();
    /// - dl/module contains sources
    /// - bld/module/ts Timestamp directory
    /// - bld/module/bld  Build directory
    /// - bld/module/sdk Install directory
    macros["fetch_bulid_dir"]    = cmake::path(get_full_fetch_bld_dir(bc));
    macros["fetch_subbulid_dir"] = cmake::path(get_full_fetch_sbld_dir(bc));
    macros["fetch_sdk_dir"]      = cmake::path(get_full_sdk_dir(bc));
    macros["fetch_src_dir"]      = cmake::path(get_full_dl_dir(bc));
    macros["fetch_package"]      = fetch->package;
    macros["fetch_namespace"]    = fetch->namespace_name;
    macros["fetch_repo"]         = fetch->repo;
    macros["fetch_commit"]       = fetch->commit;
    macros["fetch_components"]   = components;
    macros["fetch_extern_name"]   = fetch->extern_name;
    // macros["fetch_ts_dir"]       = cmake::path(get_full_ts_dir(bc));
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
    std::filesystem::remove_all(get_full_fetch_bld_dir(bc), ec);
    std::filesystem::remove_all(get_full_fetch_sbld_dir(bc), ec);
    std::filesystem::remove_all(get_full_bld_dir(bc), ec);
    // std::filesystem::remove_all(get_full_sdk_dir(bc), ec);
  }

  fetch_content(bc);
}

void nsmodule::generate_plugin_manifest(nsbuild const& bc)
{
  if (type != nsmodule_type::plugin)
    return;

  std::ofstream f{get_full_gen_dir(bc) / fmt::format("{0}Manifest.json", name)};
  f << "{" 
    << fmt::format(k_json_val, "author", manifest.author) << ", "
    << fmt::format(k_json_val, "company", manifest.company) << ", "
    << fmt::format(k_json_val, "binary", target_name) << ", "
    << fmt::format(k_json_val, "version", version.empty() ? bc.version : version) << ", "
    << fmt::format(k_json_val, "compatibility", manifest.compatibility) << ", "
    << fmt::format(k_json_val, "context", manifest.context) << ", "
    << fmt::format(k_json_val, "desc", manifest.desc) << ", "
    << fmt::format(k_json_bool,"optional", manifest.optional) << ", "
    << fmt::format(k_json_obj, "services");
  bool first = true;
  for (auto const& s : manifest.services)
  {
    if (!first)
      f << ", ";
    f << fmt::format(k_json_val, s.first, s.second);
    first = false;
  }
  f << "\n}\n}";
}

std::string nsmodule::write_fetch_build(nsbuild const& bc) const
{
  auto              ext_dir = get_full_ext_dir(bc);
  auto              cmlf    = ext_dir / "CMakeLists.txt";
  std::stringstream ofs;

  std::filesystem::create_directories(ext_dir);
  {

    if (!ofs)
    {
      nslog::error(fmt::format("Failed to write to : {}", cmlf.generic_string()));
      throw std::runtime_error("Could not create CMakeLists.txt");
    }

    ofs << fmt::format(cmake::k_project_name, name, bc.version);
    ofs << fmt::format("\nlist(PREPEND CMAKE_MODULE_PATH \"{}\")", cmake::path(get_full_sdk_dir(bc)));
    bc.macros.print(ofs);
    macros.print(ofs);
    // write_variables(ofs, bc);
    for (auto const& a : fetch->args)
    {
      a.print(ofs, output_fmt::cmake_def, false);
    }

    std::filesystem::path src = source_path;

    // Do we have prepare 
    auto prepare = src / "Prepare.cmake";
    if (std::filesystem::exists(prepare) || !fetch->prepare.fragments.empty())
    {
      if (std::filesystem::exists(prepare))
      {
        std::ifstream t{prepare};
        std::string   buffer((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        ofs << buffer;
      }

      if (!fetch->prepare.fragments.empty())
      {
        for (auto const& f : fetch->prepare.fragments)
          ofs << f << "\n";
      }
    }

    // Do we have build
    auto                  srccmk = src / "Build.cmake";
    if (fetch->fetch_content)
    {
      ofs << cmake::k_fetch_content;
    }
    else if (!fetch->custom_build.fragments.empty() || std::filesystem::exists(srccmk))
    {
      if (!fetch->custom_build.fragments.empty())
      {
        for (auto const& f : fetch->custom_build.fragments)
          ofs << f << "\n";
      }

      if (std::filesystem::exists(srccmk))
      {
        std::ifstream t{srccmk};
        std::string buffer((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        ofs << buffer;
      }
      ofs << cmake::k_ext_proj_custom;
    }
    else
    { 
      ofs << cmake::k_ext_cmake_proj_start;
      for (auto const& a : fetch->args)
      {
        for (auto const& v : a.params)
          ofs << fmt::format("\n    -D{0}=${{{0}}}", v.name);
      }
      ofs << "\n)\n";
    }

    // Do we have install
    auto packageinstall = src / "PackageInstall.cmake";
    if (std::filesystem::exists(packageinstall) || !fetch->package_install.fragments.empty())
    {
      if (std::filesystem::exists(packageinstall))
      {
        std::ifstream t{packageinstall};
        std::string   buffer((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        ofs << buffer;
      }

      if (!fetch->package_install.fragments.empty())
      {
        for (auto const& f : fetch->package_install.fragments)
          ofs << f << "\n";
      }
    }

    // Do we have post build install
    auto postinstall = src / "PostBuildInstall.cmake";
    if (std::filesystem::exists(postinstall) || !fetch->post_build_install.fragments.empty())
    {
      if (std::filesystem::exists(postinstall))
      {
        std::ifstream t{postinstall};
        std::string   buffer((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        ofs << buffer;
      }
      
      if (!fetch->post_build_install.fragments.empty())
      {
        for (auto const& f : fetch->post_build_install.fragments)
          ofs << f << "\n";
      }
    }
  }
  {
    auto          content = ofs.str();
    std::ofstream ffs{cmlf};
    ffs << content;
    std::string sha;
    picosha2::hash256_hex_string(content, sha);

    auto          cmlf = ext_dir / "CMakePresets.json";
    std::ofstream ofs{cmlf};
    nspreset::write(ofs, nspreset::write_compiler_paths, cmake::path(get_full_ext_bld_dir(bc)), {}, bc);
    return sha;
  }
}

void nsmodule::fetch_content(nsbuild const& bc)
{
  std::string sha;
  
  if ((regenerate || force_rebuild))
    sha = write_fetch_build(bc);
  
  if (bc.cmakeinfo.cmake_skip_fetch_builds)
  {
    write_fetch_meta(bc, sha);
    return;
  }

  if (sha.empty())
    return;

  if (fetch_changed(bc, sha))
  {
    nslog::print(fmt::format("Rebuilding {}..", name));
    build_fetched_content(bc);
    write_fetch_meta(bc, sha);
    was_fetch_rebuilt = true;
  }
}

bool nsmodule::fetch_changed(nsbuild const& bc, std::string const& last_sha) const 
{
  auto meta = bc.get_full_cache_dir() / fmt::format("{}.fetch", name);
  std::ifstream iff{meta};
  if (iff.is_open())
  {
    std::string sha;
    iff >> sha;
    if (sha == last_sha)
      return false;
  }
  return true;
}

void nsmodule::write_fetch_meta(nsbuild const& bc, std::string const& last_sha) const 
{
  if (last_sha.empty())
    return;
  auto          meta = bc.get_full_cache_dir() / fmt::format("{}.fetch", name);
  std::ofstream off{meta};
  if (off.is_open())
  {
    off << last_sha;
  }
}

void nsmodule::write_main_build(std::ostream& ofs, nsbuild const& bc) const
{
  if (disabled)
  {
    ofs << fmt::format("\nadd_library({} INTERFACE)", target_name);
    return;
  }
  cmake::line(ofs, "variables");
  macros.print(ofs);
  write_variables(ofs, bc);
  cmake::line(ofs, "globs");
  if (has_data(type))
    glob_media.print(ofs);
  write_sources(ofs, bc);
  write_target(ofs, bc, target_name);
  write_includes(ofs, bc);
  write_definitions(ofs, bc);
  write_find_package(ofs, bc);
  write_dependencies(ofs, bc);
  write_linklibs(ofs, bc);
  write_enums_init(ofs, bc);
  write_prebuild_steps(ofs, bc);
  write_postbuild_steps(ofs, bc);
  write_runtime_settings(ofs, bc);
  write_cxx_options(ofs, bc);
  write_final_config(ofs, bc);
  write_install_command(ofs, bc);
  for (auto const& u : unset)
    ofs << fmt::format("\nunset({})", u);
  // config file
}

void nsmodule::write_variables(std::ostream& ofs, nsbuild const& bc, char sep) const
{
  for (auto const& e : exports)
  {
    e.print(ofs, output_fmt::set_cache, false, sep);
  }
  for (auto const& e : vars)
  {
    e.print(ofs, output_fmt::cmake_def, false, sep);
  }
}

void nsmodule::write_sources(std::ostream& ofs, nsbuild const& bc) const
{
  if (has_runtime(type))
  {
    nsglob glob_sources;
    glob_sources =
        nsglob{.name        = "__module_sources",
               .relative_to = {}, //"${CMAKE_CURRENT_LIST_DIR}",
               .sub_paths   = {},
               .recurse     = false};
    write_source_subpath(glob_sources, bc);
    glob_sources.print(ofs);
  }
}

void nsmodule::write_source_subpath(nsglob& glob, nsbuild const& bc) const
{
  glob.sub_paths.push_back(source_path + "/src/*.cpp");
  glob.sub_paths.push_back(gen_path + "/*.cpp");
  glob.sub_paths.push_back(gen_path + "/local/*.cpp");
  for (auto const& r : references)
  {
    auto const& m = bc.get_module(r);
    m.write_source_subpath(glob, bc);
  }
}

void nsmodule::write_target(std::ostream& ofs, nsbuild const& bc, std::string const& name) const
{
  cmake::line(ofs, "target");
  switch (type)
  {
  case nsmodule_type::data:
    ofs << fmt::format("\nadd_custom_target({} ALL SOURCES ${{data_group}})", name);
    break;
  case nsmodule_type::exe:
    ofs << fmt::format("\nadd_executable({} {} ${{__module_sources}})", name,
                       console_app ? "${__nsbuild_console_app_options}" : "${__nsbuild_app_options}");
    break;
  case nsmodule_type::ref:
    ofs << fmt::format("\nadd_library({} INTERFACE)", name);
    break;
  case nsmodule_type::external:
    ofs << fmt::format("\nadd_library({} INTERFACE IMPORTED GLOBAL)", name);
    break;
  case nsmodule_type::lib:
    if (bc.s_current_preset->static_libs)
      ofs << fmt::format("\nadd_library({} STATIC ${{__module_sources}})", name);
    else
      ofs << fmt::format("\nadd_library({} SHARED ${{__module_sources}})", name);
    break;
  case nsmodule_type::plugin:
    ofs << fmt::format("\nadd_library({} MODULE ${{__module_sources}})", name);
    break;
  case nsmodule_type::test:
    ofs << fmt::format("\nadd_executable({} {} ${{__module_sources}})", name,
                       console_app ? "${__nsbuild_console_app_options}" : "${__nsbuild_app_options}");
    break;
  default:
    break;
  }
  if (has_runtime(type))
    ofs << "\nunset(__module_sources)";
}

void nsmodule::write_enums_init(std::ostream& ofs, nsbuild const& bc) const 
{ 
  if (!has_headers(type))
    return;
  cmake::line(ofs, "enums");
  ofs << cmake::k_enums_json;
}

void nsmodule::write_prebuild_steps(std::ostream& ofs, const nsbuild& bc) const
{
  if (!prebuild.empty())
  {
    cmake::line(ofs, "prebuild-steps");
    for (auto const& step : prebuild)
      step.print(ofs, bc, *this);

    ofs << cmake::k_begin_prebuild_steps;
    for (auto const& step : prebuild)
    {
      for (auto const& d : step.artifacts)
        ofs << fmt::format("\nlist(APPEND module_prebuild_artifacts {})", d);
    }
    ofs << cmake::k_finalize_prebuild_steps;
  }
}

void nsmodule::write_postbuild_steps(std::ostream& ofs, nsbuild const& bc) const
{
  if (!postbuild.empty())
  {
    cmake::line(ofs, "postbuild-steps");
    ofs << fmt::format("\nadd_custom_command(TARGET ${{module_target}} POST_BUILD ");
    for (auto const& step : postbuild)
    {
      for (auto const& s : step.steps)
      {
        ofs << "\n  COMMAND " << s.command << " " << s.params;
        for (auto const& m : s.msgs)
          ofs << "\n  COMMAND ${CMAKE_COMMAND} -E echo \"" << m << "\"";
      }
    }
    ofs << "\n\t)";
  }
}

void nsmodule::write_cxx_options(std::ostream& ofs, nsbuild const& bc) const
{
  if (!has_runtime(type))
    return;
  cmake::line(ofs, "cxx-options");
  ofs << "\ntarget_compile_options(${module_target} PRIVATE "
         "${__module_cxx_compile_flags})";
  ofs << "\ntarget_link_options(${module_target} PRIVATE "
         "${__module_cxx_linker_flags})";
}

void nsmodule::write_includes(std::ostream& ofs, nsbuild const& bc) const
{
  switch (type)
  {
  case nsmodule_type::data:
    break;
  case nsmodule_type::ref:
    break;
  case nsmodule_type::external:
    break;
  case nsmodule_type::test:
  case nsmodule_type::exe:
  case nsmodule_type::lib:
  case nsmodule_type::plugin:
    cmake::line(ofs, "includes");
    write_include(ofs, "${module_dir}", "include", cmake::inheritance::pub);
    write_include(ofs, "${module_dir}", "local_include", cmake::inheritance::priv);
    write_include(ofs, "${module_gen_dir}", "", cmake::inheritance::pub);
    write_include(ofs, "${module_gen_dir}", "local", cmake::inheritance::priv);
    write_refs_includes(ofs, bc, *this);
  default:
    break;
  }
}

void nsmodule::write_include(std::ostream& ofs, std::string_view path, std::string_view subpath,
                             cmake::inheritance inherit) const
{
  // Assume build
  ofs << fmt::format("\nif(EXISTS \"{}/{}\")", path, subpath);
  ofs << "\n\ttarget_include_directories(${module_target} " << cmake::to_string(inherit);
  if (subpath.empty())
    ofs << fmt::format("\n\t\t$<BUILD_INTERFACE:{}>", path);
  else
    ofs << fmt::format("\n\t\t$<BUILD_INTERFACE:{}/{}>", path, subpath);

  // if (expo == cmake::exposition::install)
  //  ofs << "\n\t\t$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}}>");
  ofs << "\n)\nendif()";
}

void nsmodule::write_refs_includes(std::ostream& ofs, nsbuild const& bc, nsmodule const& target) const
{
  for (auto const& r : references)
  {
    auto const& m = bc.get_module(r);
    m.write_refs_includes(ofs, bc, target);
    target.write_include(ofs, m.source_path, "include",
                         target.type == nsmodule_type::plugin ? cmake::inheritance::priv : cmake::inheritance::pub);
    target.write_include(ofs, m.source_path, "local_include", cmake::inheritance::priv);
    target.write_include(ofs, m.gen_path, "",
                         target.type == nsmodule_type::plugin ? cmake::inheritance::priv : cmake::inheritance::pub);
    target.write_include(ofs, m.gen_path, "local", cmake::inheritance::priv);
  }
}

void nsmodule::write_find_package(std::ostream& ofs, nsbuild const& bc) const
{
  if (!fetch || fetch->runtime_only)
    return;
  cmake::line(ofs, "find-package");
  if (fetch->components.empty())
    ofs << fmt::format(cmake::k_find_package, fetch->package, fetch->version);
  else
  {

    ofs << fmt::format(cmake::k_find_package_comp_start, fetch->package, fetch->version);
    for (auto const& c : fetch->components)
      ofs << c << " ";
    ofs << cmake::k_find_package_comp_end;
  }
 
  auto namespace_name = fetch->namespace_name.empty() ? fetch->package : fetch->namespace_name;
  if (!fetch->legacy_linking)
  {
    ofs << "\ntarget_link_libraries(${module_target} \n  " << cmake::to_string(cmake::inheritance::intf);
    if (fetch->targets.empty())
    {
      if (fetch->skip_namespace)
        ofs << fmt::format("\n   {}", fetch->package);
      else
        ofs << fmt::format("\n   {}::{}", namespace_name, fetch->package);
    }
    else
    {
      for (auto const& c : fetch->targets)
        ofs << fmt::format("\n   {}::{}", namespace_name, c);
    }
    ofs << "\n)";
  }
  else
  {
    ofs << fmt::format("\nset(__sdk_install_includes ${{{}_INCLUDE_DIRS}})", fetch->package);
    ofs << "\nlist(TRANSFORM __sdk_install_includes REPLACE ${fetch_sdk_dir} "
           "\"\")";
    ofs << fmt::format("\nset(__sdk_install_libraries ${{{}_LIBRARIES}})", fetch->package);
    ofs << "\nlist(TRANSFORM __sdk_install_libraries REPLACE ${fetch_sdk_dir} "
           "\"\")";

    ofs << "\ntarget_include_directories(${module_target} " << cmake::to_string(cmake::inheritance::intf)
        << fmt::format("\n\t$<BUILD_INTERFACE:\"${{{}_INCLUDE_DIR}}\">", fetch->package)
        << "\n\t$<INSTALL_INTERFACE:\"${__sdk_install_includes}\">"
        << "\n)"
        << "\ntarget_link_libraries(${module_target} " << cmake::to_string(cmake::inheritance::intf)
        << fmt::format("\n\t$<BUILD_INTERFACE:\"${{{}_LIBRARIES}}\">", fetch->package)
        << "\n\t$<INSTALL_INTERFACE:\"${__sdk_install_libraries}\">"
        << "\n)";
  }
}

void nsmodule::write_definitions(std::ostream& ofs, nsbuild const& bc) const
{
  switch (type)
  {
  case nsmodule_type::data:
    break;
  case nsmodule_type::plugin:
  case nsmodule_type::lib:
  case nsmodule_type::exe:
  case nsmodule_type::test:
    cmake::line(ofs, "definitions");
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

void nsmodule::write_definitions_mod(std::ostream& ofs, nsbuild const& bc) const
{
  write_definitions(ofs, bc, 0);
  write_definitions(ofs, bc, 1);
  write_refs_definitions(ofs, bc, *this);
  write_definitions(ofs, fmt::format("CurrentModule_{}", cmake::value(name)), cmake::inheritance::priv, "");
}

void nsmodule::write_definitions(std::ostream& ofs, nsbuild const& bc, std::uint32_t type) const
{
  for (std::size_t i = 0; i < intf[type].size(); ++i)
  {
    auto        filter = intf[type][i].filters;
    auto const& dep    = intf[type][i].definitions;
    for (auto const& d : dep)
    {
      auto def = fmt::format("{}={}", d.first, d.second);
      write_definitions(ofs, def, type == pub_intf ? cmake::inheritance::pub : cmake::inheritance::priv, filter);
    }
  }
}

void nsmodule::write_definitions(std::ostream& ofs, std::string_view def, cmake::inheritance inherit,
                                 std::string_view filter) const
{
  ofs << fmt::format("\ntarget_compile_definitions(${{module_target}} {} {})", cmake::to_string(inherit),
                     filter.empty() ? std::string{def} : fmt::format("$<{}:{}>", filter, def));
}

void nsmodule::write_refs_definitions(std::ostream& ofs, nsbuild const& bc, nsmodule const& target) const
{
  for (auto const& r : references)
  {
    auto const& m = bc.get_module(r);
    m.write_refs_definitions(ofs, bc, target);
    m.write_definitions_mod(ofs, bc);
  }
}

void nsmodule::write_dependencies(std::ostream& ofs, nsbuild const& bc) const
{
  switch (type)
  {
  case nsmodule_type::data:
    break;
  case nsmodule_type::plugin:
  case nsmodule_type::lib:
  case nsmodule_type::exe:
  case nsmodule_type::test:
    cmake::line(ofs, "dependencies");
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

void nsmodule::write_dependencies_begin(std::ostream& ofs, nsbuild const& bc) const
{
  ofs << "\nset(__module_priv_deps)";
  ofs << "\nset(__module_pub_deps)";
  ofs << "\nset(__module_ref_deps)";
  ofs << "\nset(__module_priv_link_libs)";
  ofs << "\nset(__module_pub_link_libs)";
}

void nsmodule::write_dependencies_mod(std::ostream& ofs, nsbuild const& bc) const
{
  write_dependencies(ofs, bc, pub_intf);
  write_dependencies(ofs, bc, priv_intf);
  write_refs_dependencies(ofs, bc, *this);
}

void nsmodule::write_dependencies_end(std::ostream& ofs, nsbuild const& bc) const { ofs << cmake::k_write_dependency; }

void nsmodule::write_dependencies(std::ostream& ofs, nsbuild const& bc, std::uint32_t type) const
{
  for (std::size_t i = 0; i < intf[type].size(); ++i)
  {
    auto        filter = intf[type][i].filters;
    auto const& dep    = intf[type][i].dependencies;
    for (auto const& d : dep)
    {
      write_dependency(ofs, d, type == pub_intf ? cmake::inheritance::pub : cmake::inheritance::priv, filter);
      auto const& mod = bc.get_module(d);
      if (mod.type == nsmodule_type::lib || mod.type == nsmodule_type::external || mod.type == nsmodule_type::ref)
        write_target_link_libs(ofs, d, type == pub_intf ? cmake::inheritance::pub : cmake::inheritance::priv, filter);
    }
  }
}

void nsmodule::write_dependency(std::ostream& ofs, std::string_view target, cmake::inheritance inh,
                                std::string_view filter) const
{
  ofs << fmt::format("\nlist(APPEND {} {})",
                     inh == cmake::inheritance::pub ? "__module_pub_deps" : "__module_priv_deps",
                     filter.empty() ? std::string{target} : fmt::format("$<{}:{}>", filter, target));
}

void nsmodule::write_target_link_libs(std::ostream& ofs, std::string_view target, cmake::inheritance inh,
                                      std::string_view filter) const
{
  ofs << fmt::format("\nlist(APPEND {} {})",
                     inh == cmake::inheritance::pub ? "__module_pub_link_libs" : "__module_priv_link_libs",
                     filter.empty() ? std::string{target} : fmt::format("$<{}:{}>", filter, target));
}

void nsmodule::write_refs_dependencies(std::ostream& ofs, nsbuild const& bc, nsmodule const& target) const
{
  for (auto const& r : references)
  {
    auto const& m = bc.get_module(r);
    m.write_refs_dependencies(ofs, bc, target);
    m.write_dependencies_mod(ofs, bc);
    ofs << fmt::format("\nlist(APPEND __module_ref_deps {})", r);
  }
}

void nsmodule::write_linklibs(std::ostream& ofs, nsbuild const& bc) const
{
  switch (type)
  {
  case nsmodule_type::data:
    break;
  case nsmodule_type::plugin:
  case nsmodule_type::lib:
  case nsmodule_type::exe:
  case nsmodule_type::test:
    cmake::line(ofs, "linklibs");
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

void nsmodule::write_linklibs_begin(std::ostream& ofs, nsbuild const& bc) const
{
  ofs << "\nset(__module_priv_libs)";
  ofs << "\nset(__module_pub_libs)";
}

void nsmodule::write_linklibs_mod(std::ostream& ofs, nsbuild const& bc) const
{
  write_linklibs(ofs, bc, pub_intf);
  write_linklibs(ofs, bc, priv_intf);
  write_refs_linklibs(ofs, bc, *this);
}

void nsmodule::write_linklibs_end(std::ostream& ofs, nsbuild const& bc) const { ofs << cmake::k_write_libs; }

void nsmodule::write_linklibs(std::ostream& ofs, nsbuild const& bc, std::uint32_t type) const
{
  for (std::size_t i = 0; i < intf[type].size(); ++i)
  {
    auto        filter = intf[type][i].filters;
    auto const& libs   = intf[type][i].sys_libraries;
    for (auto const& d : libs)
      write_linklibs(ofs, d, type == pub_intf ? cmake::inheritance::pub : cmake::inheritance::priv, filter);
  }
}

void nsmodule::write_linklibs(std::ostream& ofs, std::string_view target, cmake::inheritance inh,
                              std::string_view filter) const
{
  ofs << fmt::format("\nlist(APPEND {} {})",
                     inh == cmake::inheritance::pub ? "__module_pub_libs" : "__module_priv_libs",
                     filter.empty() ? std::string{target} : fmt::format("$<{}:{}>", filter, target));
}

void nsmodule::write_refs_linklibs(std::ostream& ofs, nsbuild const& bc, nsmodule const& target) const
{
  for (auto const& r : references)
  {
    auto const& m = bc.get_module(r);
    m.write_refs_linklibs(ofs, bc, target);
    m.write_linklibs_mod(ofs, bc);
  }
}

void nsmodule::write_install_command(std::ostream& ofs, nsbuild const& bc) const
{
  // cmake::line(ofs, "installation");
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

void nsmodule::write_final_config(std::ostream& ofs, nsbuild const& bc) const
{
  switch (type)
  {
  case nsmodule_type::data:
  case nsmodule_type::external:
    return;
  case nsmodule_type::exe:
  case nsmodule_type::lib:
  case nsmodule_type::plugin:
  case nsmodule_type::ref:
    cmake::line(ofs, "final-config");
    ofs << cmake::k_finale;
    break;
  case nsmodule_type::test:
    cmake::line(ofs, "final-config");
    ofs << cmake::k_finale;
    write_tests(ofs, bc);
    break;
  default:
    return;
  }
}

void nsmodule::write_tests(std::ostream& ofs, nsbuild const& bc) const
{
  for (auto const& t : tests)
  {
    ofs << fmt::format("\nadd_test(NAME {}\n  COMMAND ${{config_rt_dir}}/bin/{} ", t.name, target_name);
    ofs << fmt::format("--test={} ", t.name);
    bool first = true;
    for (auto const& p : t.parameters)
    {
      if (first)
      {
        first = false;
        continue;
      }
      ofs << fmt::format("--test-param-{}={} ", p.first, p.second);
    }
    ofs << "\n  WORKING_DIRECTORY ${config_rt_dir}/bin)";
  }
}

void nsmodule::write_runtime_settings(std::ostream& ofs, nsbuild const& bc) const
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
    cmake::line(ofs, "runtime-settings");
    ofs << "\nset_target_properties(" << target_name << " PROPERTIES " << cmake::k_rt_locations << ")";
    break;
  case nsmodule_type::plugin:
    cmake::line(ofs, "runtime-settings");
    ofs << "\nset_target_properties(" << target_name << " PROPERTIES " << fmt::format(cmake::k_plugin_locations, bc.plugin_dir) << ")";
    break;
  default:
    return;
  }
}

void nsmodule::build_fetched_content(nsbuild const& bc) const
{
  auto src  = get_full_ext_dir(bc);
  auto xpb  = get_full_ext_bld_dir(bc);
  auto dsdk = get_full_sdk_dir(bc);

  nsprocess::cmake_config(bc, {}, cmake::path(src), xpb);
  nsprocess::cmake_build(bc, "", xpb);
  nsprocess::cmake_install(bc, cmake::path(dsdk), xpb);
  // custom location copy
  if (!fetch->runtime_loc.empty())
  {
    for (auto const& l : fetch->runtime_loc)
    {
      namespace fs = std::filesystem;

      auto       it           = fs::directory_iterator{get_full_sdk_dir(bc) / l};
      const auto copy_options = fs::copy_options::update_existing | fs::copy_options::recursive;
      for (auto const& dir_entry : it)
      {
        if (!dir_entry.is_regular_file() && !dir_entry.is_symlink())
          continue;
        auto name = dir_entry.path().filename().generic_string();
        bool copy = false;
        if (std::regex_search(name, bc.dll_ext))
          std::filesystem::copy(dir_entry.path(), bc.get_full_rt_dir() / dir_entry.path().filename(), copy_options);

        if (fetch->runtime_files.empty())
          continue;
        name = dir_entry.path().generic_string();
        for (auto const& rt : fetch->runtime_files)
        {
          std::smatch match;
          if (!std::regex_search(name, match, bc.dll_ext))
            continue;
          if (match.empty())
            continue;
          std::filesystem::copy(dir_entry.path(), bc.get_full_rt_dir() / match[0].str(), copy_options);
          break;
        }
      }
    }
  }
}

std::filesystem::path nsmodule::get_full_bld_dir(nsbuild const& bc) const { return bc.get_full_build_dir() / name; }

std::filesystem::path nsmodule::get_full_fetch_bld_dir(nsbuild const& bc) const
{
  return bc.get_full_build_dir() / fmt::format("{}.dl", name);
}

std::filesystem::path nsmodule::get_full_fetch_sbld_dir(nsbuild const& bc) const
{
  return bc.get_full_build_dir() / fmt::format("{}.sb", name);
}

std::filesystem::path nsmodule::get_full_sdk_dir(nsbuild const& bc) const { return bc.get_full_sdk_dir(); }

std::filesystem::path nsmodule::get_full_dl_dir(nsbuild const& bc) const { return bc.get_full_dl_dir() / name; }

std::filesystem::path nsmodule::get_full_ext_bld_dir(nsbuild const& bc) const
{
  return bc.get_full_build_dir() / fmt::format("{}.ext", name);
}

std::filesystem::path nsmodule::get_full_ext_dir(nsbuild const& bc) const
{
  return bc.get_full_cmake_gen_dir() / name;
}

std::filesystem::path nsmodule::get_full_gen_dir(nsbuild const& bc) const
{
  return bc.get_full_cfg_dir() / k_gen_dir / name;
}
