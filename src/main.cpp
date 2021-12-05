#define NEO_HEADER_ONLY_IMPL
#include <iostream>
#include <neo_script.hpp>
#include <nsbuild.h>

ns_registry(nsbuild);

enum class runas
{
  generate,
  generate_enum,
  copy_media,
};

int main(int argc, char const* argv[])
{
  std::string working_dir = ".";
  std::string target_os   = ".";
  std::string target      = "";
  runas       ras         = runas::generate;
  for (int i = 1; i < argc; ++i)
  {
    std::string_view arg = argv[i];

    if (arg == "--source" || arg == "-s")
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
      ras = runas::copy_media;
      if (i + 1 < argc)
        target = argv[i + 1];
    }
  }

  nsbuild build;
  neo_register(nsbuild, build.reg);

  try
  {

    build.scan_main(working_dir);
    switch (ras)
    {
    case runas::generate_enum:
      build.generate_enum(target);
      break;
    case runas::generate:
      build.generate_cl();
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
