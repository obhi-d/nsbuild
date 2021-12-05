#pragma once
#include <list>
#include <nscommon.h>
#include <nscompiler.h>
#include <nsconfig.h>
#include <nsframework.h>
#include <nsmodule.h>
#include <nstarget.h>
#include <nspython.h>
#include <future>
#include <nsmacros.h>

struct nsbuild : public neo::command_handler
{
  std::string_view build_dir      = "../out/bld";
  std::string_view sdk_dir        = "../out/sdk";
  std::string_view runtime_dir    = "../out/rt";
  std::string_view download_dir   = "../out/dl";
  std::string_view scan_path      = ".";
  std::string_view frameworks_dir = "Frameworks";

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

  void  scan_main(std::string_view pwd);
  bool  scan_file(std::string* sha = nullptr);
  int   generate_cl();
  void  handle_error(neo::state_machine&);
  void  read_timestamps();
  void  update_macros();
  void  process_targets();
  void  process_target(std::string const&, nstarget&);
  void  process_main();


  /// @brief Generates enum files
  /// @param target Should be FwName/ModName or full path to module directory
  void  generate_enum(std::string target);
  modid get_modid(std::string_view from);

  void  add_framework(std::string_view name) 
  {
    frameworks.emplace_back();
    s_nsframework = &frameworks.back();
    s_nsframework->name = name;
  }

  void add_module(std::string_view name)
  {
    s_nsframework->modules.emplace_back();
    s_nsmodule          = &s_nsframework->modules.back();
    s_nsmodule->name = name;
  }

  template <typename L>
  void for_each_framework(L&&);
  template <typename L>
  void for_each_module(L&&);
};
