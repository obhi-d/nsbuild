
#include <nscommon.h>
#include <nsparams.h>

std::unordered_map<std::string_view, nsfilter> filters = {
    {"debug", nsfilter::debug},     {"release", nsfilter::release},
    {"windows", nsfilter::windows}, {"linux", nsfilter::linux},
    {"macOS", nsfilter::macOS},     {"clang", nsfilter::clang},
    {"msvc", nsfilter::msvc},       {"gcc", nsfilter::gcc},
    {"asan", nsfilter::asan},
};
// shared_lib, static_lib, system_lib, include_list, binary_list,
// dependency_list

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
    flags.set(
        static_cast<unsigned>(classify_filter(neo::command::as_string(p))),
        true);
  }
  else if (p.index() == neo::command::k_param_list)
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

std::string to_upper(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::toupper(c); } // correct
  );
  return s;
}

std::string to_lower(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); } // correct
  );
  return s;
}

std::string to_lower_camel_case(std::string const& s)
{
  if (s.length() == 0)
    return s;
  bool        next_upper = false;
  std::string result;
  result.reserve(s.size());
  result.push_back(std::tolower(s[0]));
  for (std::size_t i = 1; i < s.size(); ++i)
  {
    if (s[i] == '_')
    {
      next_upper = true;
      continue;
    }
    else if (next_upper)
    {
      result.push_back(std::toupper(s[i]));
      next_upper = false;
    }
    else
      result.push_back(s[i]);
  }
  return result;
}

std::string to_upper_camel_case(std::string const& s)
{
  if (s.length() == 0)
    return s;
  bool        next_upper = false;
  std::string result;
  result.reserve(s.size());
  result.push_back(std::toupper(s[0]));
  for (std::size_t i = 1; i < s.size(); ++i)
  {
    if (s[i] == '_')
    {
      next_upper = true;
      continue;
    }
    else if (next_upper)
    {
      result.push_back(std::toupper(s[i]));
      next_upper = false;
    }
    else
      result.push_back(s[i]);
  }
  return result;
}

std::string to_camel_case(std::string const& s)
{
  if (s.length() == 0)
    return s;
  bool        next_upper = false;
  std::string result;
  result.reserve(s.size());
  result.push_back(s[0]);
  for (std::size_t i = 1; i < s.size(); ++i)
  {
    if (s[i] == '_')
    {
      next_upper = true;
      continue;
    }
    else if (next_upper)
    {
      result.push_back(std::toupper(s[i]));
      next_upper = false;
    }
    else
      result.push_back(s[i]);
  }
  return result;
}

std::string to_snake_case(std::string const& s)
{
  if (s.length() == 0)
    return s;
  std::string result;
  result.reserve(s.size());
  result.push_back(std::tolower(s[0]));
  for (std::size_t i = 1; i < s.size(); ++i)
  {
    if (std::isupper(s[i]))
    {
      result.push_back('_');
      result.push_back(std::tolower(s[i]));
    }
    else
    {
      result.push_back(s[i]);
    }
  }
  return result;
}
