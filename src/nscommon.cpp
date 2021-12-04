
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

namespace cmake
{

void append(std::string& o, std::string_view v) 
{
  if (o.empty())
    o = v;
  else
  {
    o += ";";
    o += v;
  }
}

std::string value(nsparams const& params)
{ 
  std::string result;
  auto const& vv = params.value();
  std::vector<neo::list::vector const*> ss;
  ss.push_back(&vv);
  while (!ss.empty())
  {
    auto const& vv = *ss.back();
    ss.pop_back();
    for (auto const& p : vv)
    {
      if (p.index() == neo::command::k_param_single)
      {
        append(result, neo::command::as_string(p));
      }
      else if (p.index() == neo::command::k_param_list)
      {
        auto const& l = std::get<neo::list>(p);
        ss.push_back(&l.value());
      }
    }
  }
  return result;
}

void print(std::ostream& ostr, std::string_view content) 
{
  foreach_variable(ostr, content,
                   [](std::ostream& ostr, std::string_view sv) 
    { ostr << "${" << sv << "}";
    });
}

std::string get_filter(nsfilters filter) 
{ 
  bool        needsor = true;
  bool        first   = true;

  std::string val;
  for (std::uint32_t i = 0; i < (std::uint32_t)nsfilter::count; ++i)
  {
    if (!filter[i])
      continue;

    if (!first)
    {
      if (needsor)
      {
        val     = "$<OR:" + val;
        needsor = false;
      }
      val += ",";
    }
      
    switch ((nsfilter)i)
    {
    case nsfilter::debug:
      val += "$<CONFIG:Debug>";
      break;
    case nsfilter::release:
      val += "$<CONFIG:RelWithDebInfo>";
      break;
    case nsfilter::windows:
      val += "$<PLATFORM_ID:Windows>";
      break;
    case nsfilter::linux:
      val += "$<PLATFORM_ID:Linux>";
      break;
    case nsfilter::osx:
      val += "$<PLATFORM_ID:Darwin>";
      break;
    case nsfilter::clang:
      val += "$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>";
      break;
    case nsfilter::msvc:
      val += "$<COMPILE_LANG_AND_ID:CXX,MSVC>";
      break;
    case nsfilter::gcc:
      val += "$<COMPILE_LANG_AND_ID:CXX,GNU>";
      break;
    case nsfilter::asan:
      val += "$<BOOL:${L_ASAN_ACTIVE}>";
    }
    first = false;
  }
  if (!needsor)
    val += ">";
  return val;
}
}