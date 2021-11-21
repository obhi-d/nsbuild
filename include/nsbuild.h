#pragma once
#include <list>
#include <nscommon.h>
#include <nscompiler.h>
#include <nsconfig.h>
#include <nsframework.h>
#include <nsmodule.h>
#include <nstarget.h>

struct nsbuild : public neo::command_handler
{
  std::string_view build_dir      = "../build";
  std::string_view install_dir    = "../install";
  std::string_view frameworks_dir = "Frameworks";
  std::string_view output_dir     = "../output";

  std::string_view targ_os;

  nsconfig config;

  std::unordered_map<std::string, nsparams> subs;
  std::vector<nsframework>                  frameworks;

  std::vector<nscompiler> compiler;

  neo::registry          reg;
  std::list<std::string> contents;

  std::unordered_map<std::string, std::string> timestamps;
  std::unordered_map<std::string, nstarget>    targets;
  
  //--------------------------------------
  // State
  std::filesystem::path pwd;

  nsmodule*    s_nsmodule    = nullptr;
  nsbuildcmds* s_nsbuildcmds = nullptr;
  nsbuildstep* s_nsbuildstep = nullptr;
  nsfetch*     s_nsfetch     = nullptr;
  nsinterface* s_nsinterface = nullptr;
  nsvars*      s_nsvar       = nullptr;

  //--------------------------------------
  // Fn
  nsbuild()          = default;
  nsbuild(nsbuild&&) = default;
  nsbuild& operator=(nsbuild&&) = default;
  nsbuild(nsbuild const&)       = delete;
  nsbuild& operator=(nsbuild const&) = delete;


  void     scan_main(std::string_view pwd);
  bool     scan_file(std::string* sha = nullptr);
  int      do_timestamp_checks();
  void     handle_error(neo::state_machine&);
  void     read_timestamps();
  void     generate();
  void     read_all();
  void     generate_enum();

  template <typename L>
  void for_each_framework(L&&);
  template <typename L>
  void for_each_module(L&&);
};
