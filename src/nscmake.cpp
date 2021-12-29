#include "nscmake.h"
#include "nscmake_conststr.h"

namespace cmake
{

void append(std::string& o, std::string_view v, char sep)
{
  if (o.empty())
    o = v;
  else
  {
    o += sep;
    o += v;
  }
}

void value(std::string& result, neo::list::vector::const_iterator b,
           neo::list::vector::const_iterator e, char sep)
{
  for (auto it = b; it != e; ++it)
  {
    auto const& p = *it;
    if (p.index() == neo::command::k_param_single)
    {
      append(result, neo::command::as_string(p), sep);
    }
    else if (p.index() == neo::command::k_param_list)
    {
      auto const& l = std::get<neo::list>(p);
      value(result, l.begin(), l.end(), sep);
    }
  }
}

std::string value(nsparams const& params, char sep)
{
  std::string result;
  value(result, params.value().begin(), params.value().end(), sep);
  return result;
}

std::string path(std::filesystem::path const& path) 
{ 
  auto ret = path.generic_string(); 
  #if defined(WIN32) || defined(_WIN32) ||                                       \
    defined(__WIN32) && !defined(__CYGWIN__)
  std::replace(ret.begin(), ret.end(), '\\', '/');
  #endif
  return ret;
}

std::string value(std::string val) 
{ 
  std::replace(val.begin(), val.end(), '\\', '_'); 
  std::replace(val.begin(), val.end(), '.', '_');
  std::replace(val.begin(), val.end(), ' ', '_');
  return val;
}

std::string_view to_string(inheritance inh)
{
  switch (inh)
  {
  case inheritance::intf:
    return "INTERFACE";
  case inheritance::priv:
    return "PRIVATE";
  case inheritance::pub:
    return "PUBLIC";
  }
  return "";
}

std::string_view to_string(exposition ex)
{
  switch (ex)
  {
  case exposition::build:
    return "BUILD_INTERFACE";
  case exposition::install:
    return "INSTALL_INTERFACE";
  }
  return "";
}

void print(std::ostream& ostr, std::string_view content)
{
  foreach_variable(ostr, content,
                   [](std::ostream& ostr, std::string_view sv)
                   { ostr << "${" << sv << "}"; });
}

std::string_view get_filter(nsfilter filter)
{
  switch ((nsfilter)filter)
  {
  case nsfilter::clang:
    return "$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>";
  case nsfilter::msvc:
    return "$<COMPILE_LANG_AND_ID:CXX,MSVC>";
  case nsfilter::gcc:
    return "$<COMPILE_LANG_AND_ID:CXX,GNU>";
  }
  return "";
}

std::optional<std::string> get_filter(nspreset const& preset, nsfilters const& filter)
{
  if (filter.custom.empty() && !filter.known.any())
    return std::optional<std::string>{""};

  if (!filter.custom.empty())
  {
    for (auto const& f : filter.custom)
    {
      if ((std::find(preset.disallowed_filters.begin(), preset.disallowed_filters.end(), f) !=
           preset.disallowed_filters.end()) ||
          (std::find(preset.allowed_filters.begin(), preset.allowed_filters.end(), f) == preset.allowed_filters.end()))
        return std::optional<std::string>();
    }
  }

  if (!filter.known.any())
    return std::optional<std::string>{""};

  bool multiconditions = true;
  bool first   = true;

  std::string val;
  for (std::uint32_t i = 0; i < (std::uint32_t)nsfilter::count; ++i)
  {
    if (!filter.known[i])
      continue;

    if (!first)
    {
      if (multiconditions)
      {
        val     = "$<AND:" + val;
        multiconditions = false;
      }
      val += ",";
    }
    val += get_filter((nsfilter)i);
    first = false;
  }
  if (!multiconditions)
    val += ">";
  return val;
}

void line(std::ostream& oss, std::string_view name, char type, bool header) 
{
  const unsigned int shift  = header ? 16 : 2;
  auto               middle = (unsigned int)(((120u - name.length()) / 2) - shift);

  auto        midline = std::string(middle, type);
  auto        space   = std::string(shift, ' ');
  auto        name_len= std::string(name.length(), ' ');
  static auto line    = std::string(120, '=');

  if (header)
  {
    oss << "\n\n#" << line;
    oss << "\n#" << midline << space << name_len << space << midline;
  }
  oss << "\n#" << midline << space << name << space << midline;
  if (header)
  {
    oss << "\n#" << midline << space << name_len << space << midline;
    oss << "\n#" << line << "\n";
  }
}

} // namespace cmake