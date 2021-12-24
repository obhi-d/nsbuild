#define NEO_HEADER_ONLY_IMPL
#include <iostream>
#include <neo_script.hpp>
#include <nsbuild.h>
#include <nsprocess.h>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <Windows.h>
#endif

#include <nslog.h>

ns_registry(nsbuild);

nscmakeinfo read_config(char const* argv[], int i, int argc)
{
  nscmakeinfo cfg;
  if (i < argc)
    cfg.cmake_preset_name = argv[i++];
  if (i < argc)
    cfg.cmake_bin = argv[i++];
  if (i < argc)
    cfg.cmake_build_dir = argv[i++];
  for (; i < argc; ++i)
  {
    std::string_view arg = argv[i];
    if (arg.starts_with("-B="))
      cfg.cmake_config = arg.substr(3);
    else if (arg.starts_with("-X="))
      cfg.cmake_cppcompiler = arg.substr(3);
    else if (arg.starts_with("-C="))
      cfg.cmake_ccompiler = arg.substr(3);
    else if (arg.starts_with("-V="))
      cfg.cmake_cppcompiler_version = arg.substr(3);
    else if (arg.starts_with("-G="))
      cfg.cmake_generator = arg.substr(3);
    else if (arg.starts_with("-I="))
      cfg.cmake_generator_instance = arg.substr(3);
    else if (arg.starts_with("-U="))
      cfg.cmake_generator_platform = arg.substr(3);
    else if (arg.starts_with("-H="))
      cfg.cmake_generator_toolset = arg.substr(3);
    else if (arg.starts_with("-T="))
      cfg.cmake_toolchain = arg.substr(3);
    else if (arg.starts_with("-N="))
      cfg.cmake_is_multi_cfg = to_bool(arg.substr(3));
    else if (arg.starts_with("-P="))
      cfg.target_platform = arg.substr(3);
  }
  return cfg;
}

int main(int argc, char const* argv[])
{
  MessageBoxA(0, "Halt here", "Captions", 0);
  std::string working_dir = ".";
  std::string target      = "";
  nscmakeinfo nscfg;
  runas       ras = runas::main;
  std::string project;
  nsprocess::s_nsbuild = argv[0];
  for (int i = 1; i < argc; ++i)
  {
    std::string_view arg = argv[i];

    if (arg == "--project" || arg == "-p")
    {
      if (i + 1 < argc)
        project = argv[i + 1];
      i++;
    }
    else if (arg == "--check" || arg == "-c")
    {
      ras   = runas::check;
      nscfg = read_config(argv, i + 1, argc);
    }
    else if (arg == "--source" || arg == "-s")
    {
      if (i + 1 < argc)
        working_dir = argv[i + 1];
      i++;
    }
    else if (arg == "--gen-enum")
    {
      ras = runas::generate_enum;
      if (i + 1 < argc)
        target = argv[i + 1];
    }
    else if (arg == "--copy-media" || arg == "-e")
    {
      std::string from   = "";
      std::string to     = "";
      std::string ignore = "";
      ras                = runas::copy_media;
      if (i + 1 < argc)
        from = argv[++i];
      if (i + 1 < argc)
        to = argv[++i];
      if (i + 1 < argc)
        ignore = argv[++i];
      nsbuild::copy_media(from, to, ignore);
      std::cout << std::endl;
      return 0;
    }
  }

  nsbuild build;
  build.state.ras = ras;
  neo_register(nsbuild, build.reg);

  try
  {

    build.scan_main(working_dir);
    switch (ras)
    {
    case runas::main:
      build.main_project(project);
      break;
    case runas::generate_enum:
      build.generate_enum(target);
      break;
    case runas::check:
      build.cmakeinfo = nscfg;
      build.before_all();
      break;
    }
  }
  catch (std::exception ex)
  {
    nslog::error(ex.what());
    std::cout << std::endl;
    return -1;
  }

  return 0;
}
