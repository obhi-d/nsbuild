#include <nlohmann/json.hpp>
#include <fstream>
#include "nspreset.h"
#include "nsbuild.h"

void nspreset::write(std::ostream& ff, std::uint32_t options, std::string_view bin_dir, nameval_list const& extras,
                     nsbuild const& bc)
{
  using json = nlohmann::json;
  json j;
  j["version"] = 3;
  j["cmakeMinimumRequired"] = {{"major", 3}, {"minor", 20}, {"patch", 0}};

  // j["configurations"]
  auto& configurations = j["configurePresets"];
  for (auto const& cxx : bc.presets)
  {
    // only write msvc
    json cfg;
    cfg["name"]           = cxx.name;
    cfg["displayName"]    = cxx.display_name;
    cfg["description"]    = cxx.desc;
    cfg["generator"]      = (options & use_cmake_config) && (!bc.cmakeinfo.cmake_generator.empty())
                                ? bc.cmakeinfo.cmake_generator
                                : bc.preferred_gen;
    cfg["binaryDir"] = bin_dir.empty() ? std::string{"${sourceDir}/"} + bc.out_dir + "/" + cxx.name + "/cc" : bin_dir;
    if (!cxx.architecture.empty())
      cfg["architecture"] = {{"value", cxx.architecture}, {"strategy", "external"}};

    auto& cacheVars = cfg["cacheVariables"];
    if (options & write_compiler_paths)
    {
      if (!bc.cmakeinfo.cmake_config.empty() && !bc.cmakeinfo.cmake_is_multi_cfg)
        cacheVars["CMAKE_BUILD_TYPE"] = bc.cmakeinfo.cmake_config;

      if (!bc.cmakeinfo.cmake_toolchain.empty())
      //  cacheVars["CMAKE_TOOLCHAIN_FILE"] = bc.cmakeinfo.cmake_toolchain;
        cfg["toolchainFile"] = bc.cmakeinfo.cmake_toolchain;

      if (!bc.cmakeinfo.cmake_generator_platform.empty())
        cacheVars["CMAKE_GENERATOR_PLATFORM"] = bc.cmakeinfo.cmake_generator_platform;

      if (!bc.cmakeinfo.cmake_generator_instance.empty())
        cacheVars["CMAKE_GENERATOR_INSTANCE"] = bc.cmakeinfo.cmake_generator_instance;

      if (!bc.cmakeinfo.cmake_generator_toolset.empty())
        cfg["toolset"] = {{"value", bc.cmakeinfo.cmake_generator_toolset}, {"strategy", "external"}};

      if (!bc.cmakeinfo.cmake_cppcompiler.empty())
        cacheVars["CMAKE_CXX_COMPILER"] = bc.cmakeinfo.cmake_cppcompiler;

      if (!bc.cmakeinfo.cmake_ccompiler.empty())
        cacheVars["CMAKE_C_COMPILER"] = bc.cmakeinfo.cmake_ccompiler;
    }
      
    cacheVars["__nsbuild_preset"] = cxx.name;
    for (auto const& d : cxx.definitions)
      cacheVars[d.first] = d.second;
    for (auto const& d : extras)
      cacheVars[d.first] = d.second;

    cfg["installDir"] = std::string{"${sourceDir}/"} + bc.out_dir + "/" + cxx.name + "/" + bc.sdk_dir;
    configurations.emplace_back(std::move(cfg));
  }
  
  auto& builds = j["buildPresets"];
  for (auto const& cxx : bc.presets)
  {
    json bld;
    bld["name"]        = cxx.name;
    bld["displayName"]     = cxx.display_name;
    bld["configurePreset"] = cxx.name;
    builds.emplace_back(std::move(bld));
  }
    
  ff << std::setw(2) << j << std::endl;
}

