
#include <nscommon.h>
#include <string_view>

struct nsglob
{
  std::string_view name;
  std::vector<std::string_view> sub_paths;

  void print(std::ostream&);
};