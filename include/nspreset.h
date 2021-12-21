#pragma once
#include "nscommon.h"
#include <cstdint>

struct nspreset
{
  static inline constexpr std::uint32_t use_cmake_config = 1;
  static inline constexpr std::uint32_t write_compiler_paths = 3;

  std::string              name;
  std::string              display_name;
  std::string              desc;
  std::string              architecture;
  nameval_list             definitions;

  struct cfg
  {
    nsfilters                filters;
    std::vector<std::string> compiler_flags;
    std::vector<std::string> linker_flags;
  };

  std::vector<cfg> configs;

  static void write(std::ostream&, std::uint32_t options, std::string_view bin_dir, nameval_list const& extras, nsbuild const&);
};
