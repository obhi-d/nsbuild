#pragma once
#include <nscommon.h>

using nsparams = neo::command::parameters;
struct nsnamed_params
{
  std::string_view name;
  nsparams         params;
};

enum class nspath_type
{
  shared_lib,
  static_lib,
  system_lib,
  include_list,
  binary_list,
  dependency_list,
  unknown
};

struct nspath
{
  nspath_type                   type;
  std::string_view              path;
  std::vector<std::string_view> list;
};

using nspath_list = std::vector<nspath>;

struct nsvars
{
  nsfilters                   filters;
  std::vector<nsnamed_params> params;

  void print(std::ostream& os, output_fmt = output_fmt::cmake_def) const;
};

nspath_type get_nspath_type(std::string_view);

namespace cmake
{
std::string      get_filter(nsfilters);
std::string_view get_filter(nsfilter);
std::string      value(nsparams const&, char seperator = ';');
} // namespace cmake