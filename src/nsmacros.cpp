
#include <fmt/format.h>
#include <nscmake.h>
#include <nsmacros.h>

void nsmacros::im_print(std::ostream& ostr, std::string_view content, output_fmt) const
{
  foreach_variable(ostr, content,
                   [this](std::ostream& ostr, std::string_view sv)
                   {
                     std::string      wrapper{sv};
                     std::string_view subs;
                     auto const*      m = this;
                     while (m)
                     {
                       auto it = m->macros.find(wrapper);
                       if (it != m->macros.end())
                         subs = order[it->second].second;
                       m = m->fallback;
                     }

                     ostr << subs;
                   });
}

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
