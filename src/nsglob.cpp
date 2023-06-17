
#include "picosha2.h"

#include <algorithm>
#include <nscmake.h>
#include <nscmake_conststr.h>
#include <nsglob.h>

void nsglob::print(std::ostream& oss, std::string_view name) const
{
  oss << "\nset(" << name << ")";
  for (auto s : sub_paths)
  {
    for (auto const& f : *file_filters)
      oss << fmt::format("\nlist(APPEND {} \"{}/*{}\")", name, cmake::path(s), f);
  }
  if (recurse)
    oss << "\nfile(GLOB_RECURSE ";
  else
    oss << "\nfile(GLOB ";

  oss << "\n  " << name << " CONFIGURE_DEPENDS ${" << name << "}\n)";
}

void nsglob::print(std::ostream& oss, std::string_view name, std::string_view ctx,
                   std::filesystem::path const& relative_to) const
{
  oss << "\nset(" << name;
  for (auto& sp : sub_paths)
  {
    for (auto const& s : files)
    {
      if (!relative_to.empty())
        oss << "\n\t" << ctx << cmake::path(std::filesystem::relative(s, relative_to));
      else
        oss << "\n\t" << ctx << cmake::path(s);
    }
  }

  oss << "\n)";
}

void nsglob::accumulate()
{
  std::string content;
  for (auto& s : sub_paths)
  {
    if (std::filesystem::exists(s))
    {
      process_directory(files, std::filesystem::directory_entry(s));
    }
  }
  std::ranges::sort(files);
  for (auto const& p : files)
  {
    content += p.string();
    content += "\n";
  }
  picosha2::hash256_hex_string(content, sha);
}

void nsglob::process_directory(file_set& set, std::filesystem::directory_entry const& de)
{
  if (!path_exclude_filters.empty() && path_exclude_filters == de.path().stem().string())
    return;
  auto rdit = std::filesystem::directory_iterator(de);
  for (auto const& it : rdit)
  {
    if (it.is_directory() && recurse)
      process_directory(set, it);
    else
      process_entry(set, it);
  }
}

void nsglob::process_entry(file_set& set, std::filesystem::directory_entry const& de)
{
  if (file_filters && !file_filters->contains(de.path().extension().string()))
    return;
  set.emplace_back(std::filesystem::absolute(de.path()).lexically_normal());
}
