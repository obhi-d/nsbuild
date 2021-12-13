#pragma once

#include <nscommon.h>

struct nsconfig
{
  bool is_accepted(std::string_view v)
  {
    for (auto const& e : accepted)
      if (e == v)
        return true;
    return false;
  }

  std::vector<std::string> accepted;
  std::string              target_platform;
  std::string              build_type; // ${config_build_type}
  std::string              cmake_bin;
  std::string              cmake_toolchain;
  std::string              cmake_config;
  std::string              cmake_generator;
  std::string              cmake_generator_platform;
  std::string              cmake_generator_toolset;
  std::string              cmake_generator_instance;
  bool                     static_libs  = false;
  bool                     is_multi_cfg = false;
};