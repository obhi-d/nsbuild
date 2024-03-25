#include "nsinstallers.h"

#include "nslog.h"

#include <fstream>

void nsinstallers::save(std::filesystem::path const& path) const
{
  std::ofstream file(path);

  for (auto const& e : uninstallers)
  {
    if (e.second)
      file << e.first << "\n";
  }
}

void nsinstallers::load(std::filesystem::path const& path)
{
  std::ifstream file(path);
  clear();
  std::string v;
  while (std::getline(file, v))
    uninstallers[v] = false;
}

void nsinstallers::uninstall_unused_and_save(std::filesystem::path const& path) const
{

  for (auto const& e : uninstallers)
  {
    if (!e.second)
    {
      uninstall(e.first);
    }
  }
}

void nsinstallers::uninstall(std::string const& entry) const
{
  if (std::filesystem::exists(entry))
  {
    std::ifstream install_manifest(entry);
    std::string   line;

    if (install_manifest)
    {
      while (std::getline(install_manifest, line))
      {
        nslog::print(fmt::format("Deleting : {}", line));
        std::error_code ec;
        std::filesystem::remove(line, ec);
      }
    }
  }
}