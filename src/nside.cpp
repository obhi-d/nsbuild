#include <nlohmann/json.hpp>
#include <fstream>
#include "nside.h"
#include "nsbuild.h"

void nside::write(ide_type ide, nsbuild const& bc) 
{
  switch (ide)
  {
  case ide_type::all:
    nside_vs::write(bc);
    nside_vscode::write(bc);
    nside_clion::write(bc);
    break;
  case ide_type::vs:
    nside_vs::write(bc);
    break;
  case ide_type::vscode:
    nside_vscode::write(bc);
    break;
  case ide_type::clion:
    nside_clion::write(bc);
    break;
  }
}

void nside_vs::write(nsbuild const& bc)
{
  using json = nlohmann::json;
  json j;
  // j["configurations"]
  auto& configurations = j["configurations"];
  for (auto const& cxx : bc.config)
  {
    // only write msvc
    if (cxx.filters.test((std::size_t)nsfilter::msvc))
    {
      json cfg;
      cfg["name"]      = cxx.name;
      cfg["generator"] = "Ninja";
      if (cxx.filters.test((std::size_t)nsfilter::debug))
        cfg["configurationType"] = "Debug";
      else if (cxx.filters.test((std::size_t)nsfilter::release))
        cfg["configurationType"] = "RelWithDebInfo";
      cfg["inheritEnvironments"].push_back("msvc_x64_x64");
      std::filesystem::path projectDir = "${projectDir}";
      cfg["buildRoot"] =
          (projectDir / bc.out_dir / "${name}" / "cc").generic_string();
      cfg["installRoot"] =
          (projectDir / bc.out_dir / "${name}" / bc.sdk_dir).generic_string();
      configurations.emplace_back(std::move(cfg));
    }
  }

  std::ofstream ff{bc.get_full_scan_dir() / "CMakeSettings.json"};
  ff << std::setw(4) << j << std::endl;
}

void nside_vscode::write(nsbuild const&) {}

void nside_clion::write(nsbuild const&) {}
