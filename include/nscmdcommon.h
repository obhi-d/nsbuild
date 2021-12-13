#pragma once
#include "nsbuild.h"

template <typename T>
T* cmd_insert_with_filter(std::vector<T>& build, neo::command const& cmd)
{
  build.emplace_back();
  auto const& p = cmd.params().value();
  if (p.size() > 0)
  {
    build.back().filters = get_filters(p[0]);
  }
  return &build.back();
}

template <typename T>
T* cmd_insert(std::vector<T>& build, neo::command const& cmd)
{
  build.emplace_back();
  return &build.back();
}

void cmd_parse_nspath(nspath& path, neo::single const& param)
{
  if (param.name() == "path")
  {
    path.path = param.value();
  }
  else if (param.name() == "type")
  {
    path.type = get_nspath_type(param.value());
  }
  else if (param.name() == "")
  {
    path.list.emplace_back(param.value());
  }
}

void cmd_parse_nspath(nspath& path, neo::list const& param)
{
  if (param.name() == "")
  {
    for (auto const& e : param.value())
    {
      auto v = neo::command::as_string(e);
      if (!v.empty())
        path.list.emplace_back(v);
    }
  }
}

nspath cmd_parse_nspath(nspath_type default_type, neo::command const& cmd)
{
  nspath p;
  p.type             = default_type;
  auto const& params = cmd.params().value();
  for (auto const& p : params)
  {
    switch (p.index())
    {
    case neo::command::k_param_list:
    {
      auto const& l = std::get<neo::list>(p);
    }
    break;
    case neo::command::k_param_single:

      break;
    }
  }
  return p;
}

void cmd_print(nsbuildcmds& list, neo::command const& cmd)
{
  list.msgs.emplace_back(cmake::value(cmd.params(), ' '));
}

void cmd_trace(nsbuildcmds& list, neo::command const& cmd)
{
  list.msgs.emplace_back(cmake::value(cmd.params(), ' '));
}

void cmd_cmake(nsbuildcmds& list, neo::command const& cmd)
{
  list.command = "${CMAKE_COMMAND}";
  list.params  = "-E;echo;";
  list.params += cmake::value(cmd.params(), ';');
}

void cmd_copy(nsbuildcmds& list, neo::command const& cmd)
{
  list.command = "${CMAKE_COMMAND}";
  list.params  = "-E;copy;";
  list.params += cmake::value(cmd.params(), ';');
}

void cmd_copy_dir(nsbuildcmds& list, neo::command const& cmd)
{
  list.command = "${CMAKE_COMMAND}";
  list.params  = "-E;copy_directory;";
  list.params += cmake::value(cmd.params(), ';');
}

void cmd_any(nsbuildcmds& list, neo::command const& cmd)
{
  list.command = cmd.name();
  list.params  = cmake::value(cmd.params(), ';');
}
