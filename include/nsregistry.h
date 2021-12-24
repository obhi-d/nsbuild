#pragma once
#include <neo_script.hpp>

struct nsbuild;
#define ns_cmd_handler(FnName, iObj, iState, iCmd)                             \
  neo_cmd_handler(FnName, nsbuild, iObj, iState, iCmd)

#define ns_cmdend_handler(FnName, iObj, iState, iName)                         \
  neo_cmdend_handler(FnName, nsbuild, iObj, iState, iCmd)

#define ns_text_handler(FnName, iObj, iState, iType, iName, iContent)          \
  neo_text_handler(FnName, nsbuild, iObj, iState, iType, iName, iContent)

#define ns_star_handler(FnName, iObj, iState, iCmd)                            \
  neo_star_handler(FnName, nsbuild, iObj, iState, iCmd)

#define ns_registry(name) neo_registry(name)
#define ns_register(name) neo_register(name)

#define ns_star(name)                   neo_star(name)
#define ns_cmd(name)                    neo_cmd(name)
#define ns_scope_def(name)              neo_scope_def(name)
#define ns_scope_auto(name)             neo_scope_auto(name)
#define ns_scope_cust(name, end)        neo_scope_cust(name, end)
#define ns_aliasid(par_scope, name, ex) neo_aliasid(par_scope, name, ex)
#define ns_subalias_cust(name, end, alias)   neo_subalias_cust(name, end, alias)
#define ns_subalias_def(name, alias) neo_subalias_def(name, alias)

#define ns_save_current(as) neo_save_current(as)
#define ns_save_scope(as)   neo_save_scope(as)
#define ns_fn(name)         neo_fn(name)

