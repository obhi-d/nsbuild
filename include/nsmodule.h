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

// std::string_view to_string(nsmodule_type);
bool has_data(nsmodule_type);
bool has_sources(nsmodule_type);

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
  nsglob              glob_media;
  std::string         version;
  std::uint32_t       framework;

  std::vector<std::string> references;

  std::vector<std::string_view> unset;

  std::array<nsinterface_list, 2> intf;

  std::unique_ptr<nsfetch> fetch;

  std::string   name;
  nsmodule_type type = nsmodule_type::none;
  
  // deferred properties
  std::string_view framework_name;
  std::string_view framework_path;
  std::string      target_name;
  std::string      source_path;
  std::string      gen_path;

  nsplugin_manifest manifest;

  bool regenerate    = false;
  bool force_rebuild = false;
  bool has_confixx   = false;
  bool disabled      = false;
  
  // .. Options
  bool console_app              = false;

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
  void update_properties(nsbuild const& bc, std::string const& targ_name, nstarget& targ);
  void update_macros(nsbuild const& bc, std::string const& targ_name, nstarget& targ);
  void update_fetch(nsbuild const& bc);
  void generate_plugin_manifest(nsbuild const& bc);

  std::string write_fetch_build(nsbuild const& bc) const;
  void        fetch_content(nsbuild const& bc);
  bool        fetch_changed(nsbuild const& bc, std::string const& last_sha) const;
  void        write_fetch_meta(nsbuild const& bc, std::string const& last_sha) const;
  /// @brief Called to write the cmake file
  /// @param bc config
  void write_main_build(std::ostream&, nsbuild const& bc) const;
  void write_variables(std::ostream&, nsbuild const& bc, char sep = ';') const;
  void write_sources(std::ostream&, nsbuild const& bc) const;
  void write_source_subpath(nsglob& glob, nsbuild const& bc) const;
  void write_target(std::ostream&, nsbuild const& bc, std::string const& name) const;
  void write_enums_init(std::ostream&, nsbuild const& bc) const;
  
  void write_prebuild_steps(std::ostream& ofs, nsbuild const& bc) const;
  void write_postbuild_steps(std::ostream& ofs, nsbuild const& bc) const;

  void write_cxx_options(std::ostream&, nsbuild const& bc) const;

  void write_includes(std::ostream&, nsbuild const& bc) const;
  void write_include(std::ostream& ofs, std::string_view path, std::string_view subpath, cmake::inheritance) const;
  void write_refs_includes(std::ostream& ofs, nsbuild const& bc, nsmodule const& target) const;
  void write_find_package(std::ostream& ofs, nsbuild const& bc) const;

  void write_definitions(std::ostream&, nsbuild const& bc) const;
  void write_definitions_mod(std::ostream&, nsbuild const& bc) const;
  void write_definitions(std::ostream&, nsbuild const& bc, std::uint32_t type) const;
  void write_definitions(std::ostream&, std::string_view def, cmake::inheritance, std::string_view filter) const;
  void write_refs_definitions(std::ostream& ofs, nsbuild const& bc, nsmodule const& target) const;

  void write_dependencies(std::ostream& ofs, nsbuild const& bc) const;
  void write_dependencies_begin(std::ostream& ofs, nsbuild const& bc) const;
  void write_dependencies_mod(std::ostream& ofs, nsbuild const& bc) const;
  void write_dependencies_end(std::ostream& ofs, nsbuild const& bc) const;
  void write_dependencies(std::ostream& ofs, nsbuild const& bc, std::uint32_t intf) const;
  void write_dependency(std::ostream& ofs, std::string_view target, cmake::inheritance, std::string_view filter) const;
  void write_target_link_libs(std::ostream& ofs, std::string_view target, cmake::inheritance,
                              std::string_view filter) const;
  void write_refs_dependencies(std::ostream& ofs, nsbuild const& bc, nsmodule const& target) const;

  void write_linklibs(std::ostream& ofs, nsbuild const& bc) const;
  void write_linklibs_begin(std::ostream& ofs, nsbuild const& bc) const;
  void write_linklibs_mod(std::ostream& ofs, nsbuild const& bc) const;
  void write_linklibs_end(std::ostream& ofs, nsbuild const& bc) const;
  void write_linklibs(std::ostream& ofs, nsbuild const& bc, std::uint32_t intf) const;
  void write_linklibs(std::ostream& ofs, std::string_view target, cmake::inheritance, std::string_view filter) const;
  void write_refs_linklibs(std::ostream& ofs, nsbuild const& bc, nsmodule const& target) const;

  void write_install_command(std::ostream&, nsbuild const& bc) const;
  void write_final_config(std::ostream&, nsbuild const& bc) const;
  void write_runtime_settings(std::ostream&, nsbuild const& bc) const;

  void build_fetched_content(nsbuild const& bc) const;

  std::filesystem::path get_full_bld_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_fetch_bld_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_ext_bld_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_fetch_sbld_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_sdk_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_dl_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_ext_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_gen_dir(nsbuild const& bc) const;
};
