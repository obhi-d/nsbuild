#pragma once
#include <filesystem>
#include <nscommon.h>
#include <vector>

struct nsbuild;
namespace nsprocess
{
// static void git(std::vector<std::string> args, std::filesystem::path wd);
void cmake_config(nsbuild const& bc, std::vector<std::string> args, std::string src, std::filesystem::path wd);
void cmake_build(nsbuild const& bc, std::string_view target, std::filesystem::path wd);
void cmake_install(nsbuild const& bc, std::string_view prefix, std::filesystem::path wd);

void execute(std::string_view name, nsbuild const& bc, std::vector<std::string> args, std::filesystem::path wd);
void cmake(nsbuild const& bc, std::vector<std::string> args, std::filesystem::path wd);
void git(nsbuild const& bc, std::vector<std::string> args, std::filesystem::path wd);
void powershell(nsbuild const& bc, std::vector<std::string> args, std::filesystem::path wd);

bool download(nsbuild const& bc, std::filesystem::path const& dl, std::string_view const& repo, std::string_view name,
              std::string_view version, bool force);
void git_clone(nsbuild const& bc, std::filesystem::path const& dl, std::string_view const& repo, std::string_view tag);

std::filesystem::path        get_nsbuild_path();
extern std::filesystem::path s_nsbuild;
}; // namespace nsprocess
