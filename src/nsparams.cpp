
#include <nsparams.h>

void nsvars::print(std::ostream& os, output_fmt f) const
{
  if (!filters.any())
    return;

  auto sf = cmake::get_filter(filters);
  os << "\n# Conditional vars " << sf;
  for (auto const& v : params)
  {

    os << "\nset(" << prefix << v.name << " $<IF:" << sf << ", ";
    cmake::print(os, cmake::value(v.params));
    os << ", ${var_" << v.name << "}";
    if (f == output_fmt::set_cache)
      os << " CACHE INTERNAL \"\"";
    os << ")";
  }
}
