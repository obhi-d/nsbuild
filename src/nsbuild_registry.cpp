#include "nscmdcommon.h"

#include <fstream>

void halt();

nsmodule_type get_type(std::string_view type)
{
  if (type == "plugin")
    return nsmodule_type::plugin;
  else if (type == "ref")
    return nsmodule_type::ref;
  else if (type == "data")
    return nsmodule_type::data;
  else if (type == "extern")
    return nsmodule_type::external;
  else if (type == "exe")
    return nsmodule_type::exe;
  else if (type == "lib")
    return nsmodule_type::lib;
  else if (type == "shared_lib")
    return nsmodule_type::shared_lib;
  else if (type == "test")
    return nsmodule_type::test;
  return nsmodule_type::none;
}

void append(neo::text_content& dst, neo::text_content const& src)
{
  for (auto const& s : src.fragments)
    dst.fragments.emplace_back(s);
}

/**
 * @brief trim leading white space
 */
inline auto trim_leading(std::string_view str)
{
  size_t endpos = str.find_first_not_of(" \t\n\r");
  if (endpos != 0)
    str = str.substr(endpos);
  return str;
}

/**
 * @brief trim trailing white space
 */
inline auto trim_trailing(std::string_view str)
{
  size_t endpos = str.find_last_not_of(" \t\n\r");
  if (endpos != std::string::npos)
    str = str.substr(0, endpos + 1);
  return str;
}

inline std::string_view trim(std::string_view str)
{
  str = trim_leading(str);
  str = trim_trailing(str);
  return str;
}

void collect_content(neo::text_content const& src, std::filesystem::path root, ns_embed_content& content)
{
  for (auto f : src.fragments)
  {
    std::string_view name;
    std::string_view value;
    size_t           pos = 0;
    while (pos < f.size())
    {
      auto next = f.find_first_of("\n=", pos);
      if (next == 0)
        pos = next + 1;
      else if (next != f.npos)
      {
        if (f[next] == '=')
        {
          name = f.substr(pos, next - pos);
          pos  = next + 1;
          continue;
        }
        else
        {
          value = f.substr(pos, next - pos);
          if (name.empty())
          {
            auto basename = value.find_last_of('/');
            if (basename == value.npos)
              name = value;
            else
              name = value.substr(basename + 1);
          }

          if (!name.empty() && !value.empty())
          {
            name      = trim(name);
            auto file = std::ifstream(root / trim(value), std::ios::binary);
            if (file.is_open())
            {
              content.emplace_back(
                  name, value, std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()));
            }
          }
          name  = {};
          value = {};
          pos   = next + 1;
        }
      }
      else
        break;
    }
  }
}

bool add(nsmodule& mod, auto from, neo::text_content const& content, std::string_view name, std::string_view query)
{
  if (name.starts_with(query))
  {
    auto nof = name.find_first_of('-');
    if (nof != name.npos)
    {
      auto ft = mod.find_fetch(name.substr(nof + 1));
      if (ft)
        append(ft->*from, content);
    }
    else
    {
      for (auto& ft : mod.fetch)
        append(ft.*from, content);
    }
    return true;
  }
  return false;
}

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
    if (add(mod, &nsfetch::finalize, content, name, "finalize"))
    {
    }
    else if (add(mod, &nsfetch::prepare, content, name, "prepare"))
    {
    }
    else if (add(mod, &nsfetch::include, content, name, "include"))
    {
    }
  }
  else if (type == "content")
  {
    auto& mod = build.frameworks.back().modules.back();
    mod.contents.emplace_back();
    mod.contents.back().name = name;
    content.append_to(mod.contents.back().content);
  }
  else if (type == "embed")
  {
    auto& mod = build.frameworks.back().modules.back();
    // Embed the files in the list
    if (name == "binary" || name == "text")
      collect_content(content, build.s_nsmodule->location, mod.embedded_binary_files);
    else if (name == "base64")
      collect_content(content, build.s_nsmodule->location, mod.embedded_base64_files);
  }
}

ns_cmd_handler(style, build, state, cmd)
{
  auto val = get_idx_param(cmd, 0);
  if (val == "upper_camel_case")
    build.style = coding_style::UpperCamelCase;
  else if (val == "lower_camel_case")
    build.style = coding_style::LowerCamelCase;
  else if (val == "snake_case")
    build.style = coding_style::SnakeCase;
  return neo::retcode::e_success;
}

ns_cmd_handler(project_name, build, state, cmd)
{
  build.project_name = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(macro_prefix, build, state, cmd)
{
  build.macro_prefix = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(file_prefix, build, state, cmd)
{
  build.file_prefix = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(verbose, build, state, cmd)
{
  build.verbose = to_bool(get_idx_param(cmd, 0));
  return neo::retcode::e_success;
}

ns_cmd_handler(has_fmtlib, build, state, cmd)
{
  build.has_fmtlib = to_bool(get_idx_param(cmd, 0));
  return neo::retcode::e_success;
}

ns_cmd_handler(macro, build, state, cmd)
{
  auto name                       = get_idx_param(cmd, 0);
  auto value                      = get_idx_param(cmd, 1);
  build.macros[std::string{name}] = value;
  return neo::retcode::e_success;
}

ns_cmd_handler(plugin_dir, build, state, cmd)
{
  build.plugin_dir = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(media_name, build, state, cmd)
{
  build.media_name = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(media_exclude_filter, build, state, cmd)
{
  build.media_exclude_filter = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(natvis, build, state, cmd)
{
  build.natvis = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(meta, build, state, cmd) { return neo::retcode::e_success; }

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

ns_cmd_handler(timestamps, build, state, cmd) { return neo::retcode::e_success; }

ns_cmd_handler(platform, build, state, cmd)
{
  build.s_nspreset->platform = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(glob_sources, build, state, cmd)
{
  build.s_nspreset->glob_sources = to_bool(get_idx_param(cmd, 0));
  return neo::retcode::e_success;
}

ns_cmd_handler(unity_build, build, state, cmd)
{
  build.s_nspreset->unity_build = to_bool(get_idx_param(cmd, 0));
  return neo::retcode::e_success;
}

ns_cmd_handler(static_libs, build, state, cmd)
{
  build.s_nspreset->static_libs = to_bool(get_idx_param(cmd, 0));
  return neo::retcode::e_success;
}

ns_cmd_handler(static_plugins, build, state, cmd)
{
  build.s_nspreset->static_plugins = to_bool(get_idx_param(cmd, 0));
  return neo::retcode::e_success;
}

ns_cmd_handler(cppcheck, build, state, cmd)
{
  build.s_nspreset->cppcheck = to_bool(get_idx_param(cmd, 0));
  return neo::retcode::e_success;
}

ns_cmd_handler(cppcheck_suppression, build, state, cmd)
{
  build.s_nspreset->cppcheck_suppression = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_star_handler(timestamps, build, state, cmd) { return neo::retcode::e_success; }

ns_cmd_handler(version, build, state, cmd)
{
  if (build.s_nsfetch)
    build.s_nsfetch->version = get_idx_param(cmd, 0);
  else if (build.s_nsmodule)
    build.s_nsmodule->version = get_idx_param(cmd, 0);
  else
    build.version = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(sdk_dir, build, state, cmd)
{
  build.sdk_dir = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(out_dir, build, state, cmd)
{
  build.out_dir = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(build_dir, build, state, cmd)
{
  build.build_dir = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(download_dir, build, state, cmd)
{
  build.download_dir = get_idx_param(cmd, 0);
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

ns_cmd_handler(plugin_registration, build, state, cmd)
{
  build.plugin_registration = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(plugin_entry, build, state, cmd)
{
  build.plugin_entry = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(preset, build, state, cmd)
{
  build.presets.emplace_back();
  build.s_nspreset = &build.presets.back();
  auto const& p    = cmd.params().value();
  if (p.size() > 0)
  {
    build.s_nspreset->name         = get_idx_param(cmd, 0);
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

ns_cmd_handler(naming, build, state, cmd)
{
  build.s_nspreset->naming = get_idx_param(cmd, 0);
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
  build.s_nsmodule->type = get_type(get_idx_param(cmd, 0));
  if (build.state.ras == runas::generate_enum)
    return neo::retcode::e_success_stop;
  return neo::retcode::e_success;
}

ns_cmd_handler(source_paths, build, state, cmd)
{
  build.s_nsmodule->source_sub_dirs = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(source_files, build, state, cmd)
{
  build.s_nsmodule->source_files = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(name, build, state, cmd)
{
  if (build.s_nsfetch)
    build.s_nsfetch->name = get_idx_param(cmd, 0);
  else
    build.s_nsmodule->custom_target_name = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(org_name, build, state, cmd)
{
  build.s_nsmodule->org_name = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(console_app, build, state, cmd)
{
  build.s_nsmodule->console_app = to_bool(get_idx_param(cmd, 0));
  return neo::retcode::e_success;
}

ns_cmd_handler(exclude_when, build, state, cmd)
{
  auto const& p = cmd.params().value();
  if (p.size() > 0)
  {
    auto filters = cmake::get_filter(*build.s_current_preset, get_filters(p[0]));
    if (filters.has_value())
      build.s_nsmodule->disabled = true;
  }
  return neo::retcode::e_success;
}

ns_cmd_handler(include_when, build, state, cmd)
{
  auto const& p = cmd.params().value();
  if (p.size() > 0)
  {
    auto filters = cmake::get_filter(*build.s_current_preset, get_filters(p[0]));
    if (!filters.has_value())
      build.s_nsmodule->disabled = true;
  }
  return neo::retcode::e_success;
}

ns_cmd_handler(var, build, state, cmd)
{
  auto& m       = build.s_nsmodule->vars;
  build.s_nsvar = cmd_insert_with_filter(m, build, cmd);
  if (!build.s_nsvar)
    return neo::retcode::e_skip_block;
  build.s_nsvar->prefix = "var";
  return neo::retcode::e_success;
}

ns_cmd_handler(exports, build, state, cmd)
{
  auto& m       = build.s_nsmodule->exports;
  build.s_nsvar = cmd_insert_with_filter(m, build, cmd);
  if (!build.s_nsvar)
    return neo::retcode::e_skip_block;
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
  auto& m             = build.s_nsmodule ? build.s_nsmodule->prebuild : build.s_nspreset->prebuild;
  build.s_nsbuildstep = cmd_insert(m, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(postbuild, build, state, cmd)
{
  auto& m             = build.s_nsmodule ? build.s_nsmodule->postbuild : build.s_nspreset->postbuild;
  build.s_nsbuildstep = cmd_insert(m, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(module_filter, build, state, cmd)
{
  uint32_t i = 0;
  while (true)
  {
    auto const filter = get_idx_param(cmd, i++);
    if (filter.empty())
      break;
    auto type = get_type(filter);
    if (type != nsmodule_type::none)
      build.s_nsbuildstep->module_filter |= (1u << (uint32_t)type);
  }
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

ns_cmd_handler(python, build, state, cmd)
{
  cmd_python(*build.s_nsbuildcmds, cmd);
  return neo::retcode::e_success;
}

ns_star_handler(any, build, state, cmd)
{
  cmd_any(*build.s_nsbuildcmds, cmd);
  return neo::retcode::e_success;
}

ns_star_handler(run, build, state, cmd)
{
  cmd_run(*build.s_nsbuildcmds, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(fetch, build, state, cmd)
{
  build.s_nsfetch = cmd_insert_with_filter(build.s_nsmodule->fetch, build, cmd);
  if (!build.s_nsfetch)
    return neo::retcode::e_skip_block;
  return neo::retcode::e_success;
}

ns_cmd_handler(repo, build, state, cmd)
{
  build.s_nsfetch->repo = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(source, build, state, cmd)
{
  build.s_nsfetch->source = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(namespace, build, state, cmd)
{
  if (build.s_nsfetch)
    build.s_nsfetch->namespace_name = get_idx_param(cmd, 0);
  else
    build.namespace_name = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(skip_namespace, build, state, cmd)
{
  build.s_nsfetch->skip_namespace = to_bool(get_idx_param(cmd, 0));
  return neo::retcode::e_success;
}

ns_cmd_handler(extern_name, build, state, cmd)
{
  build.s_nsfetch->extern_name = get_idx_param(cmd, 0);
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

ns_cmd_handler(force_build, build, state, cmd)
{
  build.s_nsfetch->force_build = to_bool(get_idx_param(cmd, 0));
  return neo::retcode::e_success;
}

ns_cmd_handler(force_download, build, state, cmd)
{
  build.s_nsfetch->force_download = to_bool(get_idx_param(cmd, 0));
  return neo::retcode::e_success;
}

ns_cmd_handler(force, build, state, cmd)
{
  build.s_nsfetch->force_download = build.s_nsfetch->force_build = to_bool(get_idx_param(cmd, 0));
  return neo::retcode::e_success;
}

ns_cmd_handler(tag, build, state, cmd)
{
  if (build.s_nsfetch)
    build.s_nsfetch->tag = get_idx_param(cmd, 0);
  else if (build.s_nsmodule)
    build.s_nsmodule->tags += get_idx_param(cmd, 0);
  else if (build.s_nspreset)
    build.s_nspreset->tags += get_idx_param(cmd, 0);

  return neo::retcode::e_success;
}

ns_cmd_handler(test_tag, build, state, cmd)
{
  build.test_tags += get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(args, build, state, cmd)
{
  auto& m       = build.s_nsfetch->args;
  build.s_nsvar = cmd_insert_with_filter(m, build, cmd);
  if (!build.s_nsvar)
    return neo::retcode::e_skip_block;
  return neo::retcode::e_success;
}

ns_cmd_handler(package, build, state, cmd)
{
  auto& m = build.s_nsfetch->package;
  m       = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(components, build, state, cmd)
{
  auto& m = build.s_nsfetch->components;
  m       = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(targets, build, state, cmd)
{
  auto& m = build.s_nsfetch->targets;
  m       = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(steps, build, state, cmd)
{
  if (build.s_nsbuildstep)
    build.s_nsbuildcmds = cmd_insert(build.s_nsbuildstep->steps, cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(public, build, state, cmd)
{
  if (build.s_nsmodule)
    build.s_nsinterface = cmd_insert_with_filter(build.s_nsmodule->intf[nsmodule::pub_intf], build, cmd);
  if (!build.s_nsinterface)
    return neo::retcode::e_skip_block;
  return neo::retcode::e_success;
}

ns_cmd_handler(private, build, state, cmd)
{
  if (build.s_nsmodule)
    build.s_nsinterface = cmd_insert_with_filter(build.s_nsmodule->intf[nsmodule::priv_intf], build, cmd);
  if (!build.s_nsinterface)
    return neo::retcode::e_skip_block;
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

ns_cmd_handler(required_plugins, build, state, cmd)
{
  auto l = get_first_list(cmd);
  if (build.s_nsmodule)
    build.s_nsmodule->required_plugins = std::move(l);
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

ns_cmd_handler(plugin, build, state, cmd) { return neo::retcode::e_success; }

ns_cmd_handler(description, build, state, cmd)
{
  if (build.s_nspreset)
    build.s_nspreset->desc = get_idx_param(cmd, 0);
  return neo::retcode::e_success;
}

ns_cmd_handler(allow, build, state, cmd)
{
  if (build.s_nspreset)
    build.s_nspreset->allowed_filters = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmd_handler(disallow, build, state, cmd)
{
  if (build.s_nspreset)
    build.s_nspreset->disallowed_filters = get_first_list(cmd);
  return neo::retcode::e_success;
}

ns_cmdend_handler(fetch, build, state, name)
{
  if (build.s_nsfetch)
  {
    if (build.s_nsfetch->name.empty())
      build.s_nsfetch->name = build.s_nsmodule->name;
    else
      build.s_nsfetch->name = fmt::format("{}_{}", build.s_nsmodule->name, build.s_nsfetch->name);
    if (build.s_nsfetch->version.empty())
      build.s_nsfetch->version = build.s_nsmodule->version;
    if (build.s_nsfetch->extern_name.empty())
      build.s_nsfetch->extern_name = build.s_nsfetch->package;
    if (build.s_nsfetch->targets.empty())
      build.s_nsfetch->targets = build.s_nsfetch->components;
  }
  build.s_nsfetch = nullptr;
  return neo::retcode::e_success;
}

ns_cmd_handler(test_namespace, build, state, cmd)
{
  build.s_nstestNamespace = get_idx_param(cmd, 0);
  auto list               = get_list_at(1, cmd);
  bool first              = true;
  for (auto const& l : list)
  {
    if (!first)
      build.s_nstestNamespaceTags += ";";
    build.s_nstestNamespaceTags += l;
    first = false;
  }
  return neo::retcode::e_success;
}

ns_cmd_handler(test_class, build, state, cmd)
{
  build.s_nstestClass = get_idx_param(cmd, 0);
  auto list           = get_list_at(1, cmd);
  bool first          = true;
  for (auto const& l : list)
  {
    if (!first)
      build.s_nstestClassTags += ";";
    build.s_nstestClassTags += l;
    first = false;
  }
  return neo::retcode::e_success;
}

ns_cmd_handler(test_name, build, state, cmd)
{
  build.s_nsmodule->tests.emplace_back();

  nstest&          test              = build.s_nsmodule->tests.back();
  auto             name              = get_idx_param(cmd, 0);
  std::string_view testName          = name == "default" ? "" : name;
  bool             pushTestName      = name != "default";
  bool             pushClassName     = build.s_nstestClass != "any" || pushTestName;
  bool             pushNamespaceName = build.s_nstestNamespace != "any" || pushClassName;
  test.name                          = build.s_nsmodule->name;
  if (pushNamespaceName)
    fmt::format_to(std::back_inserter(test.name), ".{}", build.s_nstestNamespace);
  if (pushClassName)
    fmt::format_to(std::back_inserter(test.name), ".{}", build.s_nstestClass);
  if (pushTestName)
    fmt::format_to(std::back_inserter(test.name), ".{}", name);
  test.test_param_name = fmt::format("{}.{}.{}", build.s_nstestNamespace, build.s_nstestClass, name);
  test.tags            = build.s_nstestNamespaceTags;
  if (!build.s_nstestClassTags.empty())
  {
    if (!test.tags.empty())
      test.tags += ";";
    test.tags += build.s_nstestClassTags;
  }
  auto const& params = cmd.params().value();
  for (auto const& p : params)
  {

    std::visit(neo::overloaded{[](std::monostate const&) {},
                               [&test](neo::list const& l)
                               { test.parameters.emplace_back(l.name(), neo::command::as_string(l)); },
                               [&test](neo::esq_string const& l) { test.parameters.emplace_back(l.name(), l.value()); },
                               [&test](neo::single const& l) { test.parameters.emplace_back(l.name(), l.value()); }},
               p);
  }
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

ns_cmdend_handler(clear_testns, build, state, cmd)
{
  build.s_nstestNamespaceTags.clear();
  return neo::retcode::e_success;
}

ns_cmdend_handler(clear_test, build, state, cmd)
{
  build.s_nstestClassTags.clear();
  return neo::retcode::e_success;
}

ns_registry(nsbuild)
{
  neo::command_id buildsteps;
  neo::command_id intf;
  neo::command_id var;

  neo_handle_text(custom_cmake);
  ns_scope_def(meta)
  {
    ns_cmd(compiler_version);
    ns_cmd(compiler_name);
    ns_scope_def(timestamps) { ns_star(timestamps); }
  }

  ns_cmd(project_name);
  ns_cmd(namespace);
  ns_cmd(macro_prefix);
  ns_cmd(file_prefix);
  ns_cmd(version);
  ns_cmd(verbose);
  ns_cmd(has_fmtlib);
  ns_cmd(sdk_dir);
  ns_cmd(cmake_gen_dir);
  ns_cmd(frameworks_dir);
  ns_cmd(runtime_dir);
  ns_cmd(natvis);
  ns_cmd(style);
  ns_cmd(out_dir);
  ns_cmd(build_dir);
  ns_cmd(download_dir);
  ns_cmd(macro);
  ns_cmd(plugin_dir);
  ns_cmd(media_name);
  ns_cmd(media_exclude_filter);
  ns_cmd(test_tag);
  ns_cmd(plugin_registration);
  ns_cmd(plugin_entry);

  ns_scope_cust(preset, clear_presets)
  {
    ns_cmd(display_name);
    ns_cmd(description);
    ns_cmd(allow);
    ns_cmd(glob_sources);
    ns_cmd(disallow);
    ns_cmd(cppcheck);
    ns_cmd(cppcheck_suppression);
    ns_cmd(unity_build);
    ns_cmd(naming);
    ns_cmd(tag);
    ns_cmd(platform);
    ns_scope_def(config)
    {
      ns_cmd(compiler_flags);
      ns_cmd(linker_flags);
    }
    ns_cmd(define);
    ns_cmd(build_type);
    ns_cmd(static_libs);
    ns_cmd(static_plugins);

    ns_scope_cust(prebuild, clear_buildstep)
    {
      ns_save_scope(buildsteps);
      ns_cmd(artifacts);
      ns_cmd(dependencies);
      ns_cmd(module_filter);

      ns_scope_cust(steps, clear_buildcmds)
      {
        ns_cmd(print);
        ns_cmd(trace);
        ns_cmd(cmake);
        ns_cmd(copy);
        ns_cmd(copy_dir);
        ns_cmd(python);
        ns_star(any);
        ns_star(run);
      }
    }

    ns_subalias_cust(postbuild, clear_buildstep, buildsteps);
  }

  ns_cmd(excludes);
  ns_cmd(type);
  ns_cmd(include_when);
  ns_cmd(exclude_when);
  ns_cmd(console_app);
  ns_cmd(source_paths);
  ns_cmd(name);
  ns_cmd(org_name);
  ns_cmd(source_files);

  ns_scope_def(var)
  {
    ns_save_scope(var);
    ns_star(var);
  }

  ns_cmd(references);
  ns_cmd(required_plugins);

  ns_subalias_cust(prebuild, clear_buildstep, buildsteps);
  ns_subalias_cust(postbuild, clear_buildstep, buildsteps);

  ns_scope_cust(test_namespace, clear_testns)
  {
    ns_scope_cust(test_class, clear_test) { ns_cmd(test_name); }
  }

  ns_scope_auto(fetch)
  {
    ns_cmd(license);
    ns_cmd(force);
    ns_cmd(source);
    ns_cmd(force_build);
    ns_cmd(force_download);
    ns_cmd(tag);
    ns_scope_def(args) { ns_star(var); }
    ns_cmd(package);
    ns_cmd(components);
    ns_cmd(targets);
    ns_cmd(legacy_linking);
    ns_cmd(runtime_loc);
    ns_cmd(runtime_files);
    ns_cmd(extern_name);
    ns_cmd(namespace);
    ns_cmd(skip_namespace);
    ns_cmd(repo);
    ns_cmd(name);
    ns_cmd(version);
  }

  ns_scope_cust(public, clear_interface)
  {
    ns_save_scope(intf);
    ns_cmd(dependencies);
    ns_cmd(libraries);
    ns_cmd(define);
  }

  ns_subalias_cust(private, clear_interface, intf);
  ns_subalias_def(exports, var);
}
