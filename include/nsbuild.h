#pragma once
#include <future>
#include <list>
#include <nscmakeinfo.h>
#include <nscommon.h>
#include <nsframework.h>
#include <nsmacros.h>
#include <nsmodule.h>
#include <nspreset.h>
#include <nspython.h>
#include <nstarget.h>

struct nsbuild : public neo::command_handler
{
  // preferred generator
  std::string preferred_gen = "Ninja";
  // path relative to scan
  std::string out_dir = "../out";
  // path relative to out/presets
  std::string cmake_gen_dir = "cmake";
  // path relative to out/presets
  std::string sdk_dir = "sdk";
  // path relative to out/presets
  std::string runtime_dir = "rt";
  // path relative to out/presets
  std::string cache_dir = "ts";
  // path relative to out
  std::string download_dir = "dl";
  // path relative to out/presets
  std::string build_dir = "bld";
  // path relative to current
  std::string scan_dir = ".";
  // path relative to scan
  std::string frameworks_dir = "Frameworks";
  // path relative to runtime
  std::string plugin_rel_dir = "Media/Plugins";

  std::string ignore_media_from = "Internal";

  std::string test_ref = "";

  nscmakeinfo cmakeinfo;

  nsmacros macros;

  std::vector<nsframework> frameworks;

  std::vector<nspreset> presets;

  neo::registry          reg;
  std::list<std::string> contents;

  std::unordered_map<std::string, nstarget>            targets;
  std::vector<std::string>                             sorted_targets;
  std::vector<std::pair<std::future<void>, nsmodule const*>> process;

  // working dir
  std::filesystem::path wd;

  //--------------------------------------
  // State

  nsframework* s_nsframework = nullptr;
  nsmodule*    s_nsmodule    = nullptr;
  nsbuildcmds* s_nsbuildcmds = nullptr;
  nsbuildstep* s_nsbuildstep = nullptr;
  nsfetch*     s_nsfetch     = nullptr;
  nsinterface* s_nsinterface = nullptr;
  nsvars*      s_nsvar       = nullptr;
  nspreset*    s_nspreset    = nullptr;
  //--------------------------------------
  // Runtime options
  nsmetainfo  meta;
  nsmetastate state;

  bool static_libs = false;

  //--------------------------------------
  // Fn
  nsbuild();
  nsbuild(nsbuild&&) = default;
  nsbuild& operator=(nsbuild&&) = default;
  nsbuild(nsbuild const&)       = delete;
  nsbuild& operator=(nsbuild const&) = delete;

  void main_project(std::string_view proj);

  bool scan_file(std::filesystem::path, bool store, std::string* sha = nullptr);
  void handle_error(neo::state_machine&);
  void update_macros();
  void process_targets();
  void process_target(std::string const&, nstarget&);

  /// @brief This initiates the main build: check mode
  /// - Checks current build directory, if it does not exist creates it
  /// - If not present, writes basic presets info in build dir
  /// - Generates external build and builds and installs exteranl libs
  void before_all();
  void read_meta(std::filesystem::path);
  void act_meta();
  void write_meta(std::filesystem::path);
  void scan_main(std::filesystem::path);
  void read_framework(std::filesystem::path);
  void read_module(std::filesystem::path);
  void write_include_modules() const;
  void check_modules();

  void write_cxx_options(std::ostream&) const;

  /// @brief Generates enum files
  /// @param target Should be FwName/ModName or full path to module directory
  void generate_enum(std::string target);

  /// @brief Copy media directories and files, ignores media/Internal directory
  /// @param from
  /// @param to
  static void copy_media(std::filesystem::path from, std::filesystem::path to, std::string ignore);

  modid get_modid(std::string_view from) const;

  void add_framework(std::string_view name)
  {
    frameworks.emplace_back();
    s_nsframework       = &frameworks.back();
    s_nsframework->name = name;
  }

  void add_module(std::string_view name)
  {
    s_nsframework->modules.emplace_back();
    s_nsmodule       = &s_nsframework->modules.back();
    s_nsmodule->name = name;
  }

  nsmodule const& get_module(std::string const& targ) const
  {
    auto it = targets.find(targ);
    if (it == targets.end())
      throw std::runtime_error("Invalid module name!");
    return frameworks[it->second.fw_idx].modules[it->second.mod_idx];
  }

  template <typename L>
  inline void foreach_module(L&& l)
  {
    for (auto& f : frameworks)
    {
      for (auto& m : f.modules)
        l(m);
    }
  }

  template <typename L>
  void foreach_framework(L&&);
  template <typename L>
  void foreach_module(L&&, std::filesystem::path fwpath);

  struct fullpaths
  {
    std::filesystem::path scan_dir;
    std::filesystem::path cfg_dir;
    std::filesystem::path cmake_gen_dir;
    std::filesystem::path build_dir;
    std::filesystem::path cache_dir;
    std::filesystem::path out_dir;
    std::filesystem::path dl_dir;
  };

  fullpaths paths;

  void compute_paths();

  inline std::filesystem::path const& get_full_scan_dir() const { return paths.scan_dir; }
  inline std::filesystem::path const& get_full_cfg_dir() const { return paths.cfg_dir; }
  inline std::filesystem::path const& get_full_cmake_gen_dir() const { return paths.cmake_gen_dir; }
  inline std::filesystem::path const& get_full_build_dir() const { return paths.build_dir; }
  inline std::filesystem::path const& get_full_cache_dir() const { return paths.cache_dir; }
  inline std::filesystem::path const& get_full_out_dir() const { return paths.out_dir; }
  inline std::filesystem::path const& get_full_dl_dir() const { return paths.dl_dir; }
};
