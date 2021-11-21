#include "nsbuild.h"

#include "picosha2.h"

#include <exception>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <stdexcept>

void nsbuild::scan_main(std::string_view scan_path)
{
  pwd       = std::filesystem::current_path();
  auto spwd = pwd / scan_path;
  pwd       = spwd / "Build.ns";
  scan_file();

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

int nsbuild::do_timestamp_checks()
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
          for_each_module(
              [this, &fwname]()
              {
                auto name =
                    fwname + "." + pwd.parent_path().filename().string();
                if (!frameworks.back().excludes.contains(name))
                {
                  std::ifstream iff(pwd);
                  if (!iff.is_open())
                  {
                    throw std::runtime_error(name + " did not open");
                  }
                  std::string f1_str((std::istreambuf_iterator<char>(iff)),
                                     std::istreambuf_iterator<char>(iff));
                  std::string hash_hex_str;
                  picosha2::hash256_hex_string(f1_str, hash_hex_str);
                  auto meta = timestamps.find(name);
                  if (meta == timestamps.end())
                    throw std::runtime_error("Metadata missing");
                  if (meta->second != hash_hex_str)
                  {
                    throw std::runtime_error(name + " has changed");
                  }
                }
              });
        }
      });

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
    throw std::runtime_error("timestamps did not open");
  }
  std::string name, sha;
  while (iff >> name)
  {
    if (iff >> sha)
    {
      timestamps.emplace(std::move(name), std::move(sha));
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

void nsbuild::generate() 
{ 
  read_all();
}

void nsbuild::read_all()
{
  // redo
  frameworks.clear();
  for_each_framework(
      [this]()
      {
        if (scan_file())
        {
          auto fwname = pwd.parent_path().filename().string();
          for_each_module(
              [this, &fwname]()
              {
                auto name =
                    fwname + "." + pwd.parent_path().filename().string();
                std::string sha;
                if (scan_file(&sha))
                {
                  auto t                 = targets.emplace(name, nstarget{});
                  t.first->second.sha256 = sha;
                  t.first->second.fw_idx =
                      static_cast<std::uint32_t>(frameworks.size() - 1);
                  t.first->second.mod_idx = static_cast<std::uint32_t>(
                      frameworks.back().modules.size() - 1);
                }
              });
        }
      });
}