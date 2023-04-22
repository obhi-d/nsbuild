
#include <nscmake.h>
#include <nsvars.h>

void nsvars::print(std::ostream& os, output_fmt f, bool ignore_unfiltered, char sep) const
{
  bool has_filters = !filters.empty();
  if (!has_filters && ignore_unfiltered)
    return;

  auto sf = filters;
  if (has_filters)
    os << "\n# Conditional vars \n" << sf;

  std::string_view start  = (f == output_fmt::cache_entry) ? "\n" : "\nset(";
  std::string_view end    = (f == output_fmt::cache_entry) ? "\n" : ")";
  std::string_view assign = (f == output_fmt::cache_entry) ? "=" : " ";

  for (auto const& v : params)
  {
    if (has_filters)
    {
      os << start << prefix << v.name << assign << "$<IF:" << sf << ", ";
      cmake::print(os, cmake::value(v.params, sep));
      os << ", ${" << prefix << v.name << "}>";
      if (f == output_fmt::set_cache)
        os << " CACHE INTERNAL \"\"";
      os << end;
    }
    else
    {
      os << start << prefix << v.name << assign;
      cmake::print(os, cmake::value(v.params, sep));
      if (f == output_fmt::set_cache)
        os << " CACHE INTERNAL \"\"";
      os << end;
    }
  }
}
