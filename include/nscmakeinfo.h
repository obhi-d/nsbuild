#pragma once

#include <nscommon.h>

struct nscmakeinfo
{
  std::string              target_platform;
  std::string              cmake_bin;
  std::string              cmake_toolchain;
  std::string              cmake_config;
  std::string              cmake_generator;
  std::string              cmake_generator_platform;
  std::string              cmake_generator_toolset;
  std::string              cmake_generator_instance;
  std::string              cmake_cppcompiler;
  std::string              cmake_cppcompiler_version;
  std::string              cmake_build_dir; 
  bool                     static_libs  = false;
  bool                     is_multi_cfg = false;
};