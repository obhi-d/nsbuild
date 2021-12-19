#pragma once
#include <fmt/format.h>
#include <iostream>

namespace nslog
{

inline void error(std::string_view sv)
{
  fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::red),
             "\nnsbuild ERROR: {}\n", sv);
}

inline void warn(std::string_view sv)
{
  fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::yellow),
             "\nnsbuild WARN : {}\n", sv);
}

inline void print(std::string_view sv) { fmt::print("\nnsbuild INFO : {}\n", sv); }

} // namespace nslog