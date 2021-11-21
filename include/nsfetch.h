#pragma once 
#include <nsbuildcmds.h>
#include <nscommon.h>

struct nsfetch
{
  std::string_view              license;
  std::string_view              commit;
  std::vector<std::string_view> args;
  nsbuildcmdlist                config;
  nsbuildcmdlist                build;
  nsbuildcmdlist                install;
  bool                          cmake_build = false;
};
