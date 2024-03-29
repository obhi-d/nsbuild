
#include <nsbuild.h>
#include <nsbuildcmds.h>
#include <nscmake.h>
#include <nsmodule.h>

void nsbuildstep::print(std::ostream& ofs, nsbuild const& bc, nsmodule const& m) const
{
  std::string indent = "\n";
  ofs << "\n#  -- build-step -- " << name;
  if (!check.empty())
  {
    ofs << "\nif(" << check << ")";
    indent = "\n  ";
  }

  ofs << indent << injected_config_body;

  ofs << indent << "add_custom_command(";
  indent.push_back(' ');
  indent.push_back(' ');
  ofs << indent << "OUTPUT ";
  indent.push_back(' ');
  indent.push_back(' ');
  for (auto const& o : artifacts)
    ofs << indent << o;
  indent.pop_back();
  indent.pop_back();
  for (auto const& s : steps)
  {
    ofs << indent << "COMMAND " << s.command << " " << s.params;
    for (auto const& m : s.msgs)
      ofs << indent << "COMMAND ${CMAKE_COMMAND} -E echo \"" << m << "\"";
  }
  ofs << indent << "DEPENDS ";
  indent.push_back(' ');
  indent.push_back(' ');
  for (auto const& o : dependencies)
    ofs << indent << o;
  indent.pop_back();
  indent.pop_back();
  if (!wd.empty())
    ofs << indent << "WORKING_DIRECTORY " << wd;
  indent.pop_back();
  ofs << indent << ")" << indent << injected_config_end;

  if (!check.empty())
    ofs << "\nendif()";
}