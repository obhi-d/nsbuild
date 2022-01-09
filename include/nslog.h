#pragma once
#include <fmt/format.h>
#include <iostream>

namespace nslog
{

inline void error(std::string_view sv)
{
  fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::red),
             " -- [ERROR]: {}\n", sv);
}

inline void warn(std::string_view sv)
{
  fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::yellow),
             " -- [WARN ]: {}\n", sv);
}

inline void print(std::string_view sv) { fmt::print(" -- [INFO ]: {}\n", sv); }

} // namespace nslog