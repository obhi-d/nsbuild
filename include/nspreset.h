#pragma once
#include "nsbuildcmds.h"
#include "nscommon.h"

#include <cstdint>

struct nspreset
{
  static inline constexpr std::uint32_t use_cmake_config     = 1;
  static inline constexpr std::uint32_t write_compiler_paths = 3;

  std::string  name;
  std::string  build_type;
  std::string  display_name;
  std::string  naming;
  std::string  desc;
  std::string  architecture;
  std::string  tags;
  std::string  platform;
  nameval_list definitions;

  nsbuildsteplist prebuild;
  nsbuildsteplist postbuild;

  std::vector<std::string> allowed_filters;
  std::vector<std::string> disallowed_filters;

  struct cfg
  {
    nsfilters                filters;
    std::vector<std::string> compiler_flags;
    std::vector<std::string> linker_flags;
  };

  std::vector<cfg> configs;

  bool static_plugins = false;
  bool static_libs    = false;
  bool cppcheck       = false;
  bool unity_build    = false;
  bool glob_sources   = false;

  std::string cppcheck_suppression;

  static void write(std::ostream&, std::uint32_t options, nameval_list const& extras, nsbuild const&);
  // write current preset
  static void write(std::ostream&, std::uint32_t options, std::string_view bin_dir, nameval_list const& extras,
                    nsbuild const&);
};
