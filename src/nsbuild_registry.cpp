#include "nscmdcommon.h"

// -------------------------------------------------------------------------------------------------------------
ns_cmd_handler(install_dir, build, state, cmd)
{
  build.install_dir = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(build_dir, build, state, cmd)
{
  build.build_dir = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(frameworks_dir, build, state, cmd)
{
  build.frameworks_dir = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(output_dir, build, state, cmd)
{
  build.output_dir = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(compiler, build, state, cmd)
{
  build.compiler.emplace_back();
  auto const& p = cmd.params().value();
  if (p.size() > 0)
  {
    build.compiler.back().filters = get_filters(p[0]);
  }
  return neo::retcode::e_success;
}

ns_cmd_handler(compiler_flags, build, state, cmd)
{
  build.compiler.back().compiler_flags = get_first_concat(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(linker_flags, build, state, cmd)
{
  build.compiler.back().linker_flags = get_first_concat(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(framework, build, state, cmd)
{
  build.frameworks.emplace_back();
  build.frameworks.back().name = build.pwd.parent_path().filename().string();
  return neo::retcode::e_success;
}

ns_cmd_handler(excludes, build, state, cmd)
{
  build.frameworks.back().excludes = get_first_set(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(module, build, state, cmd)
{
  auto& m = build.frameworks.back().modules;
  m.emplace_back();
  build.s_nsmodule       = &m.back();
  build.s_nsmodule->name = build.pwd.parent_path().filename().string();
  return neo::retcode::e_success;
}

ns_cmdend_handler(module, build, state, name) { build.s_nsmodule = nullptr; }

ns_cmd_handler(var, build, state, cmd)
{
  auto& m       = build.s_nsmodule->vars;
  build.s_nsvar = cmd_insert_with_filter(m, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(exports, build, state, cmd)
{
  auto& m       = build.s_nsmodule->exports;
  build.s_nsvar = cmd_insert_with_filter(m, cmd);
  return neo::retcode::e_success;
}

ns_star_handler(var, build, state, cmd)
{
  auto& m = *build.s_nsvar;
  m.params.emplace_back();
  m.params.back().name   = cmd.name();
  m.params.back().params = cmd.params();
  return neo::retcode::e_success;
}

ns_cmd_handler(prebuild, build, state, cmd)
{
  auto& m             = build.s_nsmodule->prebuild;
  build.s_nsbuildstep = cmd_insert(m, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(postbuild, build, state, cmd)
{
  auto& m             = build.s_nsmodule->postbuild;
  build.s_nsbuildstep = cmd_insert(m, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(artifacts, build, state, cmd)
{
  build.s_nsbuildstep->artifacts = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(print, build, state, cmd)
{
  cmd_print(*build.s_nsbuildcmds, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(cmake, build, state, cmd)
{
  cmd_cmake(*build.s_nsbuildcmds, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(trace, build, state, cmd)
{
  cmd_trace(*build.s_nsbuildcmds, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(copy, build, state, cmd)
{
  cmd_copy(*build.s_nsbuildcmds, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(copy_dir, build, state, cmd)
{
  cmd_copy_dir(*build.s_nsbuildcmds, cmd);
  return neo::retcode::e_success;
}

ns_star_handler(any, build, state, cmd)
{
  cmd_any(*build.s_nsbuildcmds, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(fetch, build, state, cmd)
{
  build.s_nsmodule->fetch = std::make_unique<nsfetch>();
  build.s_nsfetch         = build.s_nsmodule->fetch.get();
  return neo::retcode::e_success;
}

ns_cmd_handler(license, build, state, cmd)
{
  build.s_nsfetch->license = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(commit, build, state, cmd)
{
  build.s_nsfetch->commit = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(args, build, state, cmd)
{
  build.s_nsfetch->args = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(config, build, state, cmd)
{
  if (build.s_nsfetch)
    build.s_nsbuildcmds = cmd_insert_with_filter(build.s_nsfetch->config, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(build, build, state, cmd)
{
  if (build.s_nsfetch)
    build.s_nsbuildcmds = cmd_insert_with_filter(build.s_nsfetch->build, cmd);
  return neo::retcode::e_success;
}
ns_cmd_handler(steps, build, state, cmd)
{
  if (build.s_nsbuildstep)
    build.s_nsbuildcmds =
        cmd_insert_with_filter(build.s_nsbuildstep->steps, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(install, build, state, cmd)
{
  if (build.s_nsfetch)
    build.s_nsbuildcmds = cmd_insert_with_filter(build.s_nsfetch->install, cmd);
  else if (build.s_nsmodule)
    build.s_nsbuildcmds =
        cmd_insert_with_filter(build.s_nsmodule->install, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(interface, build, state, cmd)
{
  if (build.s_nsmodule)
    build.s_nsinterface = cmd_insert_with_filter(build.s_nsmodule->intf, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(private, build, state, cmd)
{
  if (build.s_nsmodule)
    build.s_nsinterface = cmd_insert_with_filter(build.s_nsmodule->priv, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(dependencies, build, state, cmd)
{
  auto l = get_first_list(cmd);
  if (build.s_nsbuildstep)
    build.s_nsbuildstep->dependencies = std::move(l);
  else if (build.s_nsinterface)
    build.s_nsinterface->dependencies = std::move(l);
  return neo::retcode::e_success;
}

ns_cmd_handler(libraries, build, state, cmd)
{
  build.s_nsinterface->libraries.emplace_back(
      cmd_parse_nspath(nspath_type::shared_lib, cmd));
  return neo::retcode::e_success;
}

ns_cmd_handler(binaries, build, state, cmd)
{
  build.s_nsinterface->binaries.emplace_back(
      cmd_parse_nspath(nspath_type::binary_list, cmd));
  return neo::retcode::e_success;
}

ns_cmd_handler(includes, build, state, cmd)
{
  build.s_nsinterface->includes.emplace_back(
      cmd_parse_nspath(nspath_type::include_list, cmd));
  return neo::retcode::e_success;
}

ns_cmd_handler(definitions, build, state, cmd)
{
  build.s_nsinterface->definitions = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmdend_handler(fetch, build, state, name) { build.s_nsfetch = nullptr; }

ns_cmdend_handler(clear_buildstep, build, state, cmd)
{
  build.s_nsbuildstep = nullptr;
}

ns_cmdend_handler(clear_buildcmds, build, state, cmd)
{
  build.s_nsbuildcmds = nullptr;
}

ns_cmdend_handler(clear_interface, build, state, cmd)
{
  build.s_nsinterface = nullptr;
}

ns_registry(nsbuild)
{
  neo::command_id prebuild;
  neo::command_id build;
  neo::command_id intf;
  neo::command_id var;

  ns_cmd(install_dir);
  ns_cmd(build_dir);
  ns_cmd(frameworks_dir);
  ns_cmd(output_dir);
  ns_scope_def(compiler)
  {
    ns_cmd(compiler_flags);
    ns_cmd(linker_flags);
  }
  ns_scope_def(framework) { ns_cmd(excludes); }
  ns_scope_auto(module)
  {
    ns_scope_def(var)
    {
      ns_save_scope(var);
      ns_star(var);
    }

    ns_scope_cust(prebuild, clear_buildstep)
    {
      ns_save_scope(prebuild);
      ns_cmd(artifacts);
      ns_cmd(dependencies);

      ns_scope_cust(build, clear_buildcmds)
      {
        ns_save_scope(build);
        ns_cmd(print);
        ns_cmd(trace);
        ns_cmd(cmake);
        ns_cmd(copy);
        ns_cmd(copy_dir);
        ns_star(any);
      }
    }

    ns_subalias_cust(postbuild, clear_buildstep, prebuild);

    ns_scope_auto(fetch)
    {
      ns_cmd(license);
      ns_cmd(commit);
      ns_cmd(args);
      ns_subalias_cust(config, clear_buildstep, build);
      ns_subalias_cust(build, clear_buildstep, build);
      ns_subalias_cust(install, clear_buildstep, build);
    }

    ns_scope_cust(interface, clear_interface)
    {
      ns_save_scope(intf);
      ns_cmd(dependencies);
      ns_cmd(binaries);
      ns_cmd(includes);
      ns_cmd(libraries);
      ns_cmd(definitions);
    }

    ns_subalias_cust(private, clear_interface, intf);
    ns_subalias_def(exports, var);
    ns_subalias_cust(install, clear_buildcmds, build);
  }
}
