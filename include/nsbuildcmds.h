#pragma once
#include <nscommon.h>
#include <nsparams.h>

struct nsbuildcmds
{  
  nsfilters                     filters;
  std::vector<std::string>      msgs;
  std::vector<nsparams>         params;
};

using nsbuildcmdlist = std::vector<nsbuildcmds>;

struct nsbuildstep
{
  std::vector<std::string_view> artifacts;
  std::vector<std::string_view> dependencies;
  nsbuildcmdlist                steps;
};

using nsbuildsteplist = std::vector<nsbuildstep>;