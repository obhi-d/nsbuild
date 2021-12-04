
#pragma once

#include <nscommon.h>
#include <string_view>
#include <filesystem>

class nspython
{
public:
  void enumspy(std::string_view mname, std::string_view mtype,
               std::string_view src, std::string_view gen);
};
