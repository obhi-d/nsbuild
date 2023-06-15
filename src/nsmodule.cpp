#include "nsmodule.h"

#include "nsbuild.h"
#include "nscmake.h"
#include "nscmake_conststr.h"
#include "nsenums.h"
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

bool is_executable(nsmodule_type t) { return t == nsmodule_type::exe || t == nsmodule_type::test; }

void nsmodule::process(nsbuild const& bc, std::string const& targ_name, nstarget& targ)
{
  update_properties(bc, targ_name, targ);
  if (disabled)
    return;
  update_macros(bc, targ_name, targ);
  update_fetch(bc);
  generate_plugin_manifest(bc);
}

void nsmodule::check_enums(nsbuild const& bc) const
{
  auto lenums = std::filesystem::path(source_path) / "private" / "Enums.json";
  auto enums  = std::filesystem::path(source_path) / "public" / "Enums.json";

  std::string content;
  size_t      lenums_size = std::filesystem::exists(lenums) ? std::filesystem::file_size(lenums) : 0;
  size_t      enums_size  = std::filesystem::exists(enums) ? std::filesystem::file_size(enums) : 0;

  if (!lenums_size && !enums_size)
  {
    nsenum_context::clean(*this, bc);
    if (sha_changed(bc, "enums", ""))
    {
      write_sha_changed(bc, "enums", "");
    }
  }
  content.resize(lenums_size + enums_size, ' ');
  if (std::filesystem::exists(lenums))
  {
    std::ifstream iff{lenums};
    if (iff.is_open())
      iff.read(content.data(), lenums_size);
  }

  if (std::filesystem::exists(enums))
  {
    std::ifstream iff{enums};
    if (iff.is_open())
      iff.read(content.data() + lenums_size, enums_size);
  }

  std::string sha;
  picosha2::hash256_hex_string(content, sha);
  if (sha_changed(bc, "enums", sha) || nsenum_context::outputs_missing(*this, bc, lenums_size > 0, enums_size > 0))
  {
    nsenum_context::clean(*this, bc);
    write_sha_changed(bc, "enums", sha);

    nsenum_context ctx;

    ctx.local_enums_json = std::string_view(content.data(), lenums_size);
    ctx.enums_json       = std::string_view(content.data() + lenums_size, enums_size);

    ctx.generate(*this, bc);
  }
}

void nsmodule::update_properties(nsbuild const& bc, std::string const& targ_name, nstarget& targ)
{
  target_name = targ_name;
  auto& fw    = bc.frameworks[targ.fw_idx];

  std::filesystem::path p = fw.source_path;
  p /= name;

  force_build    = bc.state.full_regenerate;
  framework_path = fw.source_path;
  framework_name = fw.name;
  source_path    = p.generic_string();
  gen_path       = cmake::path(get_full_gen_dir(bc));

  if (has_data(type))
  {
    glob_media.add_set(p / bc.media_name);
    glob_media.recurse              = true;
    glob_media.path_exclude_filters = bc.media_exclude_filter;
    glob_media.accumulate();
    if (has_globs_changed |= sha_changed(bc, "data_group", glob_media.sha))
      write_sha_changed(bc, "data_group", glob_media.sha);
  }

  if (has_data(type))
  {
    // Add a step
    nsbuildstep step;
    nsbuildcmds cmd;
    step.name = "data group";
    step.artifacts.push_back("${data_group_output}");
    step.check = "data_group";
    step.dependencies.push_back("${data_group}");
    step.injected_config_body = "if(data_group)";
    step.injected_config_end  = "endif()";
    unset.emplace_back("data_group");
    cmd.msgs.push_back(fmt::format("Building data files for {}", name));
    cmd.command = "${nsbuild}";
    cmd.params  = fmt::format("--copy-media ${{module_dir}}/{0} ${{config_rt_dir}}/{0} ${{module_gen_dir}}/media.files "
                               "${{config_ignored_media}}",
                              bc.media_name);

    step.steps.push_back(cmd);
    step.wd = bc.wd.generic_string();
    prebuild.push_back(step);
  }

  for (size_t c = 0; c < contents.size(); ++c)
  {
    nsbuildstep step;
    nsbuildcmds cmd;
    step.name = "copy content";
    step.artifacts.push_back(contents[c].name);
    step.dependencies.push_back(fmt::format("${{module_gen_dir}}/c{0}.txt", c));
    cmd.msgs.push_back(fmt::format("Copying plugin manifest for {}", name));
    cmd.command = "${CMAKE_COMMAND}";
    cmd.params  = fmt::format("-E copy_if_different \"${{module_gen_dir}}/c{0}.txt\" \"{1}\" ", c, contents[c].name);
    step.steps.push_back(cmd);
    step.wd = bc.wd.generic_string();
    postbuild.push_back(step);
  }

  if (has_headers(type))
    check_enums(bc);

  if (has_runtime(type))
  {
    static std::unordered_set<std::string> cpp_filters = {".cpp", ".cxx"};
    glob_sources.file_filters                          = &cpp_filters;
    gather_sources(glob_sources, bc);
    glob_sources.accumulate();
    if (has_globs_changed |= sha_changed(bc, "src", glob_media.sha))
      write_sha_changed(bc, "src", glob_media.sha);
  }

  if (intf[nsmodule::priv_intf].empty())
    intf[nsmodule::priv_intf].emplace_back();

  intf[nsmodule::priv_intf].back().definitions.emplace_back("BC_CONFIG_HEADER", fmt::format("\"{}ModuleConfig.hpp\"", name));

  if (!bc.s_current_preset->static_libs)
  {
    intf[nsmodule::priv_intf].back().definitions.emplace_back("BC_SHARED_LIBS", "1");
  }

  if (bc.s_current_preset->build_type == "debug")
  {
    intf[nsmodule::priv_intf].back().definitions.emplace_back("BC_DEBUG_BUILD", "1");
  }

  if (fetch)
  {
    if (fetch->extern_name.empty())
      fetch->extern_name = fetch->package;
    if (fetch->targets.empty())
      fetch->targets = fetch->components;
  }

  std::filesystem::create_directories(get_full_gen_dir(bc));
  std::filesystem::create_directories(get_full_gen_dir(bc) / "local");
}

void nsmodule::update_macros(nsbuild const& bc, std::string const& targ_name, nstarget& targ)
{
  macros.fallback          = &bc.macros;
  macros["framework_dir"]  = framework_path;
  macros["framework_name"] = framework_name;
  macros["module_name"]    = name;
  macros["module_dir"]     = source_path;
  macros["module_gen_dir"] = cmake::path(get_full_gen_dir(bc));
  macros["module_target"]  = targ_name;

  std::string tags = this->tags;
  tags += bc.s_current_preset->tags;
  if (type == nsmodule_type::test)
    tags += bc.test_tags;

  macros["module_tags"] = tags;

  if (is_executable(type))
    macros["module_extras"] =
        fmt::format(R"(constexpr std::string_view AppName = \"{}\";\n  constexpr std::string_view OrgName = \"{}\";\n)",
                    name, org_name);
  else
    macros["module_extras"] = "";

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
    macros["fetch_bulid_dir"]   = cmake::path(get_full_bld_dir(bc));
    macros["fetch_sdk_dir"]     = cmake::path(get_full_sdk_dir(bc));
    macros["fetch_src_dir"]     = cmake::path(get_fetch_src_dir(bc));
    macros["fetch_package"]     = fetch->package;
    macros["fetch_namespace"]   = fetch->namespace_name;
    macros["fetch_repo"]        = fetch->repo;
    macros["fetch_commit"]      = fetch->tag;
    macros["fetch_components"]  = components;
    macros["fetch_extern_name"] = fetch->extern_name;

    // macros["fetch_ts_dir"]       = cmake::path(get_full_ts_dir(bc));
  }
  if (!custom_target_name.empty())
    target_name = custom_target_name;
  else if (bc.has_naming())
  {
    std::ostringstream ss;
    macros.im_print(ss, bc.naming());
    target_name             = std::move(ss.str());
    macros["module_target"] = target_name;
  }
}

void nsmodule::delete_build(nsbuild const& bc)
{
  if (deleted)
    return;
  std::error_code ec;
  if (fetch)
  {
    nslog::print(fmt::format("Deleting Fetch : {}", name));
    std::filesystem::remove(get_full_fetch_file(bc), ec);
    nsbuild::remove_cache(get_full_dl_dir(bc));
    nsbuild::remove_cache(get_full_bld_dir(bc));
    if (std::filesystem::exists(get_full_bld_dir(bc) / "install_manifest.txt"))
    {
      std::ifstream install_manifest(get_full_bld_dir(bc) / "install_manifest.txt");
      std::string   line;

      if (install_manifest)
      {
        while (std::getline(install_manifest, line))
        {
          nslog::print(fmt::format("Deleting : {}", line));
          std::filesystem::remove(line, ec);
        }
      }
    }
  }
  regenerate  = true;
  force_build = true;
  deleted     = true;
}

void nsmodule::update_fetch(nsbuild const& bc)
{
  if (!fetch)
    return;
  auto dl = get_fetch_src_dir(bc);
  if (!std::filesystem::exists(dl))
    regenerate = true;

  std::error_code ec;
  fetch_content(bc);
}

void nsmodule::generate_plugin_manifest(nsbuild const& bc)
{
  if (type != nsmodule_type::plugin)
    return;

  std::ofstream f{get_full_gen_dir(bc) / fmt::format("{0}Manifest.json", name)};
  f << "{" << fmt::format(k_json_val, "author", manifest.author) << ", "
    << fmt::format(k_json_val, "company", manifest.company) << ", " << fmt::format(k_json_val, "binary", target_name)
    << ", " << fmt::format(k_json_val, "version", version.empty() ? bc.version : version) << ", "
    << fmt::format(k_json_val, "compatibility", manifest.compatibility) << ", "
    << fmt::format(k_json_val, "context", manifest.context) << ", " << fmt::format(k_json_val, "desc", manifest.desc)
    << ", " << fmt::format(k_json_bool, "optional", manifest.optional) << ", " << fmt::format(k_json_obj, "services");
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

void nsmodule::backup_fetch_lists(nsbuild const& bc) const
{
  auto cmakelists   = get_fetch_src_dir(bc) / "CMakeLists.txt";
  auto cmakepresets = get_fetch_src_dir(bc) / "CMakePresets.json";
  namespace fs      = std::filesystem;
  if (fs::exists(cmakelists))
    fs::copy(cmakelists, get_fetch_src_dir(bc) / "CMakeLists.txt");
  if (fs::exists(cmakepresets))
    fs::copy(cmakepresets, get_fetch_src_dir(bc) / "CMakePresets.txt");
}

void nsmodule::restore_fetch_lists(nsbuild const& bc) const
{
  auto cmakelists   = get_full_gen_dir(bc) / "CMakeLists.txt";
  auto cmakepresets = get_full_gen_dir(bc) / "CMakePresets.json";
  namespace fs      = std::filesystem;
  if (fs::exists(cmakelists))
    fs::copy(cmakelists, get_fetch_src_dir(bc) / "CMakeLists.txt");
  if (fs::exists(cmakepresets))
    fs::copy(cmakepresets, get_fetch_src_dir(bc) / "CMakePresets.txt");
}

void nsmodule::write_fetch_build_content(nsbuild const& bc, content const& cc) const
{
  // backup
  auto cmakelists   = get_fetch_src_dir(bc) / "CMakeLists.txt";
  auto cmakepresets = get_fetch_src_dir(bc) / "CMakePresets.json";

  namespace fs = std::filesystem;

  {
    std::ofstream ffs{cmakelists};
    ffs << cc.data;
  }

  {
    std::ofstream ofs{cmakepresets};
    nspreset::write(ofs, nspreset::write_compiler_paths, cmake::path(get_full_bld_dir(bc)), {}, bc);
  }
}

nsmodule::content nsmodule::make_fetch_build_content(nsbuild const& bc) const
{
  std::stringstream ofs;

  ofs << fmt::format(cmake::k_project_name, name, version.empty() ? bc.version : version);
  ofs << fmt::format("\nlist(PREPEND CMAKE_MODULE_PATH \"{}\")", cmake::path(get_full_sdk_dir(bc)));
  bc.macros.print(ofs);
  macros.print(ofs);
  // write_variables(ofs, bc);
  for (auto const& a : fetch->args)
  {
    a.print(ofs, output_fmt::set_cache, false);
  }

  std::filesystem::path src = source_path;

  bc.write_cxx_options(ofs);

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

  auto cmakelists = get_fetch_src_dir(bc) / "CMakeLists.txt";
  if (std::filesystem::exists(cmakelists))
  {
    std::ifstream t{cmakelists};
    std::string   buffer((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    ofs << buffer;
  }

  // Do we have build
  auto srccmk = src / "Finalize.cmake";
  if (!fetch->finalize.fragments.empty() || std::filesystem::exists(srccmk))
  {
    if (!fetch->finalize.fragments.empty())
    {
      for (auto const& f : fetch->finalize.fragments)
        ofs << f << "\n";
    }

    if (std::filesystem::exists(srccmk))
    {
      std::ifstream t{srccmk};
      std::string   buffer((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
      ofs << buffer;
    }
  }

  content cc;
  cc.data = ofs.str();

  picosha2::hash256_hex_string(cc.data, cc.sha);
  return cc;
}

void nsmodule::fetch_content(nsbuild const& bc)
{
  std::string sha;
  bool        change = false;

  // download
  regenerate |= download(bc);

  if (regenerate)
  {
    auto cc = make_fetch_build_content(bc);
    write_fetch_build_content(bc, cc);
    sha    = cc.sha;
    change = fetch_changed(bc, sha);
  }

  if (bc.cmakeinfo.cmake_skip_fetch_builds)
  {
    write_fetch_meta(bc, sha);
    return;
  }

  if (change || fetch->force_build || force_build)
  {
    nslog::print(fmt::format("Rebuilding : {}..", name));
    build_fetched_content(bc);
    write_fetch_meta(bc, sha);
    was_fetch_rebuilt = true;
  }
  else
  {
    nslog::print(fmt::format("Already Built : {}..", name));
  }
}

bool nsmodule::fetch_changed(nsbuild const& bc, std::string const& last_sha) const
{
  auto          meta = get_full_fetch_file(bc);
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
  auto          meta = get_full_fetch_file(bc);
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

  ofs << fmt::format("\nproject({} VERSION {} LANGUAGES C CXX)\n", name, version.empty() ? bc.version : version);

  cmake::line(ofs, "variables");
  macros.print(ofs);
  write_variables(ofs, bc);
  cmake::line(ofs, "globs");
  if (has_data(type))
  {
    glob_media.print(ofs, "data_group", "${CMAKE_CURRENT_LIST_DIR}/", bc.get_full_cmake_gen_dir());
    glob_media.print(ofs, "data_group_output", fmt::format("${{config_rt_dir}}/{}/", bc.media_name), {});
  }
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
  for (size_t c = 0; c < contents.size(); ++c)
  {
    std::ofstream of{get_full_gen_dir(bc) / fmt::format("c{}.txt", c)};
    of << contents[c].content;
  }
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

void nsmodule::gather_sources(nsglob& glob, nsbuild const& bc) const
{
  auto sp = std::filesystem::path(source_path);
  auto gp = std::filesystem::path(gen_path);
  glob.add_set(sp / "src");
  for (auto const sub : source_sub_dirs)
    glob.add_set(sp / "src" / sub);
  glob.add_set(gp);
  glob.add_set(gp / "local");
  for (auto const& r : references)
  {
    auto const& m = bc.get_module(r);
    m.gather_sources(glob, bc);
  }
}

void nsmodule::write_sources(std::ostream& ofs, nsbuild const& bc) const
{
  if (has_runtime(type))
    glob_sources.print(ofs, "__module_sources", "${CMAKE_CURRENT_LIST_DIR}/", bc.get_full_cmake_gen_dir());
}

void nsmodule::write_target(std::ostream& ofs, nsbuild const& bc, std::string const& name) const
{
  if (disabled)
    return;
  cmake::line(ofs, "target");
  switch (type)
  {
  case nsmodule_type::data:
    ofs << fmt::format("\nadd_custom_target({} ALL SOURCES ${{data_group}})", name);
    break;
  case nsmodule_type::exe:
    ofs << fmt::format("\nadd_executable({} {} ${{__module_sources}} ${{__natvis_file}})", name,
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
    ofs << fmt::format("\nadd_executable({} {} ${{__module_sources}} ${{__natvis_file}})", name,
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
  // cmake::line(ofs, "cxx-options");
  // ofs << "\ntarget_compile_options(${module_target} PRIVATE "
  //        "${__module_cxx_compile_flags})";
  // ofs << "\ntarget_link_options(${module_target} PRIVATE "
  //        "${__module_cxx_linker_flags})";
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
    write_include(ofs, "${CMAKE_CURRENT_LIST_DIR}", "", cmake::inheritance::pub);
    write_include(ofs, "${module_dir}", "public", cmake::inheritance::pub);
    write_include(ofs, "${module_dir}", "private", cmake::inheritance::priv);
    write_include(ofs, "${module_dir}", "src", cmake::inheritance::priv);
    write_include(ofs, "${module_gen_dir}", "", cmake::inheritance::pub);
    write_include(ofs, "${module_gen_dir}", "local", cmake::inheritance::priv);
    for (auto const& s : source_sub_dirs)
      write_include(ofs, "${module_dir}", "src/" + s, cmake::inheritance::priv);
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
    target.write_include(ofs, m.source_path, "public",
                         target.type == nsmodule_type::plugin ? cmake::inheritance::priv : cmake::inheritance::pub);
    target.write_include(ofs, m.source_path, "private", cmake::inheritance::priv);
    target.write_include(ofs, m.source_path, "src", cmake::inheritance::priv);
    for (auto const& s : m.source_sub_dirs)
      target.write_include(ofs, m.source_path, "src/" + s, cmake::inheritance::priv);
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
      if (fetch->skip_namespace)
      {
        for (auto const& c : fetch->targets)
          ofs << fmt::format("\n   {}", c);
      }
      else
      {
        for (auto const& c : fetch->targets)
          ofs << fmt::format("\n   {}::{}", namespace_name, c);
      }
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
    cmake::line(ofs, "definitions");
    write_definitions_itf(ofs, bc);
    break;
  default:
    break;
  }
}

void nsmodule::write_definitions_itf(std::ostream& ofs, nsbuild const& bc) const
{
  constexpr uint32_t type = pub_intf;
  for (std::size_t i = 0; i < intf[type].size(); ++i)
  {
    auto        filter = intf[type][i].filters;
    auto const& dep    = intf[type][i].definitions;
    for (auto const& d : dep)
    {
      auto def = fmt::format("{}={}", d.first, d.second);
      write_definitions(ofs, def, cmake::inheritance::intf, filter);
    }
  }
}

void nsmodule::write_definitions_mod(std::ostream& ofs, nsbuild const& bc) const
{
  write_definitions(ofs, bc, 0);
  write_definitions(ofs, bc, 1);
  write_refs_definitions(ofs, bc, *this);
  write_definitions(ofs, fmt::format("BC_MODULE_{}", cmake::value(name)), cmake::inheritance::priv, "");
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
      auto const& mod = bc.get_module(d);
      write_dependency(ofs, mod.target_name, type == pub_intf ? cmake::inheritance::pub : cmake::inheritance::priv,
                       filter);
      if (mod.type == nsmodule_type::lib || mod.type == nsmodule_type::external || mod.type == nsmodule_type::ref)
        write_target_link_libs(ofs, mod.target_name,
                               type == pub_intf ? cmake::inheritance::pub : cmake::inheritance::priv, filter);
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
    ofs << fmt::format("\nlist(APPEND __module_ref_deps {})", m.target_name);
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
    ofs << "\nset_target_properties(${module_target} PROPERTIES " << cmake::k_rt_locations << ")";
    break;
  case nsmodule_type::plugin:
    cmake::line(ofs, "runtime-settings");
    ofs << "\nset_target_properties(${module_target} PROPERTIES "
        << fmt::format(cmake::k_plugin_locations, bc.plugin_dir) << ")";
    break;
  default:
    return;
  }
}

void nsmodule::build_fetched_content(nsbuild const& bc)
{
  auto src  = get_fetch_src_dir(bc);
  auto xpb  = get_full_bld_dir(bc);
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
      auto path    = get_full_sdk_dir(bc) / l;

      if (!fs::exists(path))
      {
        nslog::error(fmt::format("Path does not exist: {}", path.string()));
        continue;
      }

      auto       it           = fs::directory_iterator{path};
      const auto copy_options = fs::copy_options::update_existing | fs::copy_options::recursive;
      for (auto const& dir_entry : it)
      {
        if (!dir_entry.is_regular_file() && !dir_entry.is_symlink())
          continue;
        auto name = dir_entry.path().filename().generic_string();
        if (bc.verbose)
          nslog::print(fmt::format("File to copy : {}", name));
        bool copy = false;

        if (std::regex_search(name, bc.dll_ext))
        {
          if (bc.verbose)
            nslog::print(fmt::format("Copying : {}", name));
          std::filesystem::copy(dir_entry.path(), bc.get_full_rt_dir() / "bin" / dir_entry.path().filename(),
                                copy_options);
          continue;
        }
        else if (bc.verbose)
          nslog::print(fmt::format("Skipping copy of : {}", name));

        if (fetch->runtime_files.empty())
          continue;

        name = dir_entry.path().generic_string();
        for (auto const& rt : fetch->runtime_files)
        {
          std::smatch match;
          std::regex  rtregex = std::regex(rt, std::regex_constants::icase);
          if (!std::regex_search(name, match, rtregex))
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

std::filesystem::path nsmodule::get_full_bld_dir(nsbuild const& bc) const
{
  return bc.get_full_build_dir() / framework_name / name;
}

std::filesystem::path nsmodule::get_full_sdk_dir(nsbuild const& bc) const { return bc.get_full_sdk_dir(); }

std::filesystem::path nsmodule::get_full_dl_dir(nsbuild const& bc) const { return bc.get_full_dl_dir() / name; }
std::filesystem::path nsmodule::get_fetch_src_dir(nsbuild const& bc) const
{
  return fetch ? (fetch->source.empty() ? bc.get_full_dl_dir() / name : bc.get_full_dl_dir() / name / fetch->source)
               : std::filesystem::path();
}

std::filesystem::path nsmodule::get_full_fetch_file(nsbuild const& bc) const
{
  return bc.get_full_cache_dir() / fmt::format("{}.fetch", name);
}

std::filesystem::path nsmodule::get_full_gen_dir(nsbuild const& bc) const
{
  return bc.get_full_cfg_dir() / k_gen_dir / framework_name / name;
}

bool nsmodule::download(nsbuild const& bc)
{
  auto dld = get_full_dl_dir(bc);
  if (std::filesystem::exists(get_fetch_src_dir(bc) / "CMakeLists.txt") && !regenerate && !fetch->force_download)
    return false;
  if (fetch->repo.ends_with(".git"))
    nsprocess::git_clone(bc, get_full_dl_dir(bc), fetch->repo, fetch->tag);
  else
    nsprocess::download(bc, get_full_dl_dir(bc), fetch->repo, fetch->source);
  if (!deleted)
    delete_build(bc);
  return true;
}

bool nsmodule::sha_changed(nsbuild const& bc, std::string_view name, std::string_view isha) const
{
  auto          meta = bc.get_full_cache_dir() / fmt::format("{}.{}.{}.glob", framework_name, this->name, name);
  std::ifstream iff{meta};
  if (iff.is_open())
  {
    std::string sha;
    iff >> sha;
    if (sha == isha)
      return false;
  }
  return true;
}

void nsmodule::write_sha_changed(nsbuild const& bc, std::string_view name, std::string_view isha) const
{
  auto          meta = bc.get_full_cache_dir() / fmt::format("{}.{}.{}.glob", framework_name, this->name, name);
  std::ofstream off{meta};
  if (off.is_open())
    off << isha;
}
