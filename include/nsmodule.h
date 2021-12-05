#pragma once
#include <array>
#include <nsbuildcmds.h>
#include <nscommon.h>
#include <nsfetch.h>
#include <nsglob.h>
#include <nsinterface.h>
#include <nsmacros.h>

struct nstarget;
struct nsbuild;
enum class modtype
{
  none,
  data,
  plugin,
  lib,
  exe,
  external,
  ref
};

std::string_view to_string(modtype);
bool             has_data(modtype);
bool             has_sources(modtype);

struct nsmodule
{
  enum
  {
    priv_intf = 0,
    pub_intf  = 1,
  };

  std::vector<nsvars> exports;
  std::vector<nsvars> vars;
  nsmacros            macros;
  nsbuildsteplist     prebuild;
  nsbuildsteplist     postbuild;
  nsbuildcmdlist      install;
  nsglob              glob_sources;
  nsglob              glob_media;

  std::vector<std::string_view> references;

  std::array<nsinterface_list, 2> intf;

  std::unique_ptr<nsfetch> fetch;

  std::string name;
  modtype     type = modtype::none;

  // deferred properties
  std::string_view framework_name;
  std::string_view framework_path;
  std::string_view target_name;
  std::string      source_path;
  std::string      build_path;
  std::string      gen_path;

  bool regenerate = false;

  nsmodule()                    = default;
  nsmodule(nsmodule&&) noexcept = default;
  nsmodule& operator=(nsmodule&&) noexcept = default;
  nsmodule(nsmodule const&)                = delete;
  nsmodule& operator=(nsmodule const&) = delete;

  template <typename Lambda>
  void foreach_dependency(Lambda&& l) const
  {
    for (auto const& i : intf)
    {
      for (auto const& ii : i)
      {
        for (auto sv : ii.dependencies)
          l(sv);
      }
    }
  }
  template <typename Lambda>
  void foreach_references(Lambda&& l) const
  {
    for (auto const& sv : references)
    {
      l(sv);
    }
  }

  void update_properties(nsbuild const& bc, std::string const& name,
                         nstarget& targ);
  void update_macros(nsbuild const& bc, nstarget& targ);

  /// @brief Called in a thread to generate CMakeLists.txt
  /// @param bc Config
  /// @param name Name of this target
  /// @param targ Target object for reference
  void process(nsbuild const& bc, std::string const& name, nstarget& targ);
  void write_variables(std::ofstream&, nsbuild const& bc);
};
