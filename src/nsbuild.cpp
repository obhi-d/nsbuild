#include "nsbuild.h"

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

void nsbuild::scan_main(std::string_view sp)
{
  scan_path = sp;
  wd        = std::filesystem::current_path();
  pwd       = wd;
  auto spwd = pwd / scan_path;
  pwd       = spwd / "Build.ns";
  scan_file();

  // make sure build_dir is pointing to build_dir/cmake_config

  pwd = spwd; // move to Build.ns parent path
}

bool nsbuild::scan_file(std::string* sha)
{
  std::ifstream iff(pwd);
  if (iff.is_open())
  {
    std::string f1_str((std::istreambuf_iterator<char>(iff)),
                       std::istreambuf_iterator<char>(iff));
    contents.emplace_back(std::move(f1_str));

    neo::state_machine sm{reg, this};
    sm.parse(pwd.filename().string(), contents.back());
    handle_error(sm);
    if (sha)
    {
      picosha2::hash256_hex_string(contents.back(), *sha);
    }

    return true;
  }
  return false;
}

int nsbuild::generate_cl()
{
  nsbuild* me = this;
  {
    auto spwd = pwd;
    pwd       = pwd / build_dir / ".modulets";
    read_timestamps();
    pwd = spwd;
  }

  for_each_framework(
      [this]()
      {
        if (scan_file())
        {
          auto fwname = pwd.parent_path().filename().string();
          add_framework(fwname);
          for_each_module(
              [this, &fwname]()
              {
                auto mod_name  = pwd.parent_path().filename().string();
                auto targ_name = target_name(fwname, mod_name);
                add_module(mod_name);
                if (!frameworks.back().excludes.contains(targ_name))
                {
                  std::ifstream iff(pwd);
                  if (!iff.is_open())
                  {
                    throw std::runtime_error(targ_name + " did not open");
                  }

                  {

                    std::string f1_str((std::istreambuf_iterator<char>(iff)),
                                       std::istreambuf_iterator<char>(iff));

                    contents.emplace_back(std::move(f1_str));
                  }

                  std::string hash_hex_str;
                  picosha2::hash256_hex_string(contents.back(), hash_hex_str);

                  // Check if timestamp has changed
                  auto meta = timestamps.find(targ_name);
                  if (meta == timestamps.end() || meta->second != hash_hex_str)
                  {
                    nslog::warn(fmt::format("{} has changed. Regenerating!",
                                            targ_name));
                    s_nsmodule->regenerate = true;
                    regenerate_main        = true;
                    timestamps[targ_name]  = hash_hex_str;
                  }

                  neo::state_machine sm{reg, this};
                  sm.parse(pwd.filename().string(), contents.back());
                  handle_error(sm);

                  // Add a target
                  auto t = targets.emplace(targ_name, nstarget{});
                  t.first->second.sha256 = hash_hex_str;
                  t.first->second.fw_idx =
                      static_cast<std::uint32_t>(frameworks.size() - 1);
                  t.first->second.mod_idx = static_cast<std::uint32_t>(
                      frameworks.back().modules.size() - 1);
                }
                s_nsmodule = nullptr;
              });
          s_nsframework = nullptr;
        }
      });

  update_macros();
  process_targets();
  if (regenerate_main)
    process_main();
  return 0;
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

void nsbuild::read_timestamps()
{
  std::ifstream iff(pwd);
  if (!iff.is_open())
  {
    nslog::print("Timestamp/SHA file was missing!");
    return;
  }
  std::string name, sha;
  while (iff >> name)
  {
    if (iff >> sha)
    {
      timestamps.emplace(name, sha);
    }
  }
}

template <typename L>
void nsbuild::for_each_framework(L&& l)
{
  auto pwds  = pwd;
  auto fwdir = pwd / frameworks_dir;
  for (auto const& dir_entry : std::filesystem::directory_iterator{fwdir})
  {
    // for each fw
    pwd = dir_entry.path();
    pwd = pwd / "Framework.ns";
    if (std::filesystem::exists(pwd))
    {
      l();
    }
  }
  pwd = pwds;
}

template <typename L>
void nsbuild::for_each_module(L&& l)
{
  auto pwds = pwd;
  for (auto const& dir_entry : std::filesystem::directory_iterator{pwds})
  {
    // for each fw
    pwd = dir_entry.path();
    pwd = pwd / "Module.ns";
    if (std::filesystem::exists(pwd))
    {
      l();
    }
  }
  pwd = pwds;
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
          process.emplace_back(std::move(std::async(
              std::launch::async, &nsmodule::write, &m, std::cref(*this))));
        }
      });

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

void nsbuild::before_all(std::string_view build_dir, std::string_view src_dir)
{
  std::filesystem::path p    = build_dir;
  std::string           type = p.filename().generic_string();
  if (type.empty())
    type = p.parent_path().filename().generic_string();
  if (type.empty())
  {
    throw std::runtime_error("Build config type could not be determined.");
  }
  scan_main(src_dir);
}

void nsbuild::generate_enum(std::string target)
{
  pwd       = std::filesystem::current_path();
  auto spwd = pwd / scan_path;

  std::filesystem::path path = target;
  if (path.is_relative())
    path = spwd / frameworks_dir / path;

  auto [fw, mod] = get_modid(target);
  add_framework(fw);
  add_module(mod);

  neo::state_machine sm{reg, this};
  stop_after_modtype = true;
  sm.parse(pwd.filename().string(), contents.back());
  handle_error(sm);

  auto gen_path = spwd / build_dir / target / k_gen_dir;
  if (!std::filesystem::create_directories(gen_path))
    throw std::runtime_error(fmt::format("Failed to create directory: {}",
                                         gen_path.generic_string()));
  python.enumspy(mod, to_string(s_nsmodule->type), path.generic_string(),
                 gen_path.generic_string());
}

modid nsbuild::get_modid(std::string_view path)
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
  pwd       = std::filesystem::current_path();
  auto spwd = pwd / scan_path;

  macros["config_scan_dir"]       = spwd.generic_string();
  macros["config_build_dir"]      = (spwd / build_dir).generic_string();
  macros["config_sdk_dir"]        = (spwd / sdk_dir).generic_string();
  macros["config_frameworks_dir"] = (spwd / frameworks_dir).generic_string();
  macros["config_runtime_dir"]    = (spwd / runtime_dir).generic_string();
  macros["config_download_dir"]   = (spwd / download_dir).generic_string();
  macros["config_build_type"] = "$<IF:$<CONFIG:Debug>,Debug,RelWithDebInfo>";
  macros["config_platform"]   = config.target_platform;
}
