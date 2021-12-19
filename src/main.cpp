#define NEO_HEADER_ONLY_IMPL
#include <iostream>
#include <neo_script.hpp>
#include <nsbuild.h>

ns_registry(nsbuild);

nscmakeinfo read_config(char const* argv[], int i, int argc) 
{
  nscmakeinfo cfg;
  if (i < argc)
    cfg.cmake_bin = argv[i++];
  if (i < argc)
    cfg.cmake_build_dir = argv[i++];
  if (i < argc)
    cfg.cmake_config = argv[i++];
  if (i < argc)
    cfg.cmake_cppcompiler = argv[i++];
  if (i < argc)
    cfg.cmake_cppcompiler_version = argv[i++];
  if (i < argc)
    cfg.cmake_generator = argv[i++];
  if (i < argc)
    cfg.cmake_generator_instance = argv[i++];
  if (i < argc)
    cfg.cmake_generator_platform = argv[i++];
  if (i < argc)
    cfg.cmake_generator_toolset = argv[i++];
  if (i < argc)
    cfg.cmake_toolchain = argv[i++];
  if (i < argc)
    cfg.target_platform = argv[i++];
  return cfg;
}

int      main(int argc, char const* argv[])
{
  std::string working_dir = ".";
  std::string target      = "";
  nscmakeinfo nscfg;
  runas       ras = runas::main;
  ide_type    ide = ide_type::all;
  std::string project;
  for (int i = 1; i < argc; ++i)
  {
    std::string_view arg = argv[i];

    if (arg == "--project" || arg == "-p")
    {
      if (i + 1 < argc)
        project = argv[i + 1];
      i++;
    }
    else if (arg == "--ide" || arg == "-i")
    {
      if (i + 1 < argc)
        ide = get_ide(argv[i + 1]);
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
      build.main_project(project, ide);
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
    std::cerr << ex.what() << std::endl;
    return -1;
  }

  return 0;
}
