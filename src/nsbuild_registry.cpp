#include "nscmdcommon.h"

// -------------------------------------------------------------------------------------------------------------
ns_text_handler(custom_cmake, build, state, type, name, content)
{
  if (type == "cmake")
  {
    if (build.frameworks.empty())
      return;
    if (build.frameworks.back().modules.empty())
      return;
    auto& mod = build.frameworks.back().modules.back();
    if (name == "custom_build" && mod.fetch)
    {
      mod.fetch->custom_build = content;
    }
    if (name == "post_build_install" && mod.fetch)
    {
      mod.fetch->custom_build = content;
    }
  }
}

ns_cmd_handler(project_name, build, state, cmd) 
{ 
  build.project_name = get_idx_param(cmd, 0);
  return neo::retcode::e_success; 
}

ns_cmd_handler(meta, build, state, cmd) 
{
  return neo::retcode::e_success;
}

ns_cmd_handler(compiler_version, build, state, cmd)
{
  build.meta.compiler_version = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(compiler_name, build, state, cmd)
{
  build.meta.compiler_name = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(timestamps, build, state, cmd)
{
  return neo::retcode::e_success;
}

ns_cmd_handler(static_libs, build, state, cmd)
{
  build.static_libs = to_bool(get_idx_param(cmd, 0));
  return neo::retcode::e_success;
}

ns_star_handler(timestamps, build, state, cmd)
{
  build.meta.timestamps[std::string{cmd.name()}] = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(version, build, state, cmd)
{
  build.version = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(sdk_dir, build, state, cmd)
{
  build.sdk_dir = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(cmake_gen_dir, build, state, cmd)
{
  build.cmake_gen_dir = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(frameworks_dir, build, state, cmd)
{
  build.frameworks_dir = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(runtime_dir, build, state, cmd)
{
  build.runtime_dir = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(generator, build, state, cmd)
{
  build.preferred_gen = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(preset, build, state, cmd)
{
  build.presets.emplace_back();
  build.s_nspreset = &build.presets.back();
  auto const& p = cmd.params().value();
  if (p.size() > 0)
  {
    build.s_nspreset->name = get_idx_param(cmd, 0);
    build.s_nspreset->display_name = build.s_nspreset->name;
  }
  
  return neo::retcode::e_success;
}

ns_cmd_handler(config, build, state, cmd) 
{
  build.s_nspreset->configs.emplace_back();
  auto const& p = cmd.params().value();
  if (p.size() > 0)
  {
    build.s_nspreset->configs.back().filters = get_filters(p[0]);
  }

  return neo::retcode::e_success;
}

ns_cmd_handler(build_type, build, state, cmd)
{
  build.s_nspreset->build_type = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(display_name, build, state, cmd)
{
  build.s_nspreset->display_name = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(compiler_flags, build, state, cmd)
{
  build.s_nspreset->configs.back().compiler_flags = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(linker_flags, build, state, cmd)
{
  build.s_nspreset->configs.back().linker_flags = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(excludes, build, state, cmd)
{
  build.frameworks.back().excludes = get_first_set(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(type, build, state, cmd)
{
  auto& t    = build.s_nsmodule->type;
  auto  type = get_idx_param(cmd, 0);
  if (type == "plugin")
    t = nsmodule_type::plugin;
  else if (type == "ref")
    t = nsmodule_type::ref;
  else if (type == "data")
    t = nsmodule_type::data;
  else if (type == "extern")
    t = nsmodule_type::external;
  else if (type == "exe")
    t = nsmodule_type::exe;
  else if (type == "lib")
    t = nsmodule_type::lib;
  else if (type == "test")
    t = nsmodule_type::test;
  if (build.state.ras == runas::generate_enum)
    return neo::retcode::e_success_stop;
  return neo::retcode::e_success;
}

ns_cmd_handler(var, build, state, cmd)
{
  auto& m       = build.s_nsmodule->vars;
  build.s_nsvar = cmd_insert_with_filter(m, cmd);
  build.s_nsvar->prefix = "var";
  return neo::retcode::e_success;
}

ns_cmd_handler(exports, build, state, cmd)
{
  auto& m       = build.s_nsmodule->exports;
  build.s_nsvar = cmd_insert_with_filter(m, cmd);
  build.s_nsvar->prefix = "glob";
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

ns_cmd_handler(check_deps, build, state, cmd)
{
  build.s_nsbuildstep->check = get_idx_param(cmd, 0, "");
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
  build.s_nsfetch->repo   = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(runtime_loc, build, state, cmd)
{
  build.s_nsfetch->runtime_loc = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(runtime_files, build, state, cmd)
{
  build.s_nsfetch->runtime_files = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(legacy_linking, build, state, cmd)
{
  build.s_nsfetch->legacy_linking = to_bool(get_idx_param(cmd, 0));
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
  auto& m               = build.s_nsfetch->args;
  build.s_nsvar         = cmd_insert_with_filter(m, cmd);
  
  return neo::retcode::e_success;
}

ns_cmd_handler(package, build, state, cmd)
{
  auto& m       = build.s_nsfetch->package;
  m       = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(components, build, state, cmd)
{
  auto& m = build.s_nsfetch->components;
  m       = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(steps, build, state, cmd)
{
  if (build.s_nsbuildstep)
    build.s_nsbuildcmds =
        cmd_insert(build.s_nsbuildstep->steps, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(public, build, state, cmd)
{
  if (build.s_nsmodule)
    build.s_nsinterface =
        cmd_insert_with_filter(build.s_nsmodule->intf[nsmodule::pub_intf], cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(private, build, state, cmd)
{
  if (build.s_nsmodule)
    build.s_nsinterface = cmd_insert_with_filter(
        build.s_nsmodule->intf[nsmodule::priv_intf], cmd);
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

ns_cmd_handler(references, build, state, cmd)
{
  auto l = get_first_list(cmd);
  if (build.s_nsmodule)
    build.s_nsmodule->references = std::move(l);
  return neo::retcode::e_success;
}

ns_cmd_handler(libraries, build, state, cmd)
{
  build.s_nsinterface->sys_libraries = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(define, build, state, cmd)
{
  auto& def = build.s_nspreset ? build.s_nspreset->definitions : build.s_nsinterface->definitions;
  def.emplace_back(get_idx_param(cmd, 0), get_idx_param(cmd, 1));
  return neo::retcode::e_success;
}

ns_cmd_handler(plugin, build, state, cmd) 
{ return neo::retcode::e_success; }

ns_cmd_handler(description, build, state, cmd) 
{
  if (build.s_nspreset)
    build.s_nspreset->desc = get_idx_param(cmd, 0);
  else if (build.s_nsmodule)
    build.s_nsmodule->manifest.desc = get_idx_param(cmd, 0);
  return neo::retcode::e_success; 
}

ns_cmd_handler(company, build, state, cmd)
{
  build.s_nsmodule->manifest.company = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(author, build, state, cmd)
{
  build.s_nsmodule->manifest.author = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(type_id, build, state, cmd)
{
  build.s_nsmodule->manifest.type = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(compatibility, build, state, cmd)
{
  build.s_nsmodule->manifest.compatibility = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(context, build, state, cmd)
{
  build.s_nsmodule->manifest.context = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(optional, build, state, cmd)
{
  build.s_nsmodule->manifest.optional = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(service, build, state, cmd)
{
  build.s_nsmodule->manifest.services.emplace_back(get_idx_param(cmd, 0), get_idx_param(cmd, 1));
  return neo::retcode::e_success;
}

ns_cmdend_handler(fetch, build, state, name)
{
  build.s_nsfetch = nullptr;
  return neo::retcode::e_success;
}

ns_cmdend_handler(clear_buildstep, build, state, cmd)
{
  build.s_nsbuildstep = nullptr;
  return neo::retcode::e_success;
}

ns_cmdend_handler(clear_buildcmds, build, state, cmd)
{
  build.s_nsbuildcmds = nullptr;
  return neo::retcode::e_success;
}

ns_cmdend_handler(clear_interface, build, state, cmd)
{
  build.s_nsinterface = nullptr;
  return neo::retcode::e_success;
}

ns_cmdend_handler(clear_presets, build, state, cmd)
{
  build.s_nspreset = nullptr;
  return neo::retcode::e_success;
}

ns_registry(nsbuild)
{
  neo::command_id prebuild;
  neo::command_id intf;
  neo::command_id var;

  neo_handle_text(custom_cmake);
  ns_scope_def(meta)
  {
    ns_cmd(compiler_version);
    ns_cmd(compiler_name);
    ns_scope_def(timestamps) 
    { 
      ns_star(timestamps);
    }
  }

  ns_cmd(project_name);
  ns_cmd(version);
  ns_cmd(sdk_dir);
  ns_cmd(cmake_gen_dir);
  ns_cmd(frameworks_dir);
  ns_cmd(runtime_dir);
  ns_cmd(static_libs);
  ns_scope_cust(preset, clear_presets)
  {
    ns_cmd(display_name);
    ns_cmd(description);
    ns_scope_def(config)
    {
      ns_cmd(compiler_flags);
      ns_cmd(linker_flags);
    }
    ns_cmd(define);
    ns_cmd(build_type);
  }
  ns_cmd(excludes);
  ns_cmd(type);
  ns_scope_def(var)
  {
    ns_save_scope(var);
    ns_star(var);
  }

  ns_cmd(references);

  ns_scope_cust(prebuild, clear_buildstep)
  {
    ns_save_scope(prebuild);
    ns_cmd(artifacts);
    ns_cmd(dependencies);

    ns_scope_cust(steps, clear_buildcmds)
    {
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
    ns_scope_def(args) 
    { 
      ns_star(var); 
    }
    ns_cmd(package);
    ns_cmd(components);
    ns_cmd(legacy_linking);
    ns_cmd(runtime_loc);
    ns_cmd(runtime_files);
  }

  ns_scope_cust(public, clear_interface)
  {
    ns_save_scope(intf);
    ns_cmd(dependencies);
    ns_cmd(libraries);
    ns_cmd(define);
  }

  ns_scope_def(plugin) 
  { 
    ns_cmd(description);
    ns_cmd(company);
    ns_cmd(author);
    ns_cmd(type_id);
    ns_cmd(compatibility);
    ns_cmd(context);
    ns_cmd(optional);
    ns_cmd(service);
  }

  ns_subalias_cust(private, clear_interface, intf);
  ns_subalias_def(exports, var);

}

