#pragma once

#include <nsvars.h>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>

/// @brief Macros do not depend on filters
struct nsmacros
{

  using value = std::string;

  std::unordered_map<std::string, int>                                           macros;
  std::vector<std::pair<std::reference_wrapper<std::string const>, std::string>> order;

  /// @brief Immediate mode print will do immediate mode substitutions
  /// @param ostr Stream
  /// @param content Input content
  /// @param Format options
  template <typename O>
  void im_print(O& ostr, std::string_view content, output_fmt = output_fmt::cmake_def) const;

  void print(std::ostream&, output_fmt = output_fmt::cmake_def) const;

  inline value& operator[](std::string const& name)
  {
    auto it = macros.find(name);
    if (it == macros.end())
    {
      auto it2 = macros.emplace(name, (int)order.size());
      order.emplace_back(std::cref(it2.first->first), "");
      it = it2.first;
    }
    return order[it->second].second;
  }

  nsmacros const* fallback = nullptr;
};

template <typename O>
void nsmacros::im_print(O& ostr, std::string_view content, output_fmt) const
{
  foreach_variable(ostr, content,
                   [this](O& ostr, std::string_view sv)
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

                     write(ostr, subs);
                   });
}
