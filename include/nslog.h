#pragma once
#include <fmt/color.h>
#include <fmt/format.h>
#include <iostream>

namespace nslog
{

inline void error(std::string_view sv)
{
  fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::red), " -- [!!!] {}\n", sv);
}

inline void warn(std::string_view sv)
{
  fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::yellow), " -- [ ! ] {}\n", sv);
}

inline void print(std::string_view sv) { fmt::print(" -- [ + ] {}\n", sv); }

} // namespace nslog
