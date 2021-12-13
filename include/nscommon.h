#pragma once
#include <bitset>
#include <cstdint>
#include <filesystem>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <future>
#include <nsregistry.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

/// @brief Directory names
static inline constexpr char k_gen_dir[]      = "gen";
static inline constexpr char k_ts_dir[]       = "ts";
static inline constexpr char k_build_dir[]    = "bld";
static inline constexpr char k_subbuild_dir[] = "sbld";
static inline constexpr char k_sdk_dir[]      = "sdk";
static inline constexpr char k_src_dir[]      = "src";
static inline constexpr char k_cmake_dir[]    = "cmk";

enum class output_fmt
{
  cmake_def,
  pipe_list_sep,
  set_cache,
};

enum class nsfilter
{
  debug,
  release,
  windows,
  linux,
  macOS,
  clang,
  msvc,
  gcc,
  asan,
  unknown,
  count
};

nsfilter classify_filter(std::string_view);
using nsfilters = std::bitset<static_cast<unsigned>(nsfilter::count)>;
nsfilters get_filters(neo::command::param_t const& p);

inline std::string_view get_idx_param(neo::command const& cmd, std::size_t i,
                                      std::string_view def = "")
{
  auto const& params = cmd.params();
  auto        size   = params.value().size();
  if (size <= i)
    return def;
  return cmd.as_string(params.value()[i], def);
}

inline std::string get_first_concat(neo::command const& cmd,
                                    std::string         def = "")
{
  auto const& params = cmd.params();
  auto        size   = params.value().size();
  if (!size)
    return def;
  auto const& l = params.value();
  if (size == 1)
    return std::string{cmd.as_string(l[0], def)};
  def = "";
  for (std::size_t i = 0; i < size; ++i)
  {
    def += cmd.as_string(l[i], "");
  }
  return def;
}

inline std::vector<std::string> get_first_list(neo::command const& cmd)
{
  auto const& params = cmd.params();
  auto        size   = params.value().size();
  if (!size)
    return {};
  auto const& l = params.value();
  if (size == 1)
    return {std::string{cmd.as_string(l[0], "")}};
  std::vector<std::string> ret;
  for (std::size_t i = 0; i < size; ++i)
  {
    ret.emplace_back(cmd.as_string(l[i], ""));
  }
  return ret;
}

inline std::unordered_set<std::string> get_first_set(neo::command const& cmd)
{
  auto const& params = cmd.params();
  auto        size   = params.value().size();
  if (!size)
    return {};
  auto const& l = params.value();
  if (size == 1)
    return {std::string{cmd.as_string(l[0], "")}};
  std::unordered_set<std::string> ret;
  for (std::size_t i = 0; i < size; ++i)
  {
    ret.emplace(cmd.as_string(l[i], ""));
  }
  return ret;
}

template <typename Lambda>
void foreach_variable(std::ostream& ostr, std::string_view content, Lambda&& l)
{
  std::size_t r   = 0;
  std::size_t off = 0;
  std::string tmp;
  while (r < content.length())
  {
    auto pos = content.find_first_of('$', off);
    if (pos != content.npos)
    {
      if (pos == content.length() - 1 || content[pos + 1] == '$' ||
          content[pos + 1] == '{')
      {
        off = pos + 1;
        continue;
      }
      else
      {
        ostr << content.substr(r, pos - r);
        pos++;
        if (content[pos] == '(')
          pos++;

        auto len = content.find_first_of(" \n\t)", pos);
        if (len == content.npos)
        {
          r   = content.npos;
          off = r;

          tmp = content.substr(pos);
          std::replace(tmp.begin(), tmp.end(), '.', '_');
          l(ostr, tmp);
        }
        else
        {
          tmp = content.substr(pos, len - pos);
          std::replace(tmp.begin(), tmp.end(), '.', '_');
          l(ostr, tmp);
          if (content[len] == ')')
            len++;
          r   = len;
          off = r;
        }
      }
    }
    else
    {
      ostr << content.substr(r);
    }
  }
}

// framework name/module name
using modid = std::tuple<std::string_view, std::string_view>;

inline std::string target_name(std::string_view fw, std::string_view mod)
{
  return fmt::format("{}.{}", fw, mod);
}

struct nsfilecopy
{
  std::vector<std::string> files;
  std::string              dest;
  bool                     is_dir_copy = false;
};