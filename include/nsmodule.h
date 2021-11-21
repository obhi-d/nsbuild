#pragma once
#include <nsbuildcmds.h>
#include <nscommon.h>
#include <nsfetch.h>
#include <nsinterface.h>

struct nsmodule
{
  std::vector<nsvars> exports;
  std::vector<nsvars> vars;
  nsbuildsteplist     prebuild;
  nsbuildsteplist     postbuild;
  nsbuildcmdlist      install;

  nsinterface_list intf;
  nsinterface_list priv;

  std::vector<std::string> dependencies;

  std::unique_ptr<nsfetch> fetch;

  std::string name;

  nsmodule()     = default;
  nsmodule(nsmodule&&)        = default;
  nsmodule& operator=(nsmodule&&)      = default;
  nsmodule(nsmodule const&) = delete;
  nsmodule& operator=(nsmodule const&) = delete;
};
