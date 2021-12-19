
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

void cmake_config(nsbuild const& bc, std::vector<std::string> args,
                         std::string src, std::filesystem::path wd)
{

  if (!bc.cmakeinfo.cmake_config.empty() && !bc.cmakeinfo.is_multi_cfg)
    args.emplace_back(cmake::dset("CMAKE_BUILD_TYPE", bc.cmakeinfo.cmake_config));

  if (!bc.cmakeinfo.cmake_toolchain.empty())
    args.emplace_back(
        cmake::dset("CMAKE_TOOLCHAIN_FILE", bc.cmakeinfo.cmake_toolchain));

  if (!bc.cmakeinfo.cmake_generator.empty())
    args.emplace_back(
        cmake::dset("CMAKE_GENERATOR", bc.cmakeinfo.cmake_generator));

  if (!bc.cmakeinfo.cmake_generator_platform.empty())
    args.emplace_back(cmake::dset("CMAKE_GENERATOR_PLATFORM",
                                  bc.cmakeinfo.cmake_generator_platform));

  if (!bc.cmakeinfo.cmake_generator_instance.empty())
    args.emplace_back(cmake::dset("CMAKE_GENERATOR_INSTANCE",
                                  bc.cmakeinfo.cmake_generator_instance));

  if (!bc.cmakeinfo.cmake_generator_toolset.empty())
    args.emplace_back(cmake::dset("CMAKE_GENERATOR_TOOLSET",
                                  bc.cmakeinfo.cmake_generator_toolset));
  args.emplace_back("-S");
  args.emplace_back(std::move(src));
  cmake(bc, std::move(args), wd);
}

void cmake_build(nsbuild const& bc, std::string_view target,
                        std::filesystem::path wd)
{
  std::vector<std::string> args;
  args.emplace_back("--build");
  args.emplace_back(".");
  if (bc.cmakeinfo.is_multi_cfg)
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

void cmake_install(nsbuild const& bc, std::string_view prefix,
                          std::filesystem::path wd)
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

void cmake(nsbuild const& bc, std::vector<std::string> args,
                  std::filesystem::path wd)
{
  std::vector<char const*> sargs;
  sargs.reserve(args.size() + 1);
  sargs.emplace_back(bc.cmakeinfo.cmake_bin.c_str());
  for (auto const& a : args)
    sargs.emplace_back(a.c_str());

  auto save = std::filesystem::current_path();
  std::filesystem::current_path(wd);

  reproc::arguments pargs{sargs.data()};
  reproc::process   proc;
  auto              rc = proc.start(pargs);

  std::filesystem::current_path(save);
  if (rc)
  {
    throw std::system_error(rc);
  }

  std::string          output;
  reproc::sink::string sink(output);
  rc = reproc::drain(proc, sink, reproc::sink::null);
  if (rc)
    nslog::error(rc.message());
  else
    nslog::print(output);
}

} // namespace nsprocess