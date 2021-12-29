
#include <nscmake.h>
#include <nscmake_conststr.h>
#include <nsglob.h>

void nsglob::print(std::ostream& oss) const 
{ 
  oss << "\nset(" << name << ")";
  for (auto s : sub_paths)
  {
    oss << "\nlist(APPEND " << name << " " << fmt::format("\"{}\"", s) << ")";
  }
  if (recurse)
    oss << "\nfile(GLOB_RECURSE ";
  else
    oss << "\nfile(GLOB ";

  oss << "\n  " << name << " CONFIGURE_DEPENDS ${" << name << "}\n)";
  if (relative_to.empty())
    return;

  oss << fmt::format(cmake::k_glob_relative, name, relative_to);
}

