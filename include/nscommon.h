#pragma once 
#include <nsregistry.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <bitset>

enum class nsfilter
{
  debug,
  release,
  windows,
  linux,
  osx,
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
                           std::string def = "")
{
  auto const& params = cmd.params();
  auto        size   = params.value().size();
  if (!size)
    return def;
  auto const& l = params.value();
  if (size == 1)
    return std::string { cmd.as_string(l[0], def) };
  def = "";
  for (std::size_t i = 0; i < size; ++i)
  {
    def += cmd.as_string(l[i], "");
  }
  return def;
}

inline std::vector<std::string_view> get_first_list(neo::command const& cmd)
{
  auto const& params = cmd.params();
  auto        size   = params.value().size();
  if (!size)
    return {};
  auto const& l = params.value();
  if (size == 1)
    return {std::string_view{cmd.as_string(l[0], "")}};
  std::vector<std::string_view> ret;
  for (std::size_t i = 0; i < size; ++i)
  {
     ret.emplace_back(std::move(cmd.as_string(l[i], "")));
  }
  return ret;
}

inline std::unordered_set<std::string_view> get_first_set(neo::command const& cmd)
{
  auto const& params = cmd.params();
  auto        size   = params.value().size();
  if (!size)
    return {};
  auto const& l = params.value();
  if (size == 1)
    return {std::string_view{cmd.as_string(l[0], "")}};
  std::unordered_set<std::string_view> ret;
  for (std::size_t i = 0; i < size; ++i)
  {
    ret.emplace(std::move(cmd.as_string(l[i], "")));
  }
  return ret;
}
