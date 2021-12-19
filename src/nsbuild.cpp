#include "nsbuild.h"

#include "nsenums.h"
#include "nside.h"
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
#include <stdexcept>

void nsbuild::main_project(std::string_view proj, ide_type ide)
{ 
  
  auto cml = get_full_scan_dir() / "CMakeLists.txt";
  
  {
    std::ofstream ff{cml};
    ff << fmt::format(cmake::k_main_preamble, proj, out_dir);
  }

  {
    auto          cml = get_full_out_dir() / "CMakeLists.txt";
    std::ofstream off{cml};
    off << "";
  }

  nside::write(ide, *this);
}

bool nsbuild::scan_file(std::filesystem::path path, bool store,
                        std::string* sha)
{
  std::ifstream iff(path);
  if (iff.is_open())
  {
    std::string f1_str((std::istreambuf_iterator<char>(iff)),
                       std::istreambuf_iterator<char>(iff));
    contents.emplace_back(std::move(f1_str));

    neo::state_machine sm{reg, this};
    sm.parse(path.filename().string(), contents.back());
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

void nsbuild::before_all()
{
  // At this point we have read config!
  determine_build_type();
  scan_main(get_full_scan_dir());
  read_meta(get_full_cache_dir() / ".meta");
  act_meta();
  foreach_framework([this](std::filesystem::path p) { read_framework(p); });
  update_macros();
  process_targets();
  write_meta(get_full_cache_dir() / ".meta");
  generate_main_config();
}

void nsbuild::determine_build_type()
{
  auto it = cmakeinfo.cmake_build_dir.find_last_of("/\\");
  if (it == std::string::npos)
    throw std::runtime_error("Build config type could not be determined.");
  config_name = cmakeinfo.cmake_build_dir.substr(it + 1);
}

void nsbuild::read_meta(std::filesystem::path path)
{
  if (!scan_file(path, false))
  {
    nslog::print("Meta info file was missing! Will regenerate!");
    state.meta_missing       = true;
    state.is_dirty           = true;
    state.full_regenerate    = true;
    state.delete_cmake_cache = true;
    state.delete_timestamps  = true;
    return;
  }
  if (meta.compiler_version != cmakeinfo.cmake_cppcompiler_version)
  {
    meta.compiler_version    = cmakeinfo.cmake_cppcompiler_version;
    state.delete_cmake_cache = true;
    state.delete_timestamps  = true;
    state.is_dirty           = true;
  }
  if (meta.compiler_name != cmakeinfo.cmake_cppcompiler)
  {
    meta.compiler_name       = cmakeinfo.cmake_cppcompiler;
    state.delete_cmake_cache = true;
    state.delete_timestamps  = true;
    state.is_dirty           = true;
  }
}

void nsbuild::act_meta() 
{ 
  if (state.delete_cmake_cache)
  {
    // TODO find paths to CMakeCache.txt and delete them
  }

  if (state.delete_timestamps)
  {
    // TODO find paths to -stamp and delete them
  }
}

void nsbuild::write_meta(std::filesystem::path path)
{
  if (!state.is_dirty)
    return;

  std::ofstream ofs{path};

  ofs << "meta {";
  ofs << fmt::format("\n compiler_version \"{}\";", meta.compiler_version)
      << fmt::format("\n compiler_name \"{}\";", meta.compiler_name)
      << "\n timestamps {";
  for (auto& t : meta.timestamps)
    ofs << fmt::format("\n  {} : \"{}\";", t.first, t.second);

  ofs << "\n }\n}";
}

void nsbuild::scan_main(std::filesystem::path sp)
{
  scan_file(sp / "Build.ns", true);
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
  auto fwname    = sp.parent_path().string();
  auto targ_name = target_name(fwname, mod_name);
  add_module(mod_name);
  if (!frameworks.back().excludes.contains(targ_name))
  {
    std::string hash_hex_str;
    scan_file(sp / "Module.ns", true, &hash_hex_str);

    // Check if timestamp has changed
    auto ts = meta.timestamps.find(targ_name);
    if (ts == meta.timestamps.end() || ts->second != hash_hex_str || state.full_regenerate)
    {
      nslog::warn(fmt::format("{} has changed. Regenerating!", targ_name));
      s_nsmodule->regenerate     = true;
      state.is_dirty             = true;
      meta.timestamps[targ_name] = hash_hex_str;
    }
    // Add a target
    auto t                 = targets.emplace(targ_name, nstarget{});
    t.first->second.sha256 = hash_hex_str;
    t.first->second.fw_idx = static_cast<std::uint32_t>(frameworks.size() - 1);
    t.first->second.mod_idx =
        static_cast<std::uint32_t>(frameworks.back().modules.size() - 1);
  }
}

void nsbuild::generate_main_config()
{
  if (!state.is_dirty)
    return;

  // get_full_out_dir() / "CMakeLists.txt";
  std::string redirect =
      fmt::format("\nadd_subdirectory(${{CMAKE_CURRENT_LISTS_DIR}}/{0}  ${{CMAKE_CURRENT_LISTS_DIR}}/{0})",
                  config_name);
  bool rewrite = true;
  auto cml     = get_full_out_dir() / "CMakeLists.txt";
  if (std::filesystem::exists(cml))
  {
    std::ifstream iff{cml};
    std::string   f1_str((std::istreambuf_iterator<char>(iff)),
                       std::istreambuf_iterator<char>(iff));
    if (f1_str == redirect)
      rewrite = false;
  }
  if (rewrite)
  {
    std::ofstream off{cml};
    off << redirect;
  }
}

template <typename L>
void nsbuild::foreach_framework(L&& l)
{
  auto fwdir = get_full_scan_dir() / frameworks_dir;
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
  {
    process_target(targ.first, targ.second);
  }
  for (auto& f : process)
    f.wait();
  process.clear();

  foreach_module(
      [this](nsmodule const& m)
      {
        if (m.regenerate)
        {
          process.emplace_back(std::move(std::async(std::launch::async,
                                                    &nsmodule::write_main_build,
                                                    &m, std::cref(*this))));
        }
      });
  write_include_modules();
  for (auto& f : process)
    f.wait();
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
      [this](auto dep)
      {
        std::string name{dep};
        process_target(name, targets[name]);
      });
  mod.foreach_dependency(
      [this](auto dep)
      {
        std::string name{dep};
        process_target(name, targets[name]);
      });

  sorted_targets.push_back(name);
  if (mod.regenerate)
    process.emplace_back(std::move(
        std::async(std::launch::async, &nsmodule::process, &mod,
                   std::cref(*this), std::cref(name), std::ref(targ))));
}

void nsbuild::generate_enum(std::string from)
{
  auto spwd         = get_full_scan_dir();
  auto [fw, mod]    = get_modid(from);
  auto mod_src_path = spwd / frameworks_dir / fw / mod;
  add_framework(fw);
  add_module(mod);
  scan_file(mod_src_path / "Module.ns", true);

  auto gen_path = s_nsmodule->get_full_gen_dir(*this);
  nsenum_context::generate(std::string{mod}, s_nsmodule->type, mod_src_path,
                           gen_path);
}

void nsbuild::copy_media(std::filesystem::path from, std::filesystem::path to,
                         std::string ignore)
{
  namespace fs  = std::filesystem;
  auto       it = std::filesystem::directory_iterator{from};
  const auto copy_options =
      fs::copy_options::update_existing | fs::copy_options::recursive;
  for (auto const& dir_entry : it)
  {
    if (dir_entry.is_directory() && dir_entry.path().filename() == ignore)
      continue;
    std::filesystem::copy(dir_entry.path(), to / dir_entry.path().filename(),
                          copy_options);
  }
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
  auto fwname = path.substr(start, fw - start);
  auto last   = path.find_first_of("/\\", fw + 1);
  auto modname =
      path.substr(fw + 1, (last == path.npos) ? last : last - fw - 1);
  return modid{fwname, modname};
}

void nsbuild::update_macros()
{
  auto pwd = get_full_scan_dir();

  macros["config_build_dir"] = cmake::path(get_full_build_dir());
  macros["config_sdk_dir"]   = cmake::path(get_full_cfg_dir() / sdk_dir);
  macros["config_rt_dir"]    = cmake::path(get_full_cfg_dir() / runtime_dir);
  macros["config_frameworks_dir"] = cmake::path((pwd / frameworks_dir));
  macros["config_download_dir"]   = cmake::path((pwd / out_dir));
  macros["config_name"]           = config_name;
  macros["config_type"]           = cmakeinfo.cmake_config;
  macros["config_platform"]       = cmakeinfo.target_platform;
  macros["config_ignored_media"]  = ignore_media_from;
}

void nsbuild::write_include_modules() const
{
  if (!state.is_dirty)
    return;

  auto          cmlf = get_full_cfg_dir() / "CMakeLists.txt";
  std::ofstream ofs{cmlf};
  if (!ofs)
  {
    nslog::error(fmt::format("Failed to write to : {}", cmlf.generic_string()));
    throw std::runtime_error("Could not create CMakeLists.txt");
  }
  macros.print(ofs);
  for (auto const& s : sorted_targets)
  {
    auto const& m = get_module(s);
    ofs << fmt::format("\nadd_subdirectory({0}/{1} {0}/{1})", build_dir, m.name);
  }
}
