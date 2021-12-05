#pragma once
#include <nscommon.h>
#include <nsparams.h>

struct nsmodule;
struct nsbuild;
struct nsbuildcmds
{
  nsfilters                filters;
  std::vector<std::string> msgs;
  std::string              params;
};

using nsbuildcmdlist = std::vector<nsbuildcmds>;

struct nsbuildstep
{
  std::vector<std::string> artifacts;
  std::vector<std::string> dependencies;
  std::vector<std::string> injected_config;

  nsbuildcmdlist steps;
  bool           check_deps = false;

  void print(std::ostream&, nsbuild const&, nsmodule const&) const;
};

using nsbuildsteplist = std::vector<nsbuildstep>;