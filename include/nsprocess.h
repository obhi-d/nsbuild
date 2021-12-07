
#include <filesystem>
#include <nscommon.h>
#include <vector>

namespace nsprocess
{
static void git(std::vector<std::string> args, std::filesystem::path wd);
static void cmake(std::vector<std::string> args, std::filesystem::path wd);
}; // namespace nsprocess
