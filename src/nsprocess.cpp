
#include <fmt/format.h>
#include <iostream>
#include <nsbuild.h>
#include <nscmake.h>
#include <nslog.h>
#include <nsprocess.h>
#include <reproc++/drain.hpp>
#include <reproc++/reproc.hpp>
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

void cmake(nsbuild const& bc, std::vector<std::string> args, std::filesystem::path wd)
{
  std::vector<char const*> sargs;
  sargs.reserve(args.size() + 1);
  sargs.emplace_back(bc.cmakeinfo.cmake_bin.c_str());
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

  auto              rc = proc.start(pargs, default_opt);

  std::filesystem::current_path(save);

  std::string msg = rc.message();
  if (rc)
  {
    nslog::error("Build failed.");
    if (!msg.empty())
      nslog::error(msg);
    throw std::system_error(rc);
  }
}

std::filesystem::path s_nsbuild;
std::filesystem::path get_nsbuild_path() { return s_nsbuild; }

} // namespace nsprocess