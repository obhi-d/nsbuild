#pragma once
#include "nsbuild.h"
#include "nscmake.h"

template <typename T>
T* cmd_insert_with_filter(std::vector<T>& v, nsbuild const& build, neo::command const& cmd)
{
  auto const& p = cmd.params().value();
  v.emplace_back();

  if (p.size() > 0)
  {
    auto filters = cmake::get_filter(*build.s_current_preset, get_filters(p[0]));
    if (!filters.has_value())
    {
      v.pop_back();
      return nullptr;
    }

    v.back().filters = filters.value();
  }

  return &v.back();
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
    case neo::command::param_list:
    {
      auto const& l = std::get<neo::list>(p);
    }
    break;
    case neo::command::param_esq_string:
    case neo::command::param_single:

      break;
    }
  }
  return p;
}

void cmd_print(nsbuildcmds& list, neo::command const& cmd) { list.msgs.emplace_back(cmake::value(cmd.params(), ' ')); }

void cmd_trace(nsbuildcmds& list, neo::command const& cmd) { list.msgs.emplace_back(cmake::value(cmd.params(), ' ')); }

void cmd_cmake(nsbuildcmds& list, neo::command const& cmd)
{
  list.command = "${CMAKE_COMMAND}";
  list.params  = cmake::value(cmd.params(), ' ');
}

void cmd_copy(nsbuildcmds& list, neo::command const& cmd)
{
  list.command = "${CMAKE_COMMAND}";
  list.params  = " -E copy ";
  list.params += cmake::value(cmd.params(), ' ');
}

void cmd_copy_dir(nsbuildcmds& list, neo::command const& cmd)
{
  list.command = "${CMAKE_COMMAND}";
  list.params  = " -E copy_directory ";
  list.params += cmake::value(cmd.params(), ' ');
}

void cmd_python(nsbuildcmds& list, neo::command const& cmd)
{
  list.command = "${Python_EXECUTABLE}";
  list.params += cmake::value(cmd.params(), ' ');
}

void cmd_any(nsbuildcmds& list, neo::command const& cmd)
{
  list.command = cmd.name();
  list.params  = cmake::value(cmd.params(), ' ');
}

void cmd_run(nsbuildcmds& list, neo::command const& cmd)
{
  auto it  = cmd.params().value().begin();
  auto end = cmd.params().value().end();
  if (it != end)
  {
    cmake::value(list.command, it, it + 1, ' ');
    it++;
    if (it != end)
      cmake::value(list.params, it, end, ' ');
  }
}
