
#include <nscmake_conststr.h>
#include <nsparams.h>

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

std::string_view to_string(inheritance);
std::string_view to_string(exposition);
void             print(std::ostream& ostr, std::string_view content);
std::string      dset(std::string_view name, std::string_view value)
{
  return fmt::format("-D{}={}", name, value);
}
std::string      get_filter(nsfilters);
std::string_view get_filter(nsfilter);
void             value(std::string& result, neo::list::vector::const_iterator b,
                       neo::list::vector::const_iterator e, char seperator = ';');
std::string      value(nsparams const&, char seperator = ';');

} // namespace cmake