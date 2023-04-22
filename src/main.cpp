#define NEO_HEADER_ONLY_IMPL
#include <iostream>
#include <neo_script.hpp>
#include <nsbuild.h>
#include <nsprocess.h>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define NS_DLL_EXT "(\\.dll$)|(\\.json$)|(\\.conf)"
#include <Windows.h>
#else
#define NS_DLL_EXT "(\\.so[\\.0-9]*$)|(\\.json$)|(\\.conf)"
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
    else if (arg.starts_with("--no-fetch-build"))
      cfg.cmake_skip_fetch_builds = true;
  }
  return cfg;
}

void halt()
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
  MessageBoxA(0, "Halt here", "Captions", 0);
#endif
}
int main(int argc, char const* argv[])
{
  #ifndef NDEBUG
  halt();
  #endif
  
  std::string working_dir = ".";
  std::string target      = "";
  std::string preset      = "";
  std::string filepfx      = "";
  std::string apipfx      = "";
  nscmakeinfo nscfg;
  runas       ras      = runas::main;
  nsprocess::s_nsbuild = std::filesystem::absolute(argv[0]);
  
  for (int i = 1; i < argc; ++i)
  {
    std::string_view arg = argv[i];

    if (arg == "--check" || arg == "-c")
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
      {
        target = argv[i + 1];
        i++;
      }
      if (i + 1 < argc)
      {
        preset = argv[i + 1];
        i++;
      }
      if (i + 1 < argc)
      {
        apipfx = argv[i + 1];
        i++;
      }
      if (i + 1 < argc)
      {
        filepfx = argv[i + 1];
        i++;
      }
    }
    else if (arg == "--copy-media" || arg == "-e")
    {
      std::string from   = "";
      std::string to     = "";
      std::string artef  = "";
      std::string ignore = "";
      ras                = runas::copy_media;
      if (i + 1 < argc)
        from = argv[++i];
      if (i + 1 < argc)
        to = argv[++i];
      if (i + 1 < argc)
        artef = argv[++i];
      if (i + 1 < argc)
        ignore = argv[++i];
      nsbuild::copy_media(from, to, artef, ignore);
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
      build.main_project();
      break;
    case runas::generate_enum:
      build.generate_enum(filepfx, apipfx, target, preset);
      break;
    case runas::check:
      build.dll_ext   = std::regex(NS_DLL_EXT, std::regex_constants::icase);
      build.cmakeinfo = nscfg;
      build.before_all();
      break;
    }
  }
  catch (module_regenerated ex)
  {
    return -30; // code -30 means rebuild
  }
  catch (std::exception ex)
  {
    nslog::print("******************************************");
    nslog::print(fmt::format("*** Build failure: {}            ***", ex.what()));
    nslog::print("******************************************\n");
    return -1; // A step failed
  }

  return 0;
}
