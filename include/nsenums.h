#pragma once
#include <nlohmann/json.hpp>
#include <nscommon.h>
#include <nsmodule.h>
#include <unordered_set>

enum class nsenum_usage
{
  as_enum,
  as_flags,
  as_const
};

enum class nsenum_modifier
{
  upper,
  lower,
  lower_camel_case,
  upper_camel_case,
  camel_case,
  snake_case,
  none
};

inline nsenum_modifier get_modifier(std::string n)
{
  if (n == "lower")
    return nsenum_modifier::lower;
  else if (n == "upper")
    return nsenum_modifier::upper;
  else if (n == "lowerCamelCase")
    return nsenum_modifier::lower_camel_case;
  else if (n == "upperCamelCase")
    return nsenum_modifier::upper_camel_case;
  else if (n == "camelCase")
    return nsenum_modifier::camel_case;
  else if (n == "snakeCase")
    return nsenum_modifier::snake_case;
  return nsenum_modifier::none;
}

struct nsenum_entry
{
  std::string              name;
  std::string              string_value;
  std::string              value;
  std::vector<std::string> tuple_values;
  bool                     flag_is_0             = false;
  bool                     enfore_write_enum_val = false;

  nsenum_entry() = default;
  nsenum_entry(std::string iname, std::string ivalue, bool iflag_is_0 = false, bool write_enum_val = false)
  {
    name                  = std::move(iname);
    string_value          = name;
    value                 = std::move(ivalue);
    flag_is_0             = iflag_is_0;
    enfore_write_enum_val = write_enum_val;
  }

  std::string proper_name() const;
  std::string get_entry(nsenum_usage u) const;
  std::string get_string(nsenum_modifier mod) const;
  std::string get_value(std::vector<nsenum_entry> const& entries, nsenum_usage u) const;
  void        fill_tuples(bool custom_strings, std::vector<std::string> values);
};

struct nstuple_type
{
  std::string prefix;
  std::string type;
  std::string name;
  bool        is_string = false;
  nstuple_type()        = default;
  nstuple_type(std::string n) : name{std::move(n)} {}
  nstuple_type(std::string pfx, std::string n, std::string t)
      : prefix{std::move(pfx)}, name{std::move(n)}, type{std::move(t)}
  {
    if (type == "String" || type == "StringView" || type == "std::string" || type == "std::string_view")
      is_string = true;
  }
};

using nstuple_type_list = std::vector<nstuple_type>;
using nsenum_entry_list = std::vector<nsenum_entry>;

struct nsbuild;

struct nsenum_context
{
  std::string                     export_api;
  std::unordered_set<std::string> includes;

  static void        begin_header(std::ostream& ofs, std::unordered_set<std::string> const&);
  static void        start_namespace(std::ostream& ofs, std::string);
  static void        end_namespace(std::ostream& ofs, std::string const&);
  static void        add_includes(std::ostream& ofs, std::unordered_set<std::string> const&);
  static std::string prefix_words(std::string const& pref, std::string const& sentence, bool is_single_word);
  static std::string modify_search(std::string const& on, nsenum_modifier modifier);

  static void parse(std::string const& mod, std::string const& header, std::filesystem::path const&, std::ofstream& cpp,
                    std::ofstream& hpp, std::vector<std::string> const&, bool exp);
  static void write_file_header(std::ofstream& ofs, bool isheader);
  static void generate(std::string mod_name, nsmodule_type type, std::filesystem::path source,
                       std::filesystem::path gen);
};

struct nsenum
{
  nsenum_context const& ctx;
  std::string           name;
  std::string           type           = "uint64";
  std::string           namespace_name = "lxe";
  std::string           comment;
  std::string           default_value;
  std::string_view      one;
  nlohmann::json const& jenum;
  nstuple_type_list     tuple_types;
  nsenum_modifier       string_modifier = nsenum_modifier::none;
  nsenum_modifier       search_modifier = nsenum_modifier::none;
  nsenum_usage          usage           = nsenum_usage::as_enum;
  nsenum_entry_list     entries;
  nsenum_entry          default_entry;
  bool                  auto_flags                    = false;
  bool                  string_table                  = true;
  bool                  skip_last_element             = false;
  bool                  auto_default                  = true;
  bool                  suffix_match                  = false;
  bool                  prefix_match                  = false;
  bool                  string_key                    = false;
  bool                  custom_strings                = false;
  bool                  has_enum                      = false;
  bool                  has_flags                     = false;
  bool                  has_consts                    = false;
  bool                  has_auto_default              = false;
  bool                  has_special_default_autoflags = false;

  nsenum(nsenum_context& ctx, nlohmann::json const&);

  inline std::string enum_name() const { return name; }

  static auto sorted_tuple(std::string const& s) { return std::make_pair<int, int>(-(int)s.length(), (int)s.length()); }

  void             parse_associations();
  void             parse_entries();
  std::string_view get_1_shift();
  void             enum_entry(nlohmann::json const&);
  nsenum_usage     default_usage_type() const;
  std::string_view default_value_type() const;
  bool             has_tuples_to_print() const;

  void write_header(std::ostream& ofs) const;
  void write_header_alias(std::ostream& ofs) const;
  void write_header_string_table(std::ostream& ofs) const;
  void write_header_string_key_table(std::ostream& ofs) const;
  void write_header_value_functions(std::ostream& ofs) const;
  void write_header_value_type(std::ostream& ofs) const;
  void write_header_consts(std::ostream& ofs) const;
  void write_header_enum(std::ostream& ofs, bool write_value) const;
  void write_header_flags(std::ostream& ofs) const;
  void write_header_auto_flags(std::ostream& ofs) const;
  void write_source(std::ostream& ofs) const;
  void write_source_to_value(std::ostream& ofs) const;
  void write_source_string_table(std::ostream& ofs) const;
};
