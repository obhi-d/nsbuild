#pragma once
#include <nscommon.h>
#include <nsvars.h>

struct nsmodule;
struct nsbuild;
struct nsbuildcmds
{
  std::vector<std::string> msgs;
  std::string              command;
  std::string              params;
};

using nsbuildcmdlist = std::vector<nsbuildcmds>;

struct nsbuildstep
{
  std::vector<std::string> artifacts;
  std::vector<std::string> dependencies;
  std::string              injected_config_body;
  std::string              injected_config_end;

  nsbuildcmdlist      steps;
  std::string         check;
  std::string         wd;
  std::string         name;
  module_filter_flags module_filter = 0;

  void print(std::ostream&, nsbuild const&, nsmodule const&) const;
};

using nsbuildsteplist = std::vector<nsbuildstep>;