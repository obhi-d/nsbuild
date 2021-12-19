#pragma once
#include <array>
#include <nsbuildcmds.h>
#include <nscmake.h>
#include <nscommon.h>
#include <nsfetch.h>
#include <nsglob.h>
#include <nsinterface.h>
#include <nsmacros.h>

struct nstarget;
struct nsbuild;
enum class nsmodule_type
{
  none,
  data,
  plugin,
  lib,
  exe,
  external,
  ref,
  test
};

std::string_view to_string(nsmodule_type);
bool             has_data(nsmodule_type);
bool             has_sources(nsmodule_type);

/// @brief These targets are defined by every module
/// target.prebuild : Optional. Executed before target build
/// target : Actual build target, depends on target.prebuild, and its artifacts
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
  std::uint32_t       framework;

  std::vector<std::string> references;

  std::array<nsinterface_list, 2> intf;

  std::unique_ptr<nsfetch> fetch;

  std::string name;
  nsmodule_type     type = nsmodule_type::none;

  // deferred properties
  std::string_view framework_name;
  std::string_view framework_path;
  std::string      target_name;
  std::string      source_path;

  nsplugin_manifest manifest;

  bool regenerate    = false;
  bool force_rebuild = false;
  bool has_confixx   = false;

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

  /// @brief Called in a thread to generate CMakeLists.txt
  /// @param bc Config
  /// @param name Name of this target
  /// @param targ Target object for reference
  void process(nsbuild const& bc, std::string const& name, nstarget& targ);
  void update_properties(nsbuild const& bc, std::string const& targ_name,
                         nstarget& targ);
  void update_macros(nsbuild const& bc, std::string const& targ_name,
                     nstarget& targ);
  void update_fetch(nsbuild const& bc);
  void generate_plugin_manifest(nsbuild const& bc);

  void write_fetch_build(nsbuild const& bc) const;
  void fetch_content(nsbuild const& bc);

  /// @brief Called to write the cmake file
  /// @param bc config
  void write_main_build(nsbuild const& bc) const;
  void write_variables(std::ofstream&, nsbuild const& bc) const;
  void write_target(std::ofstream&, nsbuild const& bc,
                    std::string const& name) const;

  void write_prebuild_steps(std::ofstream& ofs, nsbuild const& bc) const;
  void write_postbuild_steps(std::ofstream& ofs, nsbuild const& bc) const;
  void write_includes(std::ofstream&, nsbuild const& bc) const;
  void write_include(std::ofstream& ofs, std::string_view path,
                     std::string_view subpath, cmake::inheritance,
                     cmake::exposition = cmake::exposition::build) const;
  void write_refs_includes(std::ofstream& ofs, nsbuild const& bc,
                           nsmodule const& target) const;
  void write_find_package(std::ofstream& ofs, nsbuild const& bc) const;

  void write_definitions(std::ofstream&, nsbuild const& bc) const;
  void write_definitions_mod(std::ofstream&, nsbuild const& bc) const;
  void write_definitions(std::ofstream&, nsbuild const& bc,
                         std::uint32_t type) const;
  void write_definitions(std::ofstream& ,
                     std::string_view def, cmake::inheritance, std::string_view filter) const;
  void write_refs_definitions(std::ofstream& ofs, nsbuild const& bc,
                           nsmodule const& target) const;

  void write_dependencies(std::ofstream& ofs, nsbuild const& bc) const;
  void write_dependencies_begin(std::ofstream& ofs, nsbuild const& bc) const;
  void write_dependencies_mod(std::ofstream& ofs, nsbuild const& bc) const;
  void write_dependencies_end(std::ofstream& ofs, nsbuild const& bc) const;
  void write_dependencies(std::ofstream& ofs, nsbuild const& bc,
                          std::uint32_t intf) const;
  void write_dependency(std::ofstream& ofs, std::string_view target,
                        cmake::inheritance, std::string_view filter) const;
  void write_refs_dependencies(std::ofstream& ofs, nsbuild const& bc,
                               nsmodule const& target) const;

  void write_linklibs(std::ofstream& ofs, nsbuild const& bc) const;
  void write_linklibs_begin(std::ofstream& ofs, nsbuild const& bc) const;
  void write_linklibs_mod(std::ofstream& ofs, nsbuild const& bc) const;
  void write_linklibs_end(std::ofstream& ofs, nsbuild const& bc) const;
  void write_linklibs(std::ofstream& ofs, nsbuild const& bc,
                          std::uint32_t intf) const;
  void write_linklibs(std::ofstream& ofs, std::string_view target,
                        cmake::inheritance, std::string_view filter) const;
  void write_refs_linklibs(std::ofstream& ofs, nsbuild const& bc,
                               nsmodule const& target) const;

  void write_install_command(std::ofstream&, nsbuild const& bc) const;
  void write_final_config(std::ofstream&, nsbuild const& bc) const;
  void write_runtime_settings(std::ofstream&, nsbuild const& bc) const;

  void build_fetched_content(nsbuild const& bc) const;

  std::filesystem::path get_full_bld_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_xpb_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_sbld_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_sdk_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_dl_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_ts_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_ext_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_gen_dir(nsbuild const& bc) const;
};
