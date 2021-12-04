#pragma once
#include <filesystem>
#include <nscommon.h>

struct nstarget
{
  std::string   sha256;
  std::uint32_t fw_idx  = 0;
  std::uint32_t mod_idx = 0;
  // Processed tracker
  bool          processed = false;
};
