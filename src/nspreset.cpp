#include "nspreset.h"

#include "nsbuild.h"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static void write_preset(nspreset const& cxx, std::uint32_t options, json& cache_vars, json& cfg, nsbuild const& bc)
{
  cfg["name"]        = cxx.name;
  cfg["displayName"] = cxx.display_name;
  cfg["description"] = cxx.desc;
  cfg["generator"]   = (options & nspreset::use_cmake_config) && (!bc.cmakeinfo.cmake_generator.empty())
                           ? bc.cmakeinfo.cmake_generator
                           : bc.preferred_gen;
  if (cxx.build_type == "debug")
    cache_vars["CMAKE_BUILD_TYPE"] = "Debug";
  else
    cache_vars["CMAKE_BUILD_TYPE"] = "RelWithDebInfo";
  if (options & nspreset::write_compiler_paths)
  {
    if (!bc.cmakeinfo.cmake_config.empty() && !bc.cmakeinfo.cmake_is_multi_cfg)
      cache_vars["CMAKE_BUILD_TYPE"] = bc.cmakeinfo.cmake_config;

    if (!bc.cmakeinfo.cmake_toolchain.empty())
      //  cache_vars["CMAKE_TOOLCHAIN_FILE"] = bc.cmakeinfo.cmake_toolchain;
      cfg["toolchainFile"] = bc.cmakeinfo.cmake_toolchain;

    if (!bc.cmakeinfo.cmake_generator_platform.empty())
      cache_vars["CMAKE_GENERATOR_PLATFORM"] = bc.cmakeinfo.cmake_generator_platform;

    if (!bc.cmakeinfo.cmake_generator_instance.empty())
      cache_vars["CMAKE_GENERATOR_INSTANCE"] = bc.cmakeinfo.cmake_generator_instance;

    if (!bc.cmakeinfo.cmake_generator_toolset.empty())
      cfg["toolset"] = {{"value", bc.cmakeinfo.cmake_generator_toolset}, {"strategy", "external"}};

    if (!bc.cmakeinfo.cmake_cppcompiler.empty())
      cache_vars["CMAKE_CXX_COMPILER"] = bc.cmakeinfo.cmake_cppcompiler;

    if (!bc.cmakeinfo.cmake_ccompiler.empty())
      cache_vars["CMAKE_C_COMPILER"] = bc.cmakeinfo.cmake_ccompiler;
  }

  cache_vars["__nsbuild_preset"] = cxx.name;
}

void nspreset::write(std::ostream& ff, std::uint32_t options, nameval_list const& extras, nsbuild const& bc)
{
  json j;
  j["version"]              = 3;
  j["cmakeMinimumRequired"] = {{"major", 3}, {"minor", 20}, {"patch", 0}};

  // j["configurations"]
  auto& configurations = j["configurePresets"];
  for (auto const& cxx : bc.presets)
  {
    if (!bc.cmakeinfo.target_platform.empty() && cxx.platform != bc.cmakeinfo.target_platform)
      continue;
    // only write msvc
    json  cfg;
    auto& cache_vars = cfg["cacheVariables"];
    write_preset(cxx, options, cache_vars, cfg, bc);
    cfg["binaryDir"] = fmt::format("${{sourceDir}}/{}/{}/{}/main", bc.out_dir, cxx.name, bc.build_dir);
    if (!cxx.architecture.empty())
      cfg["architecture"] = {{"value", cxx.architecture}, {"strategy", "external"}};

    for (auto const& d : cxx.definitions)
      cache_vars[d.first] = d.second;
    for (auto const& d : extras)
      cache_vars[d.first] = d.second;
    cfg["installDir"] = std::string{"${sourceDir}/"} + bc.out_dir + "/" + cxx.name + "/" + bc.sdk_dir;
    configurations.emplace_back(std::move(cfg));
  }

  auto& builds = j["buildPresets"];
  for (auto const& cxx : bc.presets)
  {
    if (!bc.cmakeinfo.target_platform.empty() && cxx.platform != bc.cmakeinfo.target_platform)
      continue;
    json bld;
    bld["name"]            = cxx.name;
    bld["displayName"]     = cxx.display_name;
    bld["configurePreset"] = cxx.name;
    builds.emplace_back(std::move(bld));
  }

  ff << std::setw(2) << j << std::endl;
}

void nspreset::write(std::ostream& ff, std::uint32_t options, std::string_view bin_dir, nameval_list const& extras,
                     nsbuild const& bc)
{
  json j;
  j["version"]              = 3;
  j["cmakeMinimumRequired"] = {{"major", 3}, {"minor", 20}, {"patch", 0}};

  // j["configurations"]
  auto& configurations = j["configurePresets"];
  for (auto const& cxx : bc.presets)
  {
    if (!bc.cmakeinfo.target_platform.empty() && cxx.platform != bc.cmakeinfo.target_platform)
      continue;
    // only write msvc
    if (cxx.name == bc.cmakeinfo.cmake_preset_name)
    {

      json  cfg;
      auto& cache_vars = cfg["cacheVariables"];
      write_preset(cxx, options, cache_vars, cfg, bc);
      cfg["binaryDir"] = bin_dir;
      if (!cxx.architecture.empty())
        cfg["architecture"] = {{"value", cxx.architecture}, {"strategy", "external"}};

      for (auto const& d : cxx.definitions)
        cache_vars[d.first] = d.second;
      for (auto const& d : extras)
        cache_vars[d.first] = d.second;

      cfg["installDir"] = bc.get_full_sdk_dir();
      configurations.emplace_back(std::move(cfg));
      break;
    }
  }

  auto& builds = j["buildPresets"];
  for (auto const& cxx : bc.presets)
  {
    if (!bc.cmakeinfo.target_platform.empty() && cxx.platform != bc.cmakeinfo.target_platform)
      continue;
    if (cxx.name == bc.cmakeinfo.cmake_preset_name)
    {
      json bld;
      bld["name"]            = cxx.name;
      bld["displayName"]     = cxx.display_name;
      bld["configurePreset"] = cxx.name;
      builds.emplace_back(std::move(bld));
      break;
    }
  }

  ff << std::setw(2) << j << std::endl;
}
