#pragma once

#include <string>
#include <unordered_map>
#include <string_view>
#include <nsvars.h>
#include <sstream>
#include <variant>
#include <utility>

/// @brief Macros do not depend on filters
struct nsmacros
{
  
  using value = std::string;

  std::unordered_map<std::string, value> macros;

  /// @brief Immediate mode print will do immediate mode substitutions
  /// @param ostr Stream
  /// @param content Input content
  /// @param Format options 
  void im_print(std::ostream& ostr, std::string_view content,
               output_fmt = output_fmt::cmake_def) const;

  void print(std::ostream&, output_fmt = output_fmt::cmake_def) const;

  inline value& operator[](std::string const& name) 
  {    
    return macros[name]; 
  }
  
  nsmacros const* fallback = nullptr;
};