#pragma once
#include <future>
#include <list>
#include <nscommon.h>
#include <nscompiler.h>
#include <nsconfig.h>
#include <nsframework.h>
#include <nsmacros.h>
#include <nsmodule.h>
#include <nspython.h>
#include <nstarget.h>

struct nsbuild : public neo::command_handler
{
  std::string out_dir        = "../out";
  std::string build_dir      = "../out/${config_build_type}";
  std::string sdk_dir        = "${config_build_dir}/sdk";
  std::string runtime_dir    = "${config_build_dir}/rt";
  std::string download_dir   = "../out/dl";
  std::string scan_path      = ".";
  std::string frameworks_dir = "Frameworks";
  std::string type           = "";

  nsconfig config;

  nsmacros macros;

  std::vector<nsframework> frameworks;

  std::vector<nscompiler> compiler;

  neo::registry          reg;
  std::list<std::string> contents;

  std::unordered_map<std::string, std::string> timestamps;
  std::unordered_map<std::string, nstarget>    targets;
  std::vector<std::string>                     sorted_targets;
  std::vector<std::future<void>>               process;
  nspython                                     python;
  std::filesystem::path                        wd;

  //--------------------------------------
  // State
  std::filesystem::path pwd;

  nsframework* s_nsframework = nullptr;
  nsmodule*    s_nsmodule    = nullptr;
  nsbuildcmds* s_nsbuildcmds = nullptr;
  nsbuildstep* s_nsbuildstep = nullptr;
  nsfetch*     s_nsfetch     = nullptr;
  nsinterface* s_nsinterface = nullptr;
  nsvars*      s_nsvar       = nullptr;
  //--------------------------------------
  // Runtime options
  bool stop_after_modtype = false;
  bool regenerate_main    = false;

  //--------------------------------------
  // Fn
  nsbuild()          = default;
  nsbuild(nsbuild&&) = default;
  nsbuild& operator=(nsbuild&&) = default;
  nsbuild(nsbuild const&)       = delete;
  nsbuild& operator=(nsbuild const&) = delete;

  void scan_main(std::string_view pwd);
  bool scan_file(std::string* sha = nullptr);
  int  generate_cl();
  void handle_error(neo::state_machine&);
  void read_timestamps();
  void update_macros();
  void process_targets();
  void process_target(std::string const&, nstarget&);
  /// @brief This initiates the main build: check mode
  /// - Checks current build directory, if it does not exist creates it
  /// - If not present, writes basic config info in build dir
  /// - Generates external build and builds and installs exteranl libs
  void before_all(std::string_view build_dir, std::string_view src_dir);
  /// @brief Generates enum files
  /// @param target Should be FwName/ModName or full path to module directory
  void generate_enum(std::string target);

  modid get_modid(std::string_view from);

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
  void for_each_framework(L&&);
  template <typename L>
  void for_each_module(L&&);
};
