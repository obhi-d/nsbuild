
#include <fmt/format.h>
#include <nsmacros.h>

void nsmacros::im_print(std::ostream& ostr, std::string_view content,
                        output_fmt) const
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
                         subs = it->second;
                       m = m->fallback;
                     }

                     ostr << subs;
                   });
}

void nsmacros::print(std::ostream& os, output_fmt) const
{
  os << "\n# Config variables";
  for (auto const& v : macros)
  {
    os << "\nset(" << v.first << " \"";
    cmake::print(os, v.second);
    os << "\")";
  }
}
