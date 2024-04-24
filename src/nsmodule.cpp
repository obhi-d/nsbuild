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

nsfetch* nsmodule::find_fetch(std::string_view name)
{
  auto it = std::ranges::find(fetch, fmt::format("{}_{}", this->name, name), &nsfetch::name);

  return it != fetch.end() ? &(*it) : nullptr;
}

void nsmodule::process(nsbuild const& bc, nsinstallers& installer, std::string const& targ_name, nstarget& targ)
{
  update_properties(bc, targ_name, targ);
  if (disabled)
  {
    write_sha(bc);
    return;
  }

  update_macros(bc, targ_name, targ);
  update_fetch(bc, installer);
  write_sha(bc);
}

void nsmodule::check_embeds(nsbuild const& bc) const
{
  auto hpp = get_full_gen_dir(bc) / "local" / fmt::format("{}Resources.hpp", name);
  auto cpp = get_full_gen_dir(bc) / "local" / fmt::format("{}Resources.cpp", name);

  bool        rewrite_resources = false;
  std::string cpp_content;
  std::string csha;
  picosha2::hash256_hex_string(embedded_binary_files.cpp, csha);
  if (!std::filesystem::exists(hpp) || !std::filesystem::exists(cpp) || sha_changed(bc, "embed", csha))
  {

    std::ofstream(hpp, std::ios::binary) << "#pragma once\n#include <string_view>\nnamespace " << bc.namespace_name
                                         << "::embed \n{\n"
                                         << embedded_binary_files.hpp << "\n}";

    std::ofstream(cpp, std::ios::binary) << "#include \"" << name << "Resources.hpp\"\nnamespace " << bc.namespace_name
                                         << "::embed \n{\n"
                                         << embedded_binary_files.cpp << "\n}";
    write_sha_changed(bc, "embed", csha);
  }
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
    if ((has_globs_changed |= sha_changed(bc, "data_group", glob_media.sha)))
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
    cmd.msgs.push_back(fmt::format("Copying contents for {}", name));
    cmd.command = "${CMAKE_COMMAND}";
    cmd.params  = fmt::format("-E copy_if_different \"${{module_gen_dir}}/c{0}.txt\" \"{1}\" ", c, contents[c].name);
    step.steps.push_back(cmd);
    step.wd = bc.wd.generic_string();
    postbuild.push_back(step);
  }

  if (has_headers(type))
  {
    check_enums(bc);
    check_embeds(bc);
  }

  if (has_runtime(type))
  {
    static std::unordered_set<std::string> filters = {".cpp", ".hpp"};

    glob_sources.file_filters = &filters;
    gather_sources(glob_sources, bc);
    gather_headers(glob_sources, bc);

    if (!bc.s_current_preset->glob_sources)
    {
      glob_sources.accumulate();
      if ((has_globs_changed |= sha_changed(bc, "src", glob_media.sha)))
        write_sha_changed(bc, "src", glob_media.sha);
    }
  }

  if (intf[nsmodule::priv_intf].empty())
    intf[nsmodule::priv_intf].emplace_back();

  if (has_runtime(type))
    intf[nsmodule::priv_intf].back().definitions.emplace_back("BC_CONFIG_HEADER",
                                                              fmt::format("\"{}ModuleConfig.hpp\"", name));

  if (type == nsmodule_type::plugin && !bc.s_current_preset->static_plugins)
  {
    intf[nsmodule::priv_intf].back().definitions.emplace_back("BC_SHARED_LIBS", "1");
  }

  if (!bc.s_current_preset->static_libs && type != nsmodule_type::plugin)
  {
    intf[nsmodule::priv_intf].back().definitions.emplace_back("BC_SHARED_LIBS", "1");
  }

  if (bc.s_current_preset->build_type == "debug")
  {
    intf[nsmodule::priv_intf].back().definitions.emplace_back("BC_IS_DEBUG_BUILD", "1");
    intf[nsmodule::priv_intf].back().definitions.emplace_back("BC_DEBUG_BUILD", "1");
  }
  else
  {
    intf[nsmodule::priv_intf].back().definitions.emplace_back("BC_IS_DEBUG_BUILD", "0");
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
  {
    std::string module_reg_extern;
    std::string module_reg_calls;
    if (bc.s_current_preset->static_plugins)
    {
      for (auto const& mod : required_plugins)
      {
        if (!mod.empty())
        {
          std::string fn_name;
          auto const& d = bc.get_module(mod);
          d.macros.im_print(fn_name, bc.plugin_entry);
          fmt::format_to(std::back_inserter(module_reg_extern), R"(\n// import {})", mod);
          fmt::format_to(std::back_inserter(module_reg_extern), R"(\nextern \"C\" BC_LIB_IMPORT void {}();)", fn_name);
          fmt::format_to(std::back_inserter(module_reg_calls), R"(\n    {}();)", fn_name);
        }
      }
    }

    macros["module_extras_cpp"] = fmt::format(R"({}\nnamespace BuildConfig \n{{\n  void {}()\n  {{ {} \n  }}\n}}\n)",
                                              module_reg_extern, bc.plugin_registration, module_reg_calls);
    macros["module_extras_hpp"] =
        fmt::format(R"(constexpr std::string_view AppName = \"{}\";\n  constexpr std::string_view OrgName = \"{}\";\n)",
                    name, org_name);
  }
  else
  {
    macros["module_extras_hpp"] = "";
    macros["module_extras_cpp"] = "";
  }

  if (!fetch.empty())
  {
    for (auto& ft : fetch)
    {
      std::string components;
      bool        first = true;
      for (auto c : ft.components)
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
      macros[fmt::format("{}_bulid_dir", ft.name)]   = cmake::path(get_fetch_bld_dir(bc, ft));
      macros[fmt::format("{}_sdk_dir", ft.name)]     = cmake::path(get_full_sdk_dir(bc));
      macros[fmt::format("{}_src_dir", ft.name)]     = cmake::path(get_fetch_src_dir(bc, ft));
      macros[fmt::format("{}_package", ft.name)]     = ft.package;
      macros[fmt::format("{}_namespace", ft.name)]   = ft.namespace_name;
      macros[fmt::format("{}_repo", ft.name)]        = ft.repo;
      macros[fmt::format("{}_commit", ft.name)]      = ft.tag;
      macros[fmt::format("{}_components", ft.name)]  = components;
      macros[fmt::format("{}_extern_name", ft.name)] = ft.extern_name;

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
}

void nsmodule::delete_build(nsbuild const& bc)
{
  if (deleted)
    return;
  std::error_code ec;

  for (auto const& ft : fetch)
  {
    auto fetch_bld = get_fetch_bld_dir(bc, ft);
    nslog::print(fmt::format("Deleting Fetch : {}", ft.name));
    std::filesystem::remove(get_full_fetch_file(bc, ft), ec);
    nsbuild::remove_cache(get_full_dl_dir(bc, ft));
    nsbuild::remove_cache(fetch_bld);
    bc.install_cache.uninstall((fetch_bld / "install_manifest.txt").string());
  }
  regenerate  = true;
  force_build = true;
  deleted     = true;
}

void nsmodule::update_fetch(nsbuild const& bc, nsinstallers& installer)
{
  if (fetch.empty())
    return;
  for (auto& ft : fetch)
  {
    if (ft.repo.empty() || ft.disabled)
      continue;
    auto dl = get_fetch_src_dir(bc, ft);
    if (!std::filesystem::exists(dl))
      ft.regenerate = true;
    else
      ft.regenerate = regenerate;
  }

  for (auto& ft : fetch)
  {
    if (!ft.disabled)
      fetch_content(bc, installer, ft);
  }
}

void nsmodule::write_fetch_build_content(nsbuild const& bc, nsfetch const& ft, content const& cc) const
{
  backup_fetch_lists(bc, ft);

  // backup
  auto cmakelists   = get_fetch_src_dir(bc, ft) / "CMakeLists.txt";
  auto cmakepresets = get_fetch_src_dir(bc, ft) / "CMakePresets.json";

  namespace fs = std::filesystem;

  {
    std::ofstream ffs{cmakelists};
    ffs << cc.data;
  }

  {
    std::ofstream ofs{cmakepresets};
    nspreset::write(ofs, nspreset::write_compiler_paths, cmake::path(get_fetch_bld_dir(bc, ft)), {}, bc);
  }
}

nsmodule::content nsmodule::make_fetch_build_content(nsbuild const& bc, nsfetch const& ft) const
{
  std::stringstream ofs;

  ofs << fmt::format(cmake::k_project_name, name, version.empty() ? bc.version : version);
  ofs << fmt::format("\nlist(PREPEND CMAKE_MODULE_PATH \"{}\")", cmake::path(get_full_sdk_dir(bc)));
  bc.macros.print(ofs);
  macros.print(ofs);
  // write_variables(ofs, bc);
  for (auto const& a : ft.args)
  {
    a.print(ofs, output_fmt::set_cache, false);
  }

  std::filesystem::path src = source_path;

  bc.write_cxx_options(ofs);

  // Do we have prepare
  if (!ft.prepare.fragments.empty())
  {
    for (auto const& f : ft.prepare.fragments)
      ofs << f << "\n";
  }

  auto cmakelists = get_fetch_src_dir(bc, ft) / "CMakeLists.txt";
  if (std::filesystem::exists(cmakelists))
  {
    std::ifstream t{cmakelists};
    std::string   buffer((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    ofs << buffer;
  }

  // Do we have build
  if (!ft.finalize.fragments.empty())
  {
    for (auto const& f : ft.finalize.fragments)
      ofs << f << "\n";
  }

  content cc;
  cc.data = ofs.str();

  picosha2::hash256_hex_string(cc.data, cc.sha);
  return cc;
}

void nsmodule::fetch_content(nsbuild const& bc, nsinstallers& installer, nsfetch& ft)
{
  std::string sha;
  bool        change = false;

  // download
  regenerate |= download(bc, ft);

  if (ft.regenerate)
  {
    auto cc = make_fetch_build_content(bc, ft);
    write_fetch_build_content(bc, ft, cc);
    sha    = cc.sha;
    change = fetch_changed(bc, ft, sha);
  }

  if (bc.cmakeinfo.cmake_skip_fetch_builds)
  {
    write_fetch_meta(bc, ft, sha);
    return;
  }

  if (change || ft.force_build || force_build)
  {
    nslog::print(fmt::format("Rebuilding : {}..", ft.name));
    build_fetched_content(bc, installer, ft);
    write_fetch_meta(bc, ft, sha);
    was_fetch_rebuilt = true;
  }
  else
  {
    auto xpb = get_fetch_bld_dir(bc, ft);
    nslog::print(fmt::format("Already Built : {}..", ft.name));
    installer.installed((xpb / "install_manifest.txt").string());
  }
}

bool nsmodule::fetch_changed(nsbuild const& bc, nsfetch const& ft, std::string const& last_sha) const
{
  auto          meta = get_full_fetch_file(bc, ft);
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

void nsmodule::write_fetch_meta(nsbuild const& bc, nsfetch const& ft, std::string const& last_sha) const
{
  if (last_sha.empty())
    return;
  auto          meta = get_full_fetch_file(bc, ft);
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
    glob_media.print(ofs, "data_group_output", fmt::format("${{config_rt_dir}}/{}/", bc.media_name),
                     glob_media.sub_paths.back());
  }
  write_sources(ofs, bc);
  write_target(ofs, bc, target_name);
  write_includes(ofs, bc);
  write_definitions(ofs, bc);
  write_find_package(ofs, bc);
  write_dependencies(ofs, bc);
  write_linklibs(ofs, bc);
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

void nsmodule::gather_headers(nsglob& glob, nsbuild const& bc) const
{
  auto sp = std::filesystem::path(source_path);
  auto gp = std::filesystem::path(gen_path);
  glob.add_set(sp / "public");
  glob.add_set(sp / "private");
  for (auto const sub : source_sub_dirs)
  {
    glob.add_set(sp / "public" / sub);
    glob.add_set(sp / "private" / sub);
  }
  glob.add_set(gp);
  glob.add_set(gp / "local");
  for (auto const& r : references)
  {
    auto const& m = bc.get_module(r);
    m.gather_headers(glob, bc);
  }
}
void nsmodule::write_sources(std::ostream& ofs, nsbuild const& bc) const
{
  if (has_runtime(type))
  {
    if (bc.s_current_preset->glob_sources)
      glob_sources.print(ofs, "__module_sources");
    else
      glob_sources.print(ofs, "__module_sources", "${CMAKE_CURRENT_LIST_DIR}/", bc.get_full_cmake_gen_dir());
  }
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
    if (console_app)
      ofs << "\ntarget_compile_definitions(${{module_target}} -DBC_CONSOLE_APP)";
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
    if (bc.s_current_preset->static_plugins)
      ofs << fmt::format("\nadd_library({} STATIC ${{__module_sources}})", name);
    else
      ofs << fmt::format("\nadd_library({} MODULE ${{__module_sources}})", name);
    break;
  case nsmodule_type::test:
    ofs << fmt::format("\nadd_executable({} {} ${{__module_sources}} ${{__natvis_file}})", name,
                       "${__nsbuild_console_app_options}");
    if (console_app)
      ofs << "\ntarget_compile_definitions(${{module_target}} -DBC_CONSOLE_APP)";
    break;
  default:
    break;
  }

  if (has_runtime(type))
  {
    ofs << "\nunset(__module_sources)";
  }
}

void nsmodule::write_prebuild_steps(std::ostream& ofs, const nsbuild& bc) const
{
  int total = begin_prebuild_steps(ofs, bc.s_current_preset->prebuild, bc);
  total += begin_prebuild_steps(ofs, prebuild, bc);
  if (total)
  {
    ofs << cmake::k_begin_prebuild_steps;
    end_prebuild_steps(ofs, bc.s_current_preset->prebuild, bc);
    end_prebuild_steps(ofs, prebuild, bc);
    ofs << cmake::k_finalize_prebuild_steps;
  }
}

void nsmodule::write_postbuild_steps(std::ostream& ofs, const nsbuild& bc) const
{
  write_postbuild_steps(ofs, bc.s_current_preset->postbuild, bc);
  write_postbuild_steps(ofs, postbuild, bc);
}

int nsmodule::begin_prebuild_steps(std::ostream& ofs, nsbuildsteplist const& list, const nsbuild& bc) const
{
  int steps = 0;
  if (!list.empty())
  {
    cmake::line(ofs, "prebuild-steps");
    for (auto const& step : list)
    {
      if (step.module_filter && !(step.module_filter & (1u << (uint32_t)type)))
        continue;

      steps++;
      step.print(ofs, bc, *this);
    }
  }
  return steps;
}

int nsmodule::write_postbuild_steps(std::ostream& ofs, nsbuildsteplist const& list, nsbuild const& bc) const
{
  int steps = 0;
  for (auto const& step : list)
  {
    if (step.module_filter && !(step.module_filter & (1u << (uint32_t)type)))
      continue;
    steps++;
  }

  if (steps)
  {
    cmake::line(ofs, "postbuild-steps");
    ofs << fmt::format("\nadd_custom_command(TARGET ${{module_target}} POST_BUILD ");
    for (auto const& step : list)
    {
      if (step.module_filter && !(step.module_filter & (1u << (uint32_t)type)))
        continue;
      for (auto const& s : step.steps)
      {
        ofs << "\n  COMMAND " << s.command << " " << s.params;
        for (auto const& m : s.msgs)
          ofs << "\n  COMMAND ${CMAKE_COMMAND} -E echo \"" << m << "\"";
      }
    }
    ofs << "\n\t)";
  }
  return steps;
}

int nsmodule::end_prebuild_steps(std::ostream& ofs, nsbuildsteplist const& list, const nsbuild& bc) const
{
  int steps = 0;
  if (!list.empty())
  {
    for (auto const& step : list)
    {
      if (step.module_filter && !(step.module_filter & (1u << (uint32_t)type)))
        continue;
      for (auto const& d : step.artifacts)
        ofs << fmt::format("\nlist(APPEND module_prebuild_artifacts {})", d);
      steps++;
    }
  }
  return steps;
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
    {
      ofs << "\nset(__module_headers)";

      // nsglob header_glob;
      //
      // static std::unordered_set<std::string> hpp_filters = {".hpp"};
      // header_glob.file_filters                           = &hpp_filters;
      // header_glob.recurse                                = false;

      write_include(ofs, /*header_glob, */ "${CMAKE_CURRENT_LIST_DIR}", "", cmake::inheritance::pub);
      write_include(ofs, /*header_glob, */ "${module_dir}", "public", cmake::inheritance::pub);
      write_include(ofs, /*header_glob, */ "${module_dir}", "private", cmake::inheritance::priv);
      write_include(ofs, /*header_glob, */ "${module_dir}", "src", cmake::inheritance::priv);
      write_include(ofs, /*header_glob, */ "${module_gen_dir}", "", cmake::inheritance::pub);
      write_include(ofs, /*header_glob, */ "${module_gen_dir}", "local", cmake::inheritance::priv);
      for (auto const& s : source_sub_dirs)
        write_include(ofs, /*header_glob,*/ "${module_dir}", "src/" + s, cmake::inheritance::priv);
      write_refs_includes(ofs, /*header_glob,*/ bc, *this);

      // if (bc.glob_sources)
      // {
      //   header_glob.print(ofs, "__module_headers");
      //   ofs << "\ntarget_sources(${module_target} PRIVATE ${__module_headers})";
      //   ofs << "\nunset(__module_headers)";
      // }
    }
  default:
    break;
  }
}

void nsmodule::write_include(std::ostream& ofs, /* nsglob& glob,*/ std::string_view path, std::string_view subpath,
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
  // glob.sub_paths.push_back(fmt::format("{}/{}", path, subpath));
}

void nsmodule::write_refs_includes(std::ostream& ofs, /*  nsglob& glob,*/ nsbuild const& bc,
                                   nsmodule const& target) const
{
  for (auto const& r : references)
  {
    auto const& m = bc.get_module(r);
    m.write_refs_includes(ofs, /* glob, */ bc, target);
    target.write_include(ofs, /* glob, */ m.source_path, "public",
                         target.type == nsmodule_type::plugin ? cmake::inheritance::priv : cmake::inheritance::pub);
    target.write_include(ofs, /* glob, */ m.source_path, "private", cmake::inheritance::priv);
    target.write_include(ofs, /* glob, */ m.source_path, "src", cmake::inheritance::priv);
    for (auto const& s : m.source_sub_dirs)
      target.write_include(ofs, /* glob, */ m.source_path, "src/" + s, cmake::inheritance::priv);
    target.write_include(ofs, /* glob, */ m.gen_path, "",
                         target.type == nsmodule_type::plugin ? cmake::inheritance::priv : cmake::inheritance::pub);
    target.write_include(ofs, /* glob, */ m.gen_path, "local", cmake::inheritance::priv);
  }
}

void nsmodule::write_find_package(std::ostream& ofs, nsbuild const& bc) const
{

  if (fetch.empty())
    return;
  for (auto const& ft : fetch)
  {
    cmake::line(ofs, "find-package");
    if (ft.components.empty())
      ofs << fmt::format(cmake::k_find_package, ft.package, "", ft.name);
    else
    {

      ofs << fmt::format(cmake::k_find_package_comp_start, ft.package, ft.version);
      for (auto const& c : ft.components)
        ofs << c << " ";
      ofs << cmake::k_find_package_comp_end;
    }

    auto namespace_name = ft.namespace_name.empty() ? ft.package : ft.namespace_name;
    if (!ft.legacy_linking)
    {
      ofs << "\ntarget_link_libraries(${module_target} \n  " << cmake::to_string(cmake::inheritance::intf);
      if (ft.targets.empty())
      {
        if (ft.skip_namespace)
          ofs << fmt::format("\n   {}", ft.package);
        else
          ofs << fmt::format("\n   {}::{}", namespace_name, ft.package);
      }
      else
      {
        if (ft.skip_namespace)
        {
          for (auto const& c : ft.targets)
            ofs << fmt::format("\n   {}", c);
        }
        else
        {
          for (auto const& c : ft.targets)
            ofs << fmt::format("\n   {}::{}", namespace_name, c);
        }
      }
      ofs << "\n)";
    }
    else
    {
      ofs << fmt::format("\nset(__sdk_install_includes ${{{}_INCLUDE_DIRS}})", ft.package);
      ofs << fmt::format("\nlist(TRANSFORM __sdk_install_includes REPLACE ${{{}_sdk_dir}} \"\")", ft.name);
      ofs << fmt::format("\nset(__sdk_install_libraries ${{{}_LIBRARIES}})", ft.package);
      ofs << fmt::format("\nlist(TRANSFORM __sdk_install_libraries REPLACE ${{{}_sdk_dir}} \"\")", ft.name);

      ofs << "\ntarget_include_directories(${module_target} " << cmake::to_string(cmake::inheritance::intf)
          << fmt::format("\n\t$<BUILD_INTERFACE:\"${{{}_INCLUDE_DIR}}\">", ft.package)
          << "\n\t$<INSTALL_INTERFACE:\"${__sdk_install_includes}\">"
          << "\n)"
          << "\ntarget_link_libraries(${module_target} " << cmake::to_string(cmake::inheritance::intf)
          << fmt::format("\n\t$<BUILD_INTERFACE:\"${{{}_LIBRARIES}}\">", ft.package)
          << "\n\t$<INSTALL_INTERFACE:\"${__sdk_install_libraries}\">"
          << "\n)";
    }

    if (!ft.include.fragments.empty())
    {
      cmake::line(ofs, "include-package-cmake");
      for (auto const& f : ft.include.fragments)
        ofs << f << "\n";
    }
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
  if (bc.s_current_preset->static_plugins)
    write_plugin_dependencies(ofs, bc);
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

void nsmodule::write_plugin_dependencies(std::ostream& ofs, nsbuild const& bc) const
{
  for (auto const& r : required_plugins)
  {
    auto const& mod = bc.get_module(r);
    write_dependency(ofs, mod.target_name, cmake::inheritance::priv, "");
    write_target_link_libs(ofs, mod.target_name, cmake::inheritance::priv, "");
  }
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
    ofs << fmt::format("--test={} ", t.test_param_name);
    bool first = true;
    for (auto const& p : t.parameters)
    {
      if (first)
      {
        first = false;
        continue;
      }
      ofs << fmt::format("--{}={} ", p.first, p.second);
    }
    ofs << "\n  WORKING_DIRECTORY ${config_rt_dir}/bin)";
    if (!t.tags.empty())
    {
      ofs << fmt::format("\nset_tests_properties({} PROPERTIES LABELS \"{}\")", t.name, t.tags);
    }
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
    if (!bc.s_current_preset->static_plugins)
      ofs << "\nset_target_properties(${module_target} PROPERTIES "
          << fmt::format(cmake::k_plugin_locations, bc.plugin_dir) << ")";
    break;
  default:
    return;
  }
}

void nsmodule::build_fetched_content(nsbuild const& bc, nsinstallers& installer, nsfetch const& ft)
{
  auto src  = get_fetch_src_dir(bc, ft);
  auto xpb  = get_fetch_bld_dir(bc, ft);
  auto dsdk = get_full_sdk_dir(bc);

  nsprocess::cmake_config(bc, {}, cmake::path(src), xpb);

  nsprocess::cmake_build(bc, "", xpb);
  nsprocess::cmake_install(bc, cmake::path(dsdk), xpb);
  installer.installed((xpb / "install_manifest.txt").string());
  // custom location copy
  if (!ft.runtime_loc.empty())
  {
    for (auto const& l : ft.runtime_loc)
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
          auto path = bc.get_full_rt_dir() / "bin";
          std::filesystem::create_directories(path);
          std::filesystem::copy(dir_entry.path(), path / dir_entry.path().filename(), copy_options);
          continue;
        }
        else if (bc.verbose)
          nslog::print(fmt::format("Skipping copy of : {}", name));

        if (ft.runtime_files.empty())
          continue;

        name = dir_entry.path().generic_string();
        for (auto const& rt : ft.runtime_files)
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

std::filesystem::path nsmodule::get_fetch_bld_dir(nsbuild const& bc, nsfetch const& nfc) const
{
  auto fbld_name = get_full_bld_dir(bc);
  return nfc.name == name || fetch.size() > 1 ? fbld_name : fbld_name / nfc.name;
}

std::filesystem::path nsmodule::get_full_sdk_dir(nsbuild const& bc) const { return bc.get_full_sdk_dir(); }

std::filesystem::path nsmodule::get_full_dl_dir(nsbuild const& bc, nsfetch const& nfc) const
{
  return bc.get_full_dl_dir() / nfc.name;
}
std::filesystem::path nsmodule::get_fetch_src_dir(nsbuild const& bc, nsfetch const& nfc) const
{
  auto path = bc.get_full_dl_dir() / nfc.name;
  return (path / nfc.source);
}

std::filesystem::path nsmodule::get_full_fetch_file(nsbuild const& bc, nsfetch const& nfc) const
{
  return bc.get_full_cache_dir() / fmt::format("{}.fetch", nfc.name);
}

std::filesystem::path nsmodule::get_full_gen_dir(nsbuild const& bc) const
{
  return bc.get_full_cfg_dir() / k_gen_dir / framework_name / name;
}

void nsmodule::backup_fetch_lists(nsbuild const& bc, nsfetch const& ft) const
{
  std::error_code ec;
  auto            cmakelists = get_fetch_src_dir(bc, ft) / "CMakeLists.txt";
  namespace fs               = std::filesystem;
  if (fs::exists(cmakelists))
    fs::copy(cmakelists, get_full_gen_dir(bc) / fmt::format("{}_CMakeLists.txt", ft.name),
             fs::copy_options::overwrite_existing, ec);
  else
  {
    std::ofstream(fmt::format("{}_CMakeLists.txt", ft.name)).close();
  }
}

bool nsmodule::restore_fetch_lists(nsbuild const& bc, nsfetch const& ft) const
{
  std::error_code ec;
  auto            cmakelists = get_full_gen_dir(bc) / fmt::format("{}_CMakeLists.txt", ft.name);
  namespace fs               = std::filesystem;
  if (fs::exists(cmakelists))
    fs::copy(cmakelists, get_fetch_src_dir(bc, ft) / "CMakeLists.txt", fs::copy_options::overwrite_existing, ec);
  else
    return false;
  return true;
}

bool nsmodule::download(nsbuild const& bc, nsfetch& ft)
{
  auto dld = get_full_dl_dir(bc, ft);
  if ((std::filesystem::exists(get_fetch_src_dir(bc, ft) / "CMakeLists.txt")) && !ft.regenerate && !ft.force_download)
    return false;
  if (ft.repo.ends_with(".git"))
    nsprocess::git_clone(bc, get_full_dl_dir(bc, ft), ft.repo, ft.tag);
  else
  {
    if (!nsprocess::download(bc, get_full_dl_dir(bc, ft), ft.repo, ft.source, ft.version, ft.force_download) &&
        !restore_fetch_lists(bc, ft))
      nsprocess::download(bc, get_full_dl_dir(bc, ft), ft.repo, ft.source, ft.version, true);
  }

  if (!deleted)
    delete_build(bc);
  return true;
}

void nsmodule::write_sha(nsbuild const& bc)
{
  if (!sha_written)
  {
    bc.write_sha(name, sha);
    sha_written = true;
  }
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

void ns_embed_content::emplace_back(std::string_view name, std::string_view value, std::string file)
{

  auto name_ = to_upper_camel_case(name);
  fmt::format_to(std::back_inserter(hpp),
                 R"_(
  /** @brief Get embedded file content for {1} **/
  std::string_view get{0}();
)_",
                 name_, value);

  fmt::format_to(std::back_inserter(cpp),
                 R"_(
  namespace internal
  {{
    constexpr char ContentFor{0}[] = {{ )_",
                 name_);
  if (!file.empty())
  {
    auto last = file.back();
    file.pop_back();
    for (auto c : file)
      fmt::format_to(std::back_inserter(cpp), "{}, ", (int)c);
    fmt::format_to(std::back_inserter(cpp), "{} ", (int)last);
  }

  fmt::format_to(std::back_inserter(cpp), R"_( }};
  }}
  std::string_view get{0}() {{ return std::string_view(internal::ContentFor{0}, sizeof(internal::ContentFor{0})); }}
)_",
                 name_);
}
