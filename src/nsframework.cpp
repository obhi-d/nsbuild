
#include <nsbuild.h>
#include <nsframework.h>

void nsframework::process(nsbuild const& bc) 
{
  auto  pwd   = std::filesystem::current_path();
  auto fwpwd  = bc.get_full_source_dir() / bc.frameworks_dir / name;
  source_path = fwpwd.generic_string();
  
}
