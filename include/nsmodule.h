#pragma once
#include <array>
#include <nsbuildcmds.h>
#include <nscmake.h>
#include <nscommon.h>
#include <nsfetch.h>
#include <nsglob.h>
#include <nsinstallers.h>
#include <nsinterface.h>
#include <nsmacros.h>

struct nstarget;
struct nsbuild;

// std::string_view to_string(nsmodule_type);
bool has_data(nsmodule_type);
bool has_sources(nsmodule_type);

using name_value_pair = std::pair<std::string, std::string>;
// @brief Test configs
struct nstest
{
  std::string                  name;
  std::string                  test_param_name;
  std::string                  tags;
  std::vector<name_value_pair> parameters;
};

struct nscontent
{
  std::string name;
  std::string content;
};

struct ns_embed_content
{
  std::string hpp;
  std::string cpp;

  void emplace_back(std::string_view name, std::string_view value, std::string content);
};

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

  std::vector<nsvars>    exports;
  std::vector<nsvars>    vars;
  std::vector<nscontent> contents;
  nsmacros               macros;
  nsbuildsteplist        prebuild;
  nsbuildsteplist        postbuild;
  nsbuildcmdlist         install;
  nsglob                 glob_media;
  nsglob                 glob_sources;
  std::string            version;
  std::uint32_t          framework;

  ns_embed_content         embedded_binary_files;
  ns_embed_content         embedded_base64_files;
  std::vector<std::string> references;
  std::vector<std::string> required_plugins;

  std::vector<std::string>      source_sub_dirs;
  std::vector<std::string>      source_files;
  std::vector<std::string_view> unset;

  std::vector<nstest> tests;

  std::array<nsinterface_list, 2> intf;

  std::vector<nsfetch> fetch;

  std::string tags;
  std::string org_name;
  std::string name;
  std::string custom_target_name;

  std::filesystem::path location;

  nsmodule_type type = nsmodule_type::none;

  // deferred properties
  std::string framework_name;
  std::string framework_path;
  std::string target_name;
  std::string source_path;
  std::string gen_path;
  std::string sha;

  bool regenerate   = false;
  bool force_build  = false;
  bool has_confixx  = false;
  bool disabled     = false;
  bool direct_build = false;
  bool deleted      = false;
  bool sha_written  = false;

  // .. Options
  bool console_app         = false;
  bool was_fetch_rebuilt   = false;
  bool has_globs_changed   = false;
  bool has_headers_changed = false;

  nsmodule()                               = default;
  nsmodule(nsmodule&&) noexcept            = default;
  nsmodule& operator=(nsmodule&&) noexcept = default;
  nsmodule(nsmodule const&)                = delete;
  nsmodule& operator=(nsmodule const&)     = delete;

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
    for (auto const& i : required_plugins)
      l(i);
  }
  template <typename Lambda>
  void foreach_references(Lambda&& l) const
  {
    for (auto const& sv : references)
    {
      l(sv);
    }
  }

  inline void set_sha(std::string s) { sha = std::move(s); }

  inline void should_regenerate() { regenerate = true; }

  struct content
  {
    std::string data;
    std::string sha;
  };

  nsfetch* find_fetch(std::string_view name);

  /// @brief Called in a thread to generate CMakeLists.txt
  /// @param bc Config
  /// @param name Name of this target
  /// @param targ Target object for reference
  void process(nsbuild const& bc, nsinstallers& installer, std::string const& name, nstarget& targ);
  void update_properties(nsbuild const& bc, std::string const& targ_name, nstarget& targ);
  void update_macros(nsbuild const& bc, std::string const& targ_name, nstarget& targ);
  void update_fetch(nsbuild const& bc, nsinstallers& installer);
  void gather_sources(nsglob& glob, nsbuild const& bc) const;
  void gather_headers(nsglob& glob, nsbuild const& bc) const;
  void check_enums(nsbuild const& bc) const;
  void check_embeds(nsbuild const& bc) const;
  void write_sha(nsbuild const& bc);

  content make_fetch_build_content(nsbuild const& bc, nsfetch const& ft) const;
  void    write_fetch_build_content(nsbuild const& bc, nsfetch const& ft, content const&) const;
  void    fetch_content(nsbuild const& bc, nsinstallers& installer, nsfetch& ft);
  bool    fetch_changed(nsbuild const& bc, nsfetch const& ft, std::string const& last_sha) const;
  void    write_fetch_meta(nsbuild const& bc, nsfetch const& ft, std::string const& last_sha) const;
  /// @brief Called to write the cmake file
  /// @param bc config
  void write_main_build(std::ostream&, nsbuild const& bc) const;
  void write_variables(std::ostream&, nsbuild const& bc, char sep = ';') const;
  void write_sources(std::ostream&, nsbuild const& bc) const;
  void write_target(std::ostream&, nsbuild const& bc, std::string const& name) const;

  void write_prebuild_steps(std::ostream& ofs, nsbuild const& bc) const;
  void write_postbuild_steps(std::ostream& ofs, nsbuild const& bc) const;
  int  begin_prebuild_steps(std::ostream& ofs, nsbuildsteplist const& list, nsbuild const& bc) const;
  int  end_prebuild_steps(std::ostream& ofs, nsbuildsteplist const& list, nsbuild const& bc) const;
  int  write_postbuild_steps(std::ostream& ofs, nsbuildsteplist const& list, nsbuild const& bc) const;

  void write_cxx_options(std::ostream&, nsbuild const& bc) const;

  void write_includes(std::ostream&, nsbuild const& bc) const;
  void write_include(std::ostream& ofs, /* nsglob& glob, */ std::string_view path, std::string_view subpath,
                     cmake::inheritance) const;
  void write_refs_includes(std::ostream& ofs, /* nsglob& glob,*/ nsbuild const& bc, nsmodule const& target) const;
  void write_find_package(std::ostream& ofs, nsbuild const& bc) const;

  void write_definitions(std::ostream&, nsbuild const& bc) const;
  void write_definitions_itf(std::ostream&, nsbuild const& bc) const;
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
  void write_plugin_dependencies(std::ostream& ofs, nsbuild const& bc) const;

  void write_linklibs(std::ostream& ofs, nsbuild const& bc) const;
  void write_linklibs_begin(std::ostream& ofs, nsbuild const& bc) const;
  void write_linklibs_mod(std::ostream& ofs, nsbuild const& bc) const;
  void write_linklibs_end(std::ostream& ofs, nsbuild const& bc) const;
  void write_linklibs(std::ostream& ofs, nsbuild const& bc, std::uint32_t intf) const;
  void write_linklibs(std::ostream& ofs, std::string_view target, cmake::inheritance, std::string_view filter) const;
  void write_refs_linklibs(std::ostream& ofs, nsbuild const& bc, nsmodule const& target) const;

  void write_install_command(std::ostream&, nsbuild const& bc) const;
  void write_final_config(std::ostream&, nsbuild const& bc) const;
  void write_tests(std::ostream&, nsbuild const& bc) const;
  void write_runtime_settings(std::ostream&, nsbuild const& bc) const;

  void build_fetched_content(nsbuild const& bc, nsinstallers& installer, nsfetch const& fetch);
  void delete_build(nsbuild const& bc);
  bool download(nsbuild const& bc, nsfetch& ft);

  bool sha_changed(nsbuild const& bc, std::string_view name, std::string_view sha) const;
  void write_sha_changed(nsbuild const& bc, std::string_view name, std::string_view sha) const;

  bool restore_fetch_lists(nsbuild const& bc, nsfetch const& ft) const;
  void backup_fetch_lists(nsbuild const& bc, nsfetch const& ft) const;

  std::filesystem::path get_full_bld_dir(nsbuild const& bc) const;
  std::filesystem::path get_fetch_bld_dir(nsbuild const& bc, nsfetch const& nfc) const;
  std::filesystem::path get_fetch_src_dir(nsbuild const& bc, nsfetch const& nfc) const;
  std::filesystem::path get_full_sdk_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_dl_dir(nsbuild const& bc, nsfetch const& nfc) const;
  std::filesystem::path get_full_gen_dir(nsbuild const& bc) const;
  std::filesystem::path get_full_fetch_file(nsbuild const& bc, nsfetch const& nfc) const;
};
