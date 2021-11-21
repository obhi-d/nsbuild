#pragma once
#include <nsregistry.h>
#include <string>

enum class help_type
{
  full,
  command,
  list_subsection
};
class nshelp : public neo::command_handler
{
public:
  nshelp(std::ostream& o, help_type t, std::string a) : type(t), arg(a), out(o) {}

  static neo::retcode print_enter_scope(neo::command_handler* this_,
    neo::state_machine const& ctx,
    neo::command const& cmd) noexcept {}

  static void leave_scope(neo::command_handler* this_,
    neo::state_machine const& ctx,
    std::string_view          name) noexcept {}

  static neo::retcode print_command(neo::command_handler* _,
    neo::state_machine const& ctx,
    neo::command const& cmd) {}

  std::ostream& out;
  help_type type;
  std::string arg;
};