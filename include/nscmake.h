#pragma once
#include "nsvars.h"
#include "nspreset.h"
#include <optional>

namespace cmake
{

enum class inheritance
{
  priv,
  pub,
  intf
};

enum class exposition
{
  install,
  build
};

std::string_view   to_string(inheritance);
std::string_view   to_string(exposition);
void               print(std::ostream& ostr, std::string_view content);
inline std::string dset(std::string_view name, std::string_view value) { return fmt::format("-D{}={}", name, value); }
std::optional<std::string> get_filter(nspreset const& preset, nsfilters const&);
void        value(std::string& result, neo::list::vector::const_iterator b, neo::list::vector::const_iterator e,
                  char seperator = ';');
std::string value(nsparams const&, char seperator = ';');
std::string path(std::filesystem::path const&);
void        line(std::ostream&, std::string_view name);
} // namespace cmake