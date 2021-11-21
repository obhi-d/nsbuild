
#include <nscommon.h>
#include <nsparams.h>

std::unordered_map<std::string_view, nsfilter> filters =
{
  {"debug", nsfilter::debug },
  {"release", nsfilter::release },
  {"windows", nsfilter::windows },
  {"linux", nsfilter::linux },
  {"osx", nsfilter::osx },
  {"clang", nsfilter::clang },
  {"msvc", nsfilter::msvc },
  {"gcc", nsfilter::gcc },
  {"asan", nsfilter::asan },
};
//shared_lib, static_lib, system_lib, include_list, binary_list,
    //dependency_list

std::unordered_map<std::string_view, nspath_type> path_type = {
    {"shared", nspath_type::shared_lib},
    {"static", nspath_type::static_lib},
    {"system", nspath_type::system_lib},
    {"include", nspath_type::include_list},
    {"binary", nspath_type::binary_list},
    {"dependencies", nspath_type::dependency_list}};

nsfilter classify_filter(std::string_view name)
{
  auto it = filters.find(name);
  if (it != filters.end())
    return it->second;
  return nsfilter::unknown;
}

nsfilters get_filters(neo::command::param_t const& p)
{
  nsfilters flags;
  if (p.index() == neo::command::k_param_single)
  {
    flags.set(static_cast<unsigned>(classify_filter(neo::command::as_string(p))), true);
  }
  else if(p.index() == neo::command::k_param_list)
  {
    auto const& l = std::get<neo::list>(p);
    for (auto const& v : l)
    {
      flags.set(
          static_cast<unsigned>(classify_filter(neo::command::as_string(v))),
          true);
    }
  }
  return flags;
}

nspath_type get_nspath_type(std::string_view name)
{
  auto it = path_type.find(name);
  if (it != path_type.end())
    return it->second;
  return nspath_type::unknown;
}