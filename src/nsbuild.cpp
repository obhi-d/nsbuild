#include "nsbuild.h"

#include "nscmake.h"
#include "nscmake_conststr.h"
#include "nsenums.h"
#include "nsheader_map.h"
#include "picosha2.h"

#include <exception>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <fstream>
#include <future>
#include <iomanip>
#include <iterator>
#include <mutex>
#include <nslog.h>
#include <nsprocess.h>
#include <stdexcept>
#include <string>
#include <unordered_set>

extern void halt();
nsbuild::nsbuild()
{
  wd                              = std::filesystem::current_path();
  macros["config_source"]         = "";
  macros["config_build_dir"]      = "";
  macros["config_sdk_dir"]        = "";
  macros["config_rt_dir"]         = "";
  macros["config_frameworks_dir"] = "";
  macros["config_dl_dir"]         = "";
  macros["config_preset_name"]    = "";
  macros["config_type"]           = "";
  macros["config_platform"]       = "";
  macros["config_ignored_media"]  = "";
}

void nsbuild::main_project()
{
  compute_paths({});
  auto cml = get_full_source_dir() / "CMakeLists.txt";

  {
    std::ofstream ff{cml};
    ff << fmt::format(cmake::k_main_preamble, project_name, out_dir, cmake::path(nsprocess::get_nsbuild_path()),
                      cmake_gen_dir, version);
  }

  {
    auto          cml = get_full_source_dir() / "CMakePresets.json";
    std::ofstream ff{cml};
    nspreset::write(ff, 0, {}, *this);
  }
  // create preset empty directories
  for (auto const& preset : presets)
  {
    auto path = get_full_out_dir() / preset.name / cmake_gen_dir / "CMakeLists.txt";
    if (!std::filesystem::exists(path))
    {
      std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream ff{path};
    ff << "\nmessage(\"-- Run build for the first time to generate project files.\")\n";
    path = get_full_out_dir() / preset.name / cache_dir;
    if (std::filesystem::exists(path))
    {
      auto dit = std::filesystem::directory_iterator(path);
      for (auto const& it : dit)
      {
        if (it.path().extension() == ".glob")
          std::filesystem::remove(it.path());
      }
    }
  }
}

bool nsbuild::scan_file(std::filesystem::path path, bool store, std::string* sha)
{
  std::ifstream iff(path);
  if (iff.is_open())
  {
    std::string f1_str;
    auto        size = std::filesystem::file_size(path);
    f1_str.resize(size, ' ');
    iff.read(f1_str.data(), size);

    contents.emplace_back(std::move(f1_str));

    neo::state_machine sm{reg, this};
    sm.parse(path.string(), contents.back());
    handle_error(sm);
    if (sha)
    {
      picosha2::hash256_hex_string(contents.back(), *sha);
    }
    if (!store)
      contents.pop_back();
    return true;
  }
  return false;
}

void nsbuild::handle_error(neo::state_machine& err)
{
  if (err.fail_bit())
  {
    std::string all;
    err.for_each_error(
        [&all](std::string const& e)
        {
          all += e;
          all += "\n";
        });
    throw std::runtime_error(all);
  }
}

void nsbuild::clean_install()
{
  for (auto const& preset : presets)
    if (preset.name == cmakeinfo.cmake_preset_name)
    {
      s_current_preset = &preset;
      break;
    }
  // At this point we have read config!
  compute_paths(cmakeinfo.cmake_preset_name);
  std::filesystem::create_directories(get_full_cache_dir());
  read_meta(get_full_cache_dir());
  foreach_framework([this](std::filesystem::path p) { read_framework(p); });
  state.delete_builds = true;
  delete_builds_if_required();
}

void nsbuild::before_all()
{
  for (auto const& preset : presets)
    if (preset.name == cmakeinfo.cmake_preset_name)
    {
      s_current_preset = &preset;
      break;
    }
  if (s_current_preset->static_plugins)
    nslog::print("Plugin modules will link statically");
  // At this point we have read config!
  compute_paths(cmakeinfo.cmake_preset_name);
  std::filesystem::create_directories(get_full_cache_dir());
  read_meta(get_full_cache_dir());
  act_meta();
  foreach_framework([this](std::filesystem::path p) { read_framework(p); });
  delete_builds_if_required();
  update_macros();
  try
  {
    install_cache.load(get_full_cache_dir() / "install_cache.txt");
    process_targets();
    install_cache.uninstall_unused_and_save(get_full_cache_dir() / "install_cache.txt");
  }
  catch (std::exception&)
  {
    nslog::print("******************************************");
    nslog::print("*** Module failed to build!            ***");
    nslog::print("******************************************\n");
    // Still write out meta
    write_meta(get_full_cache_dir());
    throw;
  }
  copy_installed_binaries();
  write_meta(get_full_cache_dir());

  if (state.is_dirty || state.exit_and_rebuild)
  {
    nslog::print("******************************************");
    nslog::print("*** Modules were regenerated, rebuild! ***");
    nslog::print("******************************************\n");
    throw module_regenerated();
  }
  else
  {
    nslog::print("******************************************");
    nslog::print("*** Check finished. Resuming cmake...  ***");
    nslog::print("******************************************\n");
  }
}

void nsbuild::delete_builds_if_required()
{
  if (!state.delete_builds)
    return;
  nsbuild::remove_cache(get_full_build_dir());
  foreach_module([this](auto& m) { m.delete_build(*this); });
  std::error_code ec;
  std::filesystem::remove(get_full_cache_dir() / "module_info.ns", ec);
}

void nsbuild::header_map(std::filesystem::path targ_file)
{
  nsheader_map hmap;
  compute_paths({});
  hmap.scan_frameworks(get_full_source_dir() / frameworks_dir);
  if (targ_file.empty())
  {
    // go through source directory
  }
  else
  {
    hmap.build(targ_file);
    auto html = get_full_out_dir() / (targ_file.stem().string() + ".html");
    hmap.write_html(html, *this);
    std::system(html.string().c_str());
  }
}

bool nsbuild::read_sha(std::string_view name, std::string const& current) const
{
  std::ifstream sha_file(get_full_cache_dir() / fmt::format("{}.sha", name));
  if (sha_file.is_open())
  {
    std::string sha;
    std::getline(sha_file, sha);
    return sha == current;
  }
  return false;
}

void nsbuild::write_sha(std::string_view name, std::string const& current) const
{
  std::ofstream sha_file(get_full_cache_dir() / fmt::format("{}.sha", name));
  if (sha_file.is_open())
  {
    sha_file << current << std::endl;
  }
}

void nsbuild::read_meta(std::filesystem::path const& path)
{
  if (!scan_file(path / "compiler.ns", false))
  {
    nslog::print("Compiler info file was missing! Will force rebuild!");
    state.meta_missing    = true;
    state.is_dirty        = true;
    state.full_regenerate = true;
    return;
  }

  state.is_dirty = !read_sha("main_build", build_ns_sha);

  if (meta.compiler_version != cmakeinfo.cmake_cppcompiler_version)
  {
    meta.compiler_version = cmakeinfo.cmake_cppcompiler_version;
    state.delete_builds   = true;
    state.is_dirty        = true;
  }

  if (meta.compiler_name != cmakeinfo.cmake_cppcompiler)
  {
    meta.compiler_name  = cmakeinfo.cmake_cppcompiler;
    state.delete_builds = true;
    state.is_dirty      = true;
  }
}

void nsbuild::act_meta()
{
  if (state.delete_builds)
  {
    namespace fs = std::filesystem;
    std::error_code ec;
    auto            it = fs::recursive_directory_iterator(get_full_build_dir());
    for (const fs::directory_entry& dir_entry : it)
    {
      if (dir_entry.is_directory())
      {
        auto cache = dir_entry.path() / "CMakeCache.txt";
        if (fs::exists(cache))
          std::filesystem::remove_all(cache, ec);
      }
    }
  }
}

void nsbuild::write_meta(std::filesystem::path const& path)
{
  if (!state.is_dirty)
    return;

  {
    std::ofstream ofs{path / "compiler.ns"};
    ofs << "meta {";
    ofs << fmt::format("\n compiler_version \"{}\";", cmakeinfo.cmake_cppcompiler_version)
        << fmt::format("\n compiler_name \"{}\";", cmakeinfo.cmake_cppcompiler);
    ofs << "\n}";
  }

  write_sha("main_build", build_ns_sha);
}

void nsbuild::scan_main(std::filesystem::path sp)
{
  if (sp.is_absolute())
  {
    wd = sp;
    std::filesystem::current_path(wd);
    scan_file("Build.ns", true, &build_ns_sha);
  }
  else
    scan_file(sp / "Build.ns", true, &build_ns_sha);
}

void nsbuild::read_framework(std::filesystem::path sp)
{
  auto fwname = sp.filename().string();
  add_framework(fwname);
  scan_file(sp / "Framework.ns", false);
  foreach_module([this](std::filesystem::path m) { read_module(m); }, sp);
}

void nsbuild::read_module(std::filesystem::path sp)
{
  auto mod_name  = sp.filename().string();
  auto fwname    = sp.parent_path().filename().string();
  auto targ_name = target_name(fwname, mod_name);
  add_module(mod_name, sp);

  if (!frameworks.back().excludes.contains(mod_name))
  {
    std::string hash_hex_str;
    scan_file(sp / "Module.ns", true, nullptr);
    hash_hex_str = gather_module_hash(sp);
    // Check if timestamp has changed
    s_nsmodule->set_sha(hash_hex_str);
    if (!read_sha(mod_name, hash_hex_str) || state.full_regenerate)
    {
      nslog::warn(fmt::format("{} has changed. Regenerating!", targ_name));

      s_nsmodule->should_regenerate();
      state.is_dirty = true;
    }
    // Add a target
    auto t                  = targets.emplace(targ_name, nstarget{});
    t.first->second.sha256  = hash_hex_str;
    t.first->second.fw_idx  = static_cast<std::uint32_t>(frameworks.size() - 1);
    t.first->second.mod_idx = static_cast<std::uint32_t>(frameworks.back().modules.size() - 1);
  }
  else
    s_nsmodule->disabled = true;
}

std::string nsbuild::gather_module_hash(std::filesystem::path const& path)
{
  std::stringstream buffer;
  buffer << contents.back();
  auto others = std::array{"Prepare.cmake", "Finalize.cmake"};
  for (auto const& o : others)
  {
    auto prepare = path / o;
    if (std::filesystem::exists(prepare))
    {
      std::ifstream iff(prepare);
      buffer << iff.rdbuf();
    }
  }
  auto        content = buffer.str();
  std::string sha;
  picosha2::hash256_hex_string(content, sha);
  return sha;
}

template <typename L>
void nsbuild::foreach_framework(L&& l)
{
  auto fwdir = get_full_source_dir() / frameworks_dir;
  for (auto const& dir_entry : std::filesystem::directory_iterator{fwdir})
  {
    // for each fw
    auto pwd = dir_entry.path();
    if (std::filesystem::exists(pwd / "Framework.ns"))
    {
      l(pwd);
    }
  }
}

template <typename L>
void nsbuild::foreach_module(L&& l, std::filesystem::path fwpath)
{
  for (auto const& dir_entry : std::filesystem::directory_iterator{fwpath})
  {
    // for each fw
    auto pwd = dir_entry.path();
    if (std::filesystem::exists(pwd / "Module.ns"))
    {
      l(pwd);
    }
  }
}

void nsbuild::process_targets()
{
  for (auto& targ : targets)
    process_target(targ.first, targ.second);

  write_include_modules();
  nslog::print("Finished writing targets");
}

void nsbuild::process_target(std::string const& name, nstarget& targ)
{
  if (targ.processed)
    return;
  targ.processed = true;

  auto& fw = frameworks[targ.fw_idx];
  if (!fw.processed)
  {
    fw.processed = true;
    fw.process(*this);
  }
  auto& mod = fw.modules[targ.mod_idx];
  mod.foreach_references(
      [this, &mod](auto dep)
      {
        std::string name{dep};
        auto        it = targets.find(name);
        if (it != targets.end())
          process_target(name, it->second);
        else
          throw std::runtime_error(
              fmt::format("{} Is not a valid module. Used as reference module for - {}", name, mod.target_name));
      });
  mod.foreach_dependency(
      [this, &mod](auto dep)
      {
        std::string name{dep};
        auto        it = targets.find(name);
        if (it != targets.end())
          process_target(name, it->second);
        else
          throw std::runtime_error(
              fmt::format("{} Is not a valid module. Seen as a dependent module for {}", name, mod.name));
      });

  sorted_targets.push_back(name);
  mod.process(*this, install_cache, name, targ);
  if (mod.was_fetch_rebuilt)
    state.exit_and_rebuild = true;
  if (mod.has_globs_changed)
    state.is_dirty = true;
}

void nsbuild::copy_installed_binaries()
{
  std::array<std::string_view, 2> runtime_loc = {"bin", "lib"};
  auto                            bin         = get_full_rt_dir() / "bin";
  for (auto const& l : runtime_loc)
  {
    namespace fs = std::filesystem;

    auto top_path = get_full_sdk_dir() / l;
    if (!std::filesystem::exists(top_path))
      continue;
    auto       it           = fs::recursive_directory_iterator{top_path};
    const auto copy_options = fs::copy_options::update_existing;
    for (auto const& dir_entry : it)
    {
      if (!dir_entry.is_regular_file() && !dir_entry.is_symlink())
        continue;
      auto name = dir_entry.path().filename().generic_string();
      bool copy = false;
      if (std::regex_search(name, dll_ext))
      {
        std::error_code ec;
        auto            path = dir_entry.path();
        auto            rel  = fs::relative(path, top_path, ec);
        auto            dest = bin / rel;

        fs::create_directories(dest.parent_path(), ec);
        std::filesystem::copy(path, dest, copy_options);
      }
    }
  }
}

void nsbuild::generate_enum(std::string filepfx, std::string apipfx, std::string from, std::string preset)
{
  throw std::runtime_error("Not implemented");
  /*
  compute_paths(preset);
  auto spwd         = get_full_source_dir();
  auto [fw, mod]    = get_modid(from);
  auto mod_src_path = spwd / frameworks_dir / fw / mod;
  add_framework(fw);
  add_module(mod);
  scan_file(mod_src_path / "Module.ns", true);

  auto gen_path = s_nsmodule->get_full_gen_dir(*this);
  // nsenum_context::generate(filepfx, apipfx, std::string{mod}, s_nsmodule->type, mod_src_path, gen_path, style);
  */
}

void nsbuild::copy_media(std::filesystem::path from, std::filesystem::path to, std::filesystem::path artefacts,
                         std::string ignore)
{
  if (!std::filesystem::exists(from))
    return;
  std::filesystem::create_directories(to);

  namespace fs            = std::filesystem;
  auto       it           = fs::recursive_directory_iterator{from};
  const auto copy_options = fs::copy_options::update_existing;

  std::unordered_set<fs::path> touched;
  std::unordered_set<fs::path> added;

  if (fs::exists(artefacts))
  {
    std::string   spath;
    std::ifstream ff{artefacts};

    while (!ff.eof())
    {
      std::getline(ff, spath);
      touched.emplace(spath);
    }
  }

  for (auto const& dir_entry : it)
  {
    if (dir_entry.is_directory() && (dir_entry.path().filename() == ignore))
      continue;

    std::error_code ec       = {};
    auto const&     path     = dir_entry.path();
    auto            rel_path = path.lexically_relative(from);
    auto            dest     = to / rel_path;

    if (dir_entry.is_directory())
      fs::create_directories(dest, ec);
    else
    {
      fs::copy(path, dest, copy_options);
      added.emplace(rel_path);
      touched.erase(rel_path);
    }
  }

  for (auto const& file : touched)
  {
    std::error_code ec = {};
    fs::remove(to / file, ec);
  }

  std::ofstream ff{artefacts};
  for (auto const& file : added)
    ff << file << std::endl;
}

modid nsbuild::get_modid(std::string_view path) const
{
  auto it = path.find(frameworks_dir);
  if (it == path.npos)
    throw std::runtime_error("Not a framework path");

  auto start = it + frameworks_dir.length() + 1;
  auto fw    = path.find_first_of("/\\", start);
  if (fw == path.npos)
    throw std::runtime_error("Not a framework path");
  auto fwname  = path.substr(start, fw - start);
  auto last    = path.find_first_of("/\\", fw + 1);
  auto modname = path.substr(fw + 1, (last == path.npos) ? last : last - fw - 1);
  return modid{fwname, modname};
}

void nsbuild::compute_paths(std::string const& preset)
{
  namespace fs = std::filesystem;

  paths.data_dir = nsprocess::get_nsbuild_path().parent_path();

  auto const& preset_name = preset;
  paths.scan_dir          = fs::canonical(wd / scan_dir);
  paths.out_dir           = (paths.scan_dir / out_dir);
  paths.cfg_dir           = (paths.out_dir / preset_name);
  paths.dl_dir            = (paths.out_dir / download_dir);

  fs::create_directories(paths.scan_dir);
  fs::create_directories(paths.out_dir);
  fs::create_directories(paths.cfg_dir);
  fs::create_directories(paths.dl_dir);

  paths.scan_dir = fs::canonical(paths.scan_dir);
  paths.out_dir  = fs::canonical(paths.out_dir);
  paths.cfg_dir  = fs::canonical(paths.cfg_dir);
  paths.dl_dir   = fs::canonical(paths.dl_dir);

  if (!preset_name.empty())
  {
    paths.cmake_gen_dir = (paths.cfg_dir / cmake_gen_dir);
    paths.cache_dir     = (paths.cfg_dir / cache_dir);
    paths.build_dir     = (paths.cfg_dir / build_dir);
    paths.sdk_dir       = (paths.cfg_dir / sdk_dir);
    paths.rt_dir        = (paths.cfg_dir / runtime_dir);
    fs::create_directories(paths.cmake_gen_dir);
    fs::create_directories(paths.cache_dir);
    fs::create_directories(paths.build_dir);
    fs::create_directories(paths.sdk_dir);
    fs::create_directories(paths.rt_dir);
    paths.cmake_gen_dir = fs::canonical(paths.cmake_gen_dir);
    paths.cache_dir     = fs::canonical(paths.cache_dir);
    paths.build_dir     = fs::canonical(paths.build_dir);
    paths.sdk_dir       = fs::canonical(paths.sdk_dir);
    paths.rt_dir        = fs::canonical(paths.rt_dir);
  }
}

void nsbuild::update_macros()
{
  auto pwd = get_full_source_dir();

  macros["config_source"]         = cmake::path(pwd);
  macros["config_build_dir"]      = cmake::path(get_full_cmake_gen_dir());
  macros["config_sdk_dir"]        = cmake::path(get_full_sdk_dir());
  macros["config_rt_dir"]         = cmake::path(get_full_rt_dir());
  macros["config_frameworks_dir"] = cmake::path((pwd / frameworks_dir));
  macros["config_dl_dir"]         = cmake::path(get_full_dl_dir());
  macros["config_preset_name"]    = cmakeinfo.cmake_preset_name;
  macros["config_type"]           = cmakeinfo.cmake_config;
  macros["config_platform"]       = cmakeinfo.target_platform;
  macros["config_ignored_media"]  = media_exclude_filter;
}

void nsbuild::write_include_modules() const
{
  if (!state.is_dirty)
    return;

  auto          cmlf = get_full_cfg_dir() / cmake_gen_dir / "CMakeLists.txt";
  std::ofstream ofs{cmlf};
  if (!ofs)
  {
    nslog::error(fmt::format("Failed to write to : {}", cmlf.generic_string()));
    throw std::runtime_error("Could not create CMakeLists.txt");
  }

  auto const& preset = *s_current_preset;
  cmake::line(ofs, "Setup", '~', true);
  ofs << fmt::format(cmake::k_include_mods_preamble, preset.cppcheck ? "ON" : "OFF", preset.unity_build ? "ON" : "OFF",
                     natvis, namespace_name, macro_prefix, cmake::path(paths.data_dir), file_prefix);

  if (preset.cppcheck)
  {
    auto supression_file_cpy = get_full_cfg_dir() / cmake_gen_dir / "CppCheckSuppressions.txt";
    auto supression_file     = get_full_source_dir() / preset.cppcheck_suppression;
    if (preset.cppcheck_suppression.empty() || !std::filesystem::exists(supression_file))
    {
      std::ofstream ofs{supression_file_cpy};
    }
    else
    {
      std::filesystem::copy_file(supression_file, supression_file_cpy, std::filesystem::copy_options::update_existing);
    }
  }

  macros.print(ofs);
  write_cxx_options(ofs);

  ofs << fmt::format("\nlist(PREPEND CMAKE_MODULE_PATH \"{}\")\n", cmake::path(get_full_sdk_dir()));

  for (auto& sort_targ : sorted_targets)
  {
    auto& m = get_module(sort_targ);
    cmake::line(ofs, m.target_name, '*', true);
    m.write_main_build(ofs, *this);
  }
  write_install_configs(ofs);
}

void nsbuild::write_cxx_options(std::ostream& ofs) const
{
  cmake::line(ofs, "Compiler and Linker options", '*', true);
  ofs << "\nset(__module_cxx_compile_flags)"
         "\nset(__module_cxx_linker_flags)";

  auto const& preset = *s_current_preset;
  for (auto const& cxx : preset.configs)
  {
    auto filters = cmake::get_filter(preset, cxx.filters);
    if (!filters.has_value())
      continue;
    auto value = filters.value();
    if (value.empty())
    {
      for (auto flag : cxx.compiler_flags)
        ofs << fmt::format("\nlist(APPEND __module_cxx_compile_flags \"{}\")", flag);
      for (auto flag : cxx.linker_flags)
        ofs << fmt::format("\nlist(APPEND __module_cxx_linker_flags \"{}\")", flag);
    }
    else
    {
      for (auto flag : cxx.compiler_flags)
        ofs << fmt::format("\nlist(APPEND __module_cxx_compile_flags $<{}:{}>)", value, flag);
      for (auto flag : cxx.linker_flags)
        ofs << fmt::format("\nlist(APPEND __module_cxx_linker_flags $<{}:{}>)", value, flag);
    }
  }

  ofs << "\nadd_compile_options(${__module_cxx_compile_flags})"
         "\nadd_link_options(${__module_cxx_linker_flags})";

  ofs << "\n\n";
}

void nsbuild::write_install_configs(std::ofstream& ofs) const
{
  auto cmlf = get_full_build_dir() / fmt::format("{}Config.cmake", project_name);
  {
    std::ofstream cfg{cmlf};
    cfg << cmake::k_module_install_cfg_in;
  }
  // Write install commands
}
