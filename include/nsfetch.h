#pragma once
#include <nsbuildcmds.h>
#include <nscommon.h>
#include <nsparams.h>

struct nsfetch
{
  std::string_view              repo;
  std::string_view              name;
  std::string_view              license;
  std::string_view              commit;
  std::string_view              version;
  std::vector<nsvars>           args;
  std::string_view              package;
  std::vector<std::string>      components;
  std::vector<nsfilecopy>       runtime_install;
  bool                          link_with_ns = false;
};
