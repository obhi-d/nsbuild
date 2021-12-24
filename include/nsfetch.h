#pragma once
#include <nsbuildcmds.h>
#include <nscommon.h>
#include <nsvars.h>

struct nsfetch
{
  neo::text_content             custom_build;
  neo::text_content             post_build_install;
  std::string_view              repo;
  std::string_view              license;
  std::string_view              commit;
  std::string_view              version;
  std::vector<nsvars>           args;
  std::string_view              package;
  std::vector<std::string>      components;
  std::vector<nsfilecopy>       runtime_install;
  bool                          legacy_linking = false;
};
