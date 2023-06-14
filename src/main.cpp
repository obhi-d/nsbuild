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

template <typename T>
bool read(std::string_view name, std::string_view arg, T& to)
{
  if (arg.starts_with(name))
  {
    to = arg.substr(name.length());
    return true;
  }
  return false;
}

template <typename T, typename L>
bool read(std::string_view name, std::string_view arg, T& to, L&& l)
{
  if (arg.starts_with(name))
  {
    to = l(arg.substr(name.length()));
    return true;
  }
  return false;
}

nscmakeinfo read_config(char const* argv[], int i, int argc)
{
  nscmakeinfo cfg;

  for (; i < argc; ++i)
  {
    std::string_view arg = argv[i];
    // clang-format off
    read("--preset=", arg, cfg.cmake_preset_name) || 
    read("--cmake=", arg, cfg.cmake_bin) ||
    read("--binary-dir=", arg, cfg.cmake_build_dir) || 
    read("--build-type=", arg, cfg.cmake_config) ||
    read("--cpp-compiler-id=", arg, cfg.cmake_cppcompiler) || 
    read("--c-compiler-id=", arg, cfg.cmake_ccompiler) ||
    read("--cpp-compiler-ver=", arg, cfg.cmake_cppcompiler_version) ||
    read("--generator=", arg, cfg.cmake_generator) ||
    read("--generator-instance=", arg, cfg.cmake_generator_instance) ||
    read("--generator-platform=", arg, cfg.cmake_generator_platform) ||
    read("--generator-toolset=", arg, cfg.cmake_generator_toolset) ||
    read("--toolchain=", arg, cfg.cmake_toolchain) ||
    read("--is-multiconfig=", arg, cfg.cmake_is_multi_cfg, to_bool) ||
    read("--platform=", arg, cfg.target_platform, to_bool);
    // clang-format on
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
  std::string filepfx     = "";
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
    if (arg == "--clean" || arg == "-c")
    {
      ras   = runas::clean;
      nscfg = read_config(argv, i + 1, argc);
    }
    else if (arg == "--source" || arg == "-s")
    {
      if (i + 1 < argc)
        working_dir = argv[i + 1];
      i++;
    }
    else if (arg == "--header-map" || arg == "-m")
    {
      ras = runas::header_map;
      if (i + 1 < argc)
        target = argv[i + 1];
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
    case runas::header_map:
      build.header_map(target);
      break;
    case runas::main:
      build.main_project();
      break;
    case runas::check:
      build.dll_ext   = std::regex(NS_DLL_EXT, std::regex_constants::icase);
      build.cmakeinfo = nscfg;
      build.before_all();
      break;
    case runas::clean:
      build.dll_ext   = std::regex(NS_DLL_EXT, std::regex_constants::icase);
      build.cmakeinfo = nscfg;
      build.clean_install();
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
