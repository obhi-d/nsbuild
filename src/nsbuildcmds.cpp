
#include <nsbuild.h>
#include <nsbuildcmds.h>
#include <nsmodule.h>

void nsbuildstep::print(std::ostream& ofs, nsbuild const& bc,
                        nsmodule const& m) const
{
  std::string indent = "\n";
  if (!check.empty())
  {
    ofs << "\nif(" << check << ")";
    indent = "\n\t";
  }

  for (auto const& c : injected_config)
    ofs << indent << c;

  ofs << indent << "add_custom_command(";
  indent.push_back('\t');
  for (auto const& o : artifacts)
    ofs << indent << "OUTPUT " << o;
  for (auto const& s : steps)
  {
    ofs << indent << "COMMAND " << s.command << " " << s.params;
    for (auto const& m : s.msgs)
      ofs << indent << "COMMAND ${CMAKE_COMMAND} -E echo \"" << m << "\"";
  }
  for (auto const& o : dependencies)
    ofs << indent << "DEPENDS " << o;
  if (!wd.empty())
    ofs << indent << "WORKING_DIRECTORY " << wd;
  indent.pop_back();
  ofs << indent << ")";
  if (!check.empty())
    ofs << "\nendif()";
}