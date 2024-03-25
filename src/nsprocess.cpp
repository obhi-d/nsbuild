
#include <fmt/format.h>
#include <iostream>
#include <nsbuild.h>
#include <nscmake.h>
#include <nslog.h>
#include <nsprocess.h>
#include <reproc++/reproc.hpp>
#include <reproc++/run.hpp>
#include <system_error>

namespace nsprocess
{

void cmake_config(nsbuild const& bc, std::vector<std::string> args, std::string src, std::filesystem::path wd)
{
  args.emplace_back("--preset");
  args.emplace_back(bc.cmakeinfo.cmake_preset_name);
  args.emplace_back("-S");
  args.emplace_back(std::move(src));
  cmake(bc, std::move(args), wd);
}

void cmake_build(nsbuild const& bc, std::string_view target, std::filesystem::path wd)
{
  std::vector<std::string> args;
  args.emplace_back("--build");
  args.emplace_back(".");
  if (bc.cmakeinfo.cmake_is_multi_cfg)
  {
    args.emplace_back("--config");
    args.emplace_back(bc.cmakeinfo.cmake_config.c_str());
  }
  if (!target.empty())
  {
    args.emplace_back("--target");
    args.emplace_back(target.data());
  }

  cmake(bc, std::move(args), wd);
}

void cmake_install(nsbuild const& bc, std::string_view prefix, std::filesystem::path wd)
{
  std::vector<std::string> args;
  args.emplace_back("--install");
  args.emplace_back(".");
  if (!prefix.empty())
  {
    args.emplace_back("--prefix");
    args.emplace_back(prefix);
  }

  cmake(bc, std::move(args), wd);
}

template <typename... Args>
std::vector<std::string> make_args(Args&&... args)
{
  std::vector<std::string> result;
  (result.emplace_back(std::forward<Args>(args)), ...);
  return result;
}

bool download(nsbuild const& bc, std::filesystem::path const& dl, std::string_view const& repo, std::string_view name,
              std::string_view version, bool force)
{
  auto zip =
      name.empty() ? version.empty() ? "source.zip" : fmt::format("{}.zip", version) : fmt::format("{}.zip", name);
  if (std::filesystem::exists(dl / zip) && (!name.empty() || !version.empty()) && !force)
    return false;

  if (!std::filesystem::exists(dl / zip) || name.empty())
#ifdef _WIN64
    powershell(bc, make_args(std::format("Invoke-RestMethod -URI {} -OutFile {}", repo, zip)), dl);
#else
    execute("curl", bc, make_args("-L", "-o", zip, repo), dl);
#endif
  std::error_code ec;
  // remove source file and any existing directories
  auto dir = std::filesystem::directory_iterator(dl);
  for (auto const& d : dir)
  {
    if (d.path().filename() != zip)
      std::filesystem::remove_all(d.path(), ec);
  }

#ifdef _WIN64
  powershell(bc, make_args(std::format("Expand-Archive -Path {} -DestinationPath . -Force", zip)), dl);
#else
  execute("unzip", bc, make_args("-o", zip), dl);
#endif
  return true;
}

void git_clone(nsbuild const& bc, std::filesystem::path const& dl, std::string_view const& repo, std::string_view tag)
{
  std::error_code ec;
  if (!std::filesystem::exists(dl) || std::filesystem::is_empty(dl, ec))
  {
    git(bc, make_args("clone", repo, "--recurse-submodules", "--shallow-submodules", "--branch", tag, "--depth=1", "."),
        dl);
  }
  else
  {
    try
    {
      git(bc, make_args("remote", "set-url", "origin", repo), dl);
      git(bc, make_args("fetch", "--depth=1", "origin", "--recurse-submodules=yes", tag), dl);
      git(bc, make_args("reset", "--hard", "FETCH_HEAD"), dl);
      git(bc, make_args("submodule", "update"), dl);
      git(bc, make_args("clean", "-dfx"), dl);
    }
    catch (std::exception&)
    {
      // try another way
      std::filesystem::remove_all(dl);
      git_clone(bc, dl, repo, tag);
    }
  }
}

void execute(std::string_view name, nsbuild const& bc, std::vector<std::string> args, std::filesystem::path wd)
{
  std::vector<char const*> sargs;
  sargs.reserve(args.size() + 1);
  sargs.emplace_back(name.data());
  for (auto const& a : args)
    sargs.emplace_back(a.c_str());
  sargs.emplace_back(nullptr);
  auto save = std::filesystem::current_path();
  std::filesystem::create_directories(wd);
  std::filesystem::current_path(wd);

  reproc::arguments pargs{sargs.data()};
  reproc::process   proc;
  reproc::options   default_opt;

  default_opt.redirect.parent = true;

  auto [status, rc] = reproc::run(pargs);
  std::filesystem::current_path(save);

  std::string msg = rc.message();
  if (rc || status != 0)
  {
    nslog::error("Build failed.");
    if (rc)
      throw std::system_error(rc);
    else
      throw std::runtime_error(fmt::format("Command returned : {}", status));
  }
}

void cmake(nsbuild const& bc, std::vector<std::string> args, std::filesystem::path wd)
{
  execute(bc.cmakeinfo.cmake_bin.c_str(), bc, std::move(args), std::move(wd));
}

void git(nsbuild const& bc, std::vector<std::string> args, std::filesystem::path wd)
{
  execute("git", bc, std::move(args), std::move(wd));
}

void powershell(nsbuild const& bc, std::vector<std::string> args, std::filesystem::path wd)
{
  execute("powershell.exe", bc, std::move(args), std::move(wd));
}

std::filesystem::path s_nsbuild;
std::filesystem::path get_nsbuild_path() { return s_nsbuild; }

} // namespace nsprocess