
#include <fmt/format.h>
#include <nscmake.h>
#include <nsmacros.h>

void nsmacros::print(std::ostream& os, output_fmt f) const
{
  os << "\n# Config variables";
  switch (f)
  {
  case output_fmt::cache_entry:
    for (auto const& v : order)
    {
      os << v.first.get() << "=";
      cmake::print(os, v.second);
      os << "\n\n";
    }
    break;
  default:
  case output_fmt::cmake_def:
    for (auto const& v : order)
    {
      os << "\nset(" << v.first.get() << " \"";
      cmake::print(os, v.second);
      os << "\")";
    }
    break;
  }
  os << "\n";
}
