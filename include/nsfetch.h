#pragma once
#include <nsbuildcmds.h>
#include <nscommon.h>
#include <nsvars.h>

struct nsfetch
{
  std::string              name;
  std::string_view         filters;
  neo::text_content        finalize;
  neo::text_content        prepare;
  neo::text_content        include;
  std::string_view         repo;
  std::string_view         license;
  std::string_view         tag;
  std::string_view         source;
  std::string              version;
  std::vector<nsvars>      args;
  std::string_view         package;
  std::string_view         extern_name;
  std::string_view         namespace_name;
  std::vector<std::string> components;
  std::vector<std::string> targets;
  std::vector<nsfilecopy>  runtime_install;
  std::vector<std::string> runtime_loc;
  std::vector<std::string> runtime_files;
  bool                     legacy_linking = false;
  bool                     skip_namespace = false;
  bool                     force_build    = false;
  bool                     force_download = false;
  bool                     regenerate     = false;
  bool                     disabled       = false;
};
