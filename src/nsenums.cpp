#include "nsenums.h"
#include "nsbuild.h"
#include <fstream>
#include <regex>
extern void halt();

void nsenum_context::begin_header(std::ostream& ofs, std::unordered_set<std::string> const& includes)
{
  add_includes(ofs, includes);
}

void nsenum_context::start_namespace(std::ostream& ofs, std::string str)
{
  const std::string_view to        = "{ \nnamespace ";
  std::size_t            start_pos = 0;
  while ((start_pos = str.find("::", start_pos)) != std::string::npos)
  {
    str.replace(start_pos, 2, to);
    start_pos += to.length(); // ...
  }
  ofs << "\nnamespace " << str << "\n{";
  if (str != "lumiere")
    ofs << "\nusing namespace lumiere;";
}

void nsenum_context::end_namespace(std::ostream& ofs, std::string const& str)
{
  std::size_t start_pos = 0;
  ofs << "\n}";
  while ((start_pos = str.find("::", start_pos)) != std::string::npos)
  {
    ofs << "\n}";
    start_pos += 2;
  }
}

void nsenum_context::add_includes(std::ostream& ofs, std::unordered_set<std::string> const& includes)
{
  for (auto const& i : includes)
    ofs << fmt::format("\n#include <{}>", i);
}

std::string nsenum_context::prefix_words(std::string const& pref,
                                         std::string const& sentence,
                                         bool               is_single_word)
{
  if (pref.empty())
    return sentence;
  if (is_single_word)
    return pref + sentence;
  else
  {
    std::string result;
    std::string replace = fmt::format("{}$&", pref);
    std::regex  re("\\w+");
    return std::regex_replace(sentence, re, replace);
  }
}

std::string nsenum_context::modify_search(std::string const& on,
                                          nsenum_modifier    modifier)
{
  // clang-format off
  if (modifier == nsenum_modifier::lower)
    return "\n  String cpy {" + on + "};"
           "\n  std::transform(std::begin(cpy), std::end(cpy), std::begin(cpy), ::tolower);" 
           "\n  " + on + " = cpy;";
  else if (modifier == nsenum_modifier::upper)
    return "\n  String cpy {" + on + "};"
           "\n  std::transform(std::begin(cpy), std::end(cpy), std::begin(cpy), ::toupper);" 
           "\n  " + on + " = cpy;";
  // clang-format on
  return std::string{};
}

void nsenum_context::parse(
  std::string const& mod_name, 
  std::string const&           header, 
  std::filesystem::path const& lenumsj, std::ofstream& cpp,
  std::ofstream& hpp, std::vector<std::string> const& incl, bool exp)
{  
  // if (mod_name == "Graphics")
  //   halt();

  std::ifstream j{lenumsj};
  nlohmann::json js;
  j >> js;
  nsenum_context ctx;
  ctx.includes.emplace("FlagType.h");
  ctx.includes.emplace("StringKey.h");
  std::vector<nsenum> enums;
  for (auto const& en : js)
    enums.emplace_back(ctx, en);
  if (exp)
    ctx.export_api = fmt::format("Lum{0}API", mod_name);
  for (auto const& in : incl)
    ctx.includes.emplace(in);
  begin_header(hpp, ctx.includes);
  cpp << fmt::format("\n#include <{}>", header);
  for (auto const& p : enums)
  {
    p.write_header(hpp);
    p.write_source(cpp);
  }
}

void nsenum_context::write_file_header(std::ofstream& ofs, bool isheader)
{
  ofs << "\n // Auto generated enum file";
  if (isheader)
    ofs << "\n#pragma once\n";
  ofs << "\n\n";
}

void nsenum_context::generate(std::string mod_name, nsmodule_type type,
                              std::filesystem::path source,
                              std::filesystem::path gen)
{
  auto enums_json = source / "include" / "Enums.json";
  auto lenums_json = source / "local_include" / "Enums.json";
  bool has_enums_json = std::filesystem::exists(enums_json);
  bool has_lenums_json = std::filesystem::exists(lenums_json);
  if (!has_enums_json && !has_lenums_json)
    return;
  auto local_path = gen / "local";
  std::filesystem::create_directories(local_path);
  auto source_file = gen / "local" / fmt::format("Lum{}Enums.cpp", mod_name);
  std::ofstream            cpp{source_file};
  std::vector<std::string> headers;

  if (cpp.is_open())
  {
    write_file_header(cpp, false);
    headers.emplace_back(fmt::format("Lum{}.h", mod_name));

    if (has_enums_json)
    {
      auto          hname  = fmt::format("Lum{}Enums.h", mod_name);
      auto          header = gen / hname;
      std::ofstream hpp{header};
      if (hpp.is_open())
      {
        write_file_header(hpp, true);
        parse(mod_name, hname, enums_json, cpp, hpp, headers,
              type == nsmodule_type::lib || type == nsmodule_type::ref);
        headers.emplace_back(fmt::format("Lum{}Enums.h", mod_name));
      }
    }

    if (has_lenums_json)
    {
      auto          hname  = fmt::format("Lum{}LocalEnums.h", mod_name);
      auto header = gen / "local" / hname;
      std::ofstream hpp{header};
      if (hpp.is_open())
      {
        write_file_header(hpp, true);
        parse(mod_name, hname, lenums_json, cpp, hpp, headers, false);
      }
    }
  }
}
nsenum::nsenum(nsenum_context& ictx, nlohmann::json const& jv)
    : ctx{ictx}, jenum{jv}
{
  if (jenum.find("debug") != jenum.end())
    halt();
  auto it = jenum.find("name");
  if (it != jenum.end())
    name = (*it).get<std::string>();
  it = jenum.find("usage");
  if (it != jenum.end())
  {
    auto susage = (*it).get<std::string>();
    if (susage == "flags")
      usage = nsenum_usage::as_flags;
    else if (susage == "autoflags")
    {
      auto_flags = true;
      usage      = nsenum_usage::as_flags;
    }
    else if (susage == "const")
      usage = nsenum_usage::as_const;
  }

  it = jenum.find("options");
  if (it != jenum.end())
  {
    auto const& options = *it;
    for (auto const& v : options)
    {
      auto opt = v.get<std::string>();
      if (opt == "no-strings" || opt == "nostrings")
        string_table = false;
      if (opt == "string-key")
      {
        string_key = true;
        ictx.includes.emplace("StringKey.h");
      }
      if (opt == "custom-strings")
        custom_strings = true;
      if (opt == "skip-last-element")
        skip_last_element = true;
      if (opt == "no-auto-default")
        auto_default = false;
      if (opt == "suffix-match")
        suffix_match = true;
      if (opt == "prefix-match")
        prefix_match = true;
    }
  }
  it = jenum.find("stringModifier");
  if (it != jenum.end())
    string_modifier = get_modifier((*it).get<std::string>());
  it = jenum.find("searchModifier");
  if (it != jenum.end())
    search_modifier = get_modifier((*it).get<std::string>());
  it = jenum.find("comment");
  if (it != jenum.end())
    comment = (*it).get<std::string>();
  it = jenum.find("type");
  if (it != jenum.end())
    type = (*it).get<std::string>();
  it = jenum.find("namespace");
  if (it != jenum.end())
    namespace_name = (*it).get<std::string>();
  it = jenum.find("default");
  if (it != jenum.end())
    default_value = (*it).get<std::string>();
  it = jenum.find("include");
  if (it != jenum.end())
  {
    auto const& includes = *it;
    for (auto const& v : includes)
      ictx.includes.emplace(v.get<std::string>());
  }
  parse_associations();
  parse_entries();
  one = get_1_shift();
  if (auto_flags || usage == nsenum_usage::as_enum)
    has_enum = true;
  if (auto_flags || usage == nsenum_usage::as_flags)
    has_flags = true;
  else if (usage == nsenum_usage::as_const)
    has_consts = true;
}
inline std::string nsenum_entry::proper_name() const
{
  auto s = name;
  std::replace(s.begin(), s.end(), '.', '_');
  std::replace(s.begin(), s.end(), ' ', '_');
  return s;
}

inline std::string nsenum_entry::get_entry(nsenum_usage u) const
{
  auto s = proper_name();
  switch (u)
  {
  case nsenum_usage::as_enum:
    return "e" + s;
  case nsenum_usage::as_flags:
    return "f" + s;
  case nsenum_usage::as_const:
    return "k" + s;
  }
  return s;
}

inline std::string nsenum_entry::get_string(nsenum_modifier mod) const
{
  switch (mod)
  {
  case nsenum_modifier::upper:
    return to_upper(string_value);
  case nsenum_modifier::lower:
    return to_lower(string_value);
  case nsenum_modifier::upper_camel_case:
    return to_upper_camel_case(string_value);
  case nsenum_modifier::lower_camel_case:
    return to_lower_camel_case(string_value);
  case nsenum_modifier::camel_case:
    return to_camel_case(string_value);
  case nsenum_modifier::snake_case:
    return to_snake_case(string_value);
  default:
  case nsenum_modifier::none:
    return string_value;
  }
}

std::string nsenum_entry::get_value(std::vector<nsenum_entry> const& entries,
                                    nsenum_usage                     u) const
{
  auto svalue = value;
  for (auto const& e : entries)
    svalue = std::regex_replace(svalue, std::regex{"\\b" + e.name + "\\b"},
                                e.get_entry(u));
  return svalue;
}

void nsenum_entry::fill_tuples(bool                     custom_strings,
                               std::vector<std::string> values)
{
  tuple_values = std::move(values);
  if (custom_strings && tuple_values.size() >= 2)
    string_value = tuple_values[1];
}

void nsenum::parse_associations()
{
  if (string_table)
  {
    if (string_key)
      tuple_types.emplace_back("", "", "StringKey");
    else
      tuple_types.emplace_back("", "", "StringView");
  }
  auto it = jenum.find("associations");
  if (it != jenum.end())
  {
    auto const& v = *it;
    for (auto const& e : v)
    {
      std::string prefix, name, type;
      it = e.find("prefix");
      if (it != e.end())
        prefix = it->get<std::string>();
      it = e.find("name");
      if (it != e.end())
        name = it->get<std::string>();
      it = e.find("type");
      if (it != e.end())
        type = it->get<std::string>();
      tuple_types.emplace_back(std::move(prefix), std::move(name), std::move(type));
    }
  }
}

void nsenum::parse_entries()
{
  if (default_value.empty() && auto_default)
  {
    if (usage == nsenum_usage::as_enum)
    {

      default_entry    = nsenum_entry{"Unknown", "(" + type + ")-1"};
      has_auto_default = true;
    }
    else if (usage == nsenum_usage::as_flags)
    {

      default_entry    = nsenum_entry("None", "(" + type + ")-1", true);
      has_auto_default = true;
      has_special_default_autoflags = true;
    }
    else if (usage == nsenum_usage::as_const)
    {

      default_entry    = nsenum_entry("Invalid", "(" + type + ")-1");
      has_auto_default = true;
    }
    entries.push_back(default_entry);
  }
  auto it = jenum.find("values");
  if (it != jenum.end())
  {
    auto const& values = *it;
    for (auto const& v : values)
      enum_entry(v);
  }
}

std::string_view nsenum::get_1_shift()
{
  if (type == "uint64")
    return "1ull";
  return "1";
}

void nsenum::enum_entry(nlohmann::json const& j)
{
  std::string expression;
  if (j.is_array() && j.size() > 0)
    expression = j[0].get<std::string>();
  else
    expression = j.get<std::string>();

  std::string entry_name, expression_value;

  std::regex  re("\\b\\w*\\b");
  std::smatch match;
  if (std::regex_search(expression, match, re))
    entry_name = match[0].str();

  auto it = expression.find_first_of('=');
  if (it != expression.npos)
    expression_value = expression.substr(it + 1);
  else if (has_auto_default && usage == nsenum_usage::as_enum &&
           entries.size() == 1)
    expression_value = "0";
  else if (has_special_default_autoflags && entries.size() == 1)
    expression_value = "0";
  nsenum_entry entry = nsenum_entry{entry_name, expression_value, false, true};
  if (entry_name == default_value)
    default_entry = entry;

  std::vector<std::string> values;
  if (j.is_array() && j.size() > 0)
  {
    for (auto const& v : j)
    {
      using type = decltype(v.type());
      switch(v.type()) 
      {
      case type::number_float:
        values.push_back(std::to_string(v.get<float>()));
        break;
      case type::string:
        values.push_back(v.get<std::string>());
        break;
      case type::boolean:
        values.push_back(v.get<bool>() ? "true" : "false");
        break;
      case type::number_integer:
        values.push_back(std::to_string(v.get<std::int64_t>()));
        break;
      case type::number_unsigned:
        values.push_back(std::to_string(v.get<std::uint64_t>()));
        break;
      default:
        values.emplace_back();
        break;
      }
    }
      
  }
  else
    values.push_back(j.get<std::string>());

  entry.fill_tuples(custom_strings, std::move(values));
  entries.emplace_back(std::move(entry));
}

nsenum_usage nsenum::default_usage_type() const
{
  if (has_enum)
    return nsenum_usage::as_enum;
  if (has_flags)
    return nsenum_usage::as_flags;
  if (has_consts)
    return nsenum_usage::as_const;
  return nsenum_usage::as_enum;
}

std::string_view nsenum::default_value_type() const
{
  if (has_enum)
    return "Enum";
  else if (has_flags)
    return "Bit";
  else if (has_consts)
    return "Const";
  return "Enum";
}

bool nsenum::has_tuples_to_print() const
{
  return string_table || tuple_types.size() > 0;
}

void nsenum::write_header(std::ostream& ofs) const
{
  nsenum_context::start_namespace(ofs, namespace_name);
  if (!comment.empty())
    ofs << "\n// " << comment;
  ofs << fmt::format("\nstruct {} {}\n{{", ctx.export_api, enum_name());
  if (auto_flags || usage == nsenum_usage::as_enum)
  {
    write_header_enum(ofs, usage == nsenum_usage::as_enum);
    if (auto_flags)
      write_header_auto_flags(ofs);
    else if (has_flags)
      write_header_flags(ofs);
  }
  else if (usage == nsenum_usage::as_flags)
  {
    if (auto_flags)
      write_header_auto_flags(ofs);
    else
      write_header_flags(ofs);
  }
  else if (usage == nsenum_usage::as_const)
  {
    write_header_consts(ofs);
  }
  if (has_tuples_to_print())
  {
    write_header_value_type(ofs);
    write_header_value_functions(ofs);
    if (string_table)
    {
      if (string_key)
        write_header_string_key_table(ofs);
      else
        write_header_string_table(ofs);
    }
  }
  ofs << "\n};";
  write_header_alias(ofs);
  ctx.end_namespace(ofs, namespace_name);
  //
}

void nsenum::write_header_alias(std::ostream& ofs) const
{
  if (has_enum)
    ofs << fmt::format("\nusing {}Enum = {}::Enum;", name, enum_name());
  if (has_flags)
  {
    ofs << fmt::format("\nusing {}Flags = MaskFlags<{}::Bit>;", name, name);
    ofs << fmt::format("\nL_DECLARE_SCOPED_MASK_FLAGS({}, Bit);", name);
  }
}

void nsenum::write_header_string_table(std::ostream& ofs) const
{
  auto value_type = default_value_type();
  ofs << fmt::format("\n  using Tuple = std::tuple<StringView, {}>;",
                     value_type)
      << fmt::format("\n  static const std::array<Tuple, {}>& Table();",
                     entries.size());
  if (suffix_match || prefix_match)
  {
    if (prefix_match)
      ofs << fmt::format("\n  static std::tuple<{}, StringView> "
                         "StartsWith(StringView iParam);",
                         value_type);
    if (suffix_match)
      ofs << fmt::format(
          "\n  static std::tuple<{}, StringView> EndsWith(StringView iParam);",
          value_type);
  }
  // clang-format off
  else
  {
    ofs << fmt::format("\n  static {0} FromString(StringView iParam);"
                       "\n  static inline {0} To{0}(StringView iParam) {{ return FromString(iParam); }}",
                       value_type);
  }
  if (auto_flags && has_enum && has_flags)
      ofs << fmt::format("\n  static Bit  ToBit(Enum iValue) {{ return static_cast<Bit>({} << iValue); }}"
                         "\n  static Bit  ToBit(StringView iValue) {{ return ToBit(FromString(iValue)); }}" 
                         "\n  static Enum ToEnum(Bit iValue) {{ return static_cast<Enum>(vml::bit_count(iValue)); }}", one );
  // clang-format on
}

void nsenum::write_header_string_key_table(std::ostream& ofs) const
{
  auto value_type = default_value_type();
  // clang-format off
  // value_type str(len(self.entries))
  ofs << fmt::format("\n  using Tuple = std::tuple<StringKey, {0}>;\n" 
                     "\n  static const std::array<Tuple, {1}>& Table();"
                     "\n  static {0} FromStringKey(StringKey iParam);" 
                     "\n  static inline {0} To{0}(StringKey iParam) {{ return FromStringKey(iParam); }}", value_type, entries.size());

  if (auto_flags && has_enum && has_flags)
    ofs << fmt::format("\n  static Bit ToFlag(StringKey iValue) {{ return static_cast<Bit>({} << FromStringKey(iValue)); }}"
                       "\n  static Enum ToEnum(Bit iValue)      {{ return static_cast<Enum>(vml::bit_count(iValue));     }}\n", one);
  // clang-format on
}

void nsenum::write_header_value_functions(std::ostream& ofs) const
{
  auto value_type = default_value_type();
  ofs << fmt::format("\n  static Value ToValue({} iFrom);", value_type);
  if (string_key)
    ofs << fmt::format("\n  static StringKey ToStringKey({} iFrom) {{ return "
                       "std::get<0>(ToValue(iFrom)); }}",
                       value_type);
  else
    ofs << fmt::format("\n  static StringView ToString({} iFrom) {{ return "
                       "std::get<0>(ToValue(iFrom)); }}",
                       value_type);
  auto it = jenum.find("declarations");
  if (it != jenum.end())
  {
    auto const& declarations = *it;
    ofs << ("\n  // Declarations \n");
    if (declarations.is_array())
    {
      for (auto const& c : declarations)
      {
        ofs << "\n  " << c.get<std::string>();
      }
    }
    else
      ofs << "\n  " << declarations.get<std::string>();
    ofs << "\n  // End of declarations \n";
  }

  int i = 0;
  for (auto const& t : tuple_types)
  {
    if (!t.name.empty())
    {
      ofs << fmt::format("\n  static {} Get{}({} iValue) {{ return "
                         "std::get<{}>(ToValue(iValue)); }}",
                         t.type, t.name, value_type, i);
    }
    i++;
  }
}

void nsenum::write_header_value_type(std::ostream& ofs) const
{
  ofs << "\n  using Value = std::tuple<";
  bool print_comma = false;
  for (auto const& t : tuple_types)
  {

    if (print_comma)
      ofs << ", ";
    else
      print_comma = true;
    ofs << t.type;
  }
  ofs << ">;\n";
}

void nsenum::write_header_consts(std::ostream& ofs) const
{
  ofs << fmt::format("\n  enum Const : {}\n{{", type);
  bool print_comma = false;
  for (auto const& entry : entries)
  {
    if (print_comma)
      ofs << ", ";
    else
      print_comma = true;
    ofs << "\n    " << entry.get_entry(nsenum_usage::as_const);
    if (!entry.value.empty())
      ofs << " = " << entry.get_value(entries, nsenum_usage::as_const);
  }
  ofs << "\n  };\n";
}

void nsenum::write_header_enum(std::ostream& ofs, bool write_value) const
{
  ofs << fmt::format("\n  enum Enum : {}\n  {{", type);
  bool print_comma = false;
  for (auto const& entry : entries)
  {
    if (print_comma)
      ofs << ", ";
    else
      print_comma = true;
    ofs << "\n    " << entry.get_entry(nsenum_usage::as_enum);
    if (!entry.value.empty() &&
        (write_value || entry.flag_is_0 || entry.enfore_write_enum_val))
      ofs << " = " << entry.get_value(entries, nsenum_usage::as_enum);
  }
  if (!skip_last_element)
  {
    if (print_comma)
      ofs << ",\n";
    ofs << "    kCount";
  }
  ofs << "\n  };\n";
}

void nsenum::write_header_flags(std::ostream& ofs) const
{
  ofs << fmt::format("\n  enum Bit : {}\n  {{", type);
  bool print_comma = false;
  for (auto const& entry : entries)
  {
    if (print_comma)
      ofs << ", ";
    else
      print_comma = true;
    ofs << "\n    " << entry.get_entry(nsenum_usage::as_flags);
    if (!entry.value.empty())
      ofs << " = " << entry.get_value(entries, nsenum_usage::as_flags);
  }
  ofs << "\n  };\n";
}

void nsenum::write_header_auto_flags(std::ostream& ofs) const
{
  ofs << fmt::format("\n  enum Bit : {}\n  {{", type);
  bool print_comma = false;
  for (auto const& entry : entries)
  {
    if (print_comma)
      ofs << ", ";
    else
      print_comma = true;
    ofs << "\n    " << entry.get_entry(nsenum_usage::as_flags);
    if (entry.flag_is_0)
      ofs << " = 0";
    else
      ofs << fmt::format(" = {} << {}", one,
                         entry.get_entry(nsenum_usage::as_enum));
  }
  if (!skip_last_element)
  {
    if (print_comma)
      ofs << ", ";
    ofs << fmt::format("\n    fLastFlag = {} << kCount", one);
  }
  ofs << "\n  };\n";
}

void nsenum::write_source(std::ostream& ofs) const
{
  nsenum_context::start_namespace(ofs, namespace_name);
  if (has_tuples_to_print())
    write_source_to_value(ofs);
  if (string_table)
    write_source_string_table(ofs);
  nsenum_context::end_namespace(ofs, namespace_name);
}

void nsenum::write_source_to_value(std::ostream& ofs) const 
{
  // clang-format off
  auto value_type = default_value_type();
  auto usage_type = default_usage_type();

  ofs << fmt::format("\n{0}::Value {0}::ToValue({1} iValue) \n{{\n  switch(iValue) \n  {{\n", name, value_type);
  for (auto const& entry : entries)
  {
      bool is_default = !default_entry.name.empty() && entry.name == default_entry.name;
      if (tuple_types.size() > 0)
      {
         if (is_default)
              ofs << "\n  default:";
          ofs << "\n  case " << entry.get_entry(usage_type) << ": return Value{ ";
          int val_start = 0;
          int typ_start = 0;
          if (string_table)
          {
              val_start = 1;
              typ_start = 1;
              if (custom_strings)
                  val_start = 2;
              ofs << "\"" << entry.get_string(string_modifier) << "\"";
          }
          if ((int)tuple_types.size() > typ_start)
          {
              if (!entry.tuple_values.empty())
              {
                  int tup_val_max = (int)entry.tuple_values.size();
                  int tup_ty_max = (int)tuple_types.size();
                  for (int iv = val_start, ip = typ_start; iv < tup_val_max && ip < tup_ty_max; ++iv, ++ip)
                  //for v, p in zip(entry.tuple_values[val_start:], self.tuple_types[typ_start:]):
                  {
                      auto const& p = tuple_types[(std::size_t)ip];
                      auto v = entry.tuple_values[(std::size_t)iv];
                      auto const& pfx = p.prefix;
                      auto const& p_type = p.type;
                      if (p.is_string)
                          v = fmt::format("\"{}\"", nsenum_context::prefix_words(pfx, v, true));
                      else
                          v = nsenum_context::prefix_words(pfx, v, false);
                      ofs << ", " << v;
                  }
              }
              else
              {
                  int tup_ty_max = (int)tuple_types.size();
                  for (int ip = typ_start; ip < tup_ty_max; ++ip)
                  {
                      auto const& p = tuple_types[(std::size_t)ip];

                      if (p.is_string)
                          ofs << ", \"\"";
                      else
                          ofs << ", " << p.type << "{0}";
                  }
              }
          }
          ofs << " };";
      }
  }
  ofs << "\n  }\n  return Value{};\n}";
}

void nsenum::write_source_string_table(std::ostream& ofs) const
{
  auto const& enum_name      = name;
  auto        item_count     = entries.size();
  auto        def_usage_type = default_usage_type();
  auto        def_value_type = default_value_type();
  ofs << fmt::format("\nusing {0}Tuple = {0}::Tuple;\n", name);
  auto table_type = fmt::format("std::array<{}Tuple, {}>", name, item_count);

  if (string_key)
    ofs << fmt::format("\n{0} k{1}StringTable = SortTuple<{0}, 0>({0}{{\n",
                       table_type, name);
  else
    ofs << fmt::format("\n{0} k{1}StringTable = {{\n", table_type, name);

  nsenum_entry_list sorted_list;
  if (suffix_match || prefix_match)
  {
    sorted_list = entries;
    std::sort(sorted_list.begin(), sorted_list.end(),
              [this](nsenum_entry const& first, nsenum_entry const& second)
              {
                return sorted_tuple(first.get_string(string_modifier)) <
                       sorted_tuple(second.get_string(string_modifier));
              });
  }
  else
  {
    sorted_list = entries;
    std::sort(sorted_list.begin(), sorted_list.end(),
              [this](nsenum_entry const& first, nsenum_entry const& second)
              {
                return first.get_string(string_modifier) <
                       second.get_string(string_modifier);
              });
  }

  for (auto const& sorted_entry : sorted_list)
    ofs << fmt::format("\n  {0}Tuple{{ \"{1}\", {0}::{2} }},", name,
                       sorted_entry.get_string(string_modifier),
                       sorted_entry.get_entry(def_usage_type));

  if (string_key)
    ofs << "\n});\n";
  else
    ofs << "\n};\n";

  ofs << fmt::format(
      "\nstd::array<{0}::Tuple, {1}> const& {0}::Table() \n{{\n  "
      "return k{0}StringTable;\n}}\n",
      enum_name, item_count);
  auto finish_with = [&]()
  {
    if (!default_entry.name.empty())
      ofs << fmt::format("::{});\n}}\n",
                         default_entry.get_entry(def_usage_type));
    else
      ofs << "(0));\n}\n";
  };

  if (string_key)
  {
    ofs << fmt::format(
        "\n{0}::{1} {0}::FromStringKey(StringKey iValue) \n{{\n  return "
        "EnumFromString(k{0}StringTable, iValue, {0}::{1}",
        enum_name, def_value_type);
    finish_with();
  }
  else if (suffix_match || prefix_match)
  {

    if (prefix_match)
    {
      ofs << fmt::format("\nstd::tuple<{0}::{1}, StringView> "
                         "{0}::StartsWith(StringView iValue) \n{{\n",
                         enum_name, def_value_type);
      if (search_modifier != nsenum_modifier::none)
        ofs << nsenum_context::modify_search("iValue", search_modifier);
      ofs << fmt::format(
          "\n  return EnumStringStartsWith(k{0}StringTable, iValue, {0}::{1}",
          enum_name, def_value_type);
      finish_with();
    }
    if (suffix_match)
    {

      ofs << fmt::format("\nstd::tuple<{0}::{1}, StringView> "
                         "{0}::EndsWith(StringView iValue) \n{{\n",
                         enum_name, def_value_type);
      if (search_modifier != nsenum_modifier::none)
        ofs << nsenum_context::modify_search("iValue", search_modifier);
      ofs << fmt::format(
          "\n  return EnumStringEndsWith(k{0}StringTable, iValue, {0}::{1}",
          enum_name, def_value_type);
      finish_with();
    }
  }
  else
  {
    ofs << fmt::format("\n{0}::{1} {0}::FromString(StringView iValue) \n{{\n",
                       enum_name, def_value_type);

    if (search_modifier != nsenum_modifier::none)
      ofs << nsenum_context::modify_search("iValue", search_modifier);
    ofs << fmt::format(
        "\n  return EnumFromString(k{0}StringTable, iValue, {0}::{1}",
        enum_name, def_value_type);
    finish_with();
  }
}
