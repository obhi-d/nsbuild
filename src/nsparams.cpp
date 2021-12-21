
#include <nsparams.h>
#include <nscmake.h>

void nsvars::print(std::ostream& os, output_fmt f, bool ignore_unfiltered) const
{
  bool has_filters = filters.any();
  if (!has_filters && ignore_unfiltered)
    return;

  auto sf = cmake::get_filter(filters);
  os << "\n# Conditional vars " << sf;
  for (auto const& v : params)
  {
    if (has_filters)
    {
      os << "\nset(" << prefix << v.name << " $<IF:" << sf << ", ";
      cmake::print(os, cmake::value(v.params));
      os << ", ${" << prefix << v.name << "}";
      if (f == output_fmt::set_cache)
        os << " CACHE INTERNAL \"\"";
      os << ")";
    }
    else
    {
      os << "\nset(" << prefix << v.name << " ";
      cmake::print(os, cmake::value(v.params));
      if (f == output_fmt::set_cache)
        os << " CACHE INTERNAL \"\"";
      os << ")";
    }
  }
}
