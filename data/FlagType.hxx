
#pragma once

#include "BuildConfig.hpp"

#include <bit>
#include <compare>
#include <cstdint>
#include <initializer_list>
#include <type_traits>

// clang-format off
namespace @default_namespace_name@ // namespace
{
// clang-format on

template <typename T> concept EnumType = std::is_enum_v<T>;

//! Use this class only and only when you want to convert an Enum into flags
//! If the enum already has flag bits representation, use
//! L_DECLARE_MASK_FLAGS instead
template <EnumType Enum>
class EnumFlags
{
public:
  using StorageType = std::make_unsigned_t<std::underlying_type_t<Enum>>;

  template <EnumType Enum2>
  static constexpr inline StorageType toFlag(Enum2 enumInput)
  {
    return static_cast<StorageType>(1) << static_cast<StorageType>(enumInput);
  }

  static constexpr inline StorageType toFlag(StorageType enumInput) { return enumInput; }

  template <EnumType Enum2>
  static constexpr inline StorageType toFlag(std::initializer_list<Enum2> iValues)
  {
    StorageType result = 0;
    for (auto a : iValues)
      result |= toFlag(a);
    return result;
  }

  static constexpr inline StorageType toFlag(std::initializer_list<StorageType> iValues)
  {
    StorageType result = 0;
    for (auto a : iValues)
      result |= a;
    return result;
  }

public:
  inline constexpr EnumFlags() noexcept : value(0) {}
  inline constexpr EnumFlags(EnumFlags const& iOther) noexcept : value(iOther.value) {}

  inline constexpr explicit EnumFlags(StorageType iValue) noexcept : value(iValue) {}
  template <EnumType Enum2>
  inline constexpr EnumFlags(EnumFlags<Enum2> const& iOther) noexcept : value(iOther.value)
  {}
  inline constexpr EnumFlags(std::true_type, Enum iValue) noexcept : value(toFlag(iValue)) {}

  inline constexpr EnumFlags(std::initializer_list<Enum> iValues) noexcept : value(toFlag(iValues)) {}
  inline constexpr EnumFlags(std::initializer_list<StorageType> iValues) noexcept : value(toFlag(iValues)) {}

  inline constexpr operator std::uint64_t() const noexcept { return (std::uint64_t)value; }

  constexpr auto& operator=(std::uint64_t iVal) noexcept
  {
    value = (StorageType)iVal;
    return *this;
  }

  template <typename Enum2>
  constexpr inline EnumFlags& operator+=(Enum2 iValue) noexcept
  {
    value |= toFlag(iValue);
    return *this;
  }

  template <typename Enum2>
  constexpr inline EnumFlags& operator-=(Enum2 iValue) noexcept
  {
    value &= ~toFlag(iValue);
    return *this;
  }

  constexpr inline EnumFlags& operator|=(EnumFlags iValue) noexcept
  {
    value |= iValue.value;
    return *this;
  }

  template <typename Enum2>
  constexpr inline EnumFlags& operator|=(Enum2 iValue) noexcept
  {
    value |= toFlag(iValue);
    return *this;
  }

  template <typename Enum2>
  inline constexpr EnumFlags operator|(Enum2 iValue) const noexcept
  {
    EnumFlags f(*this);
    f |= iValue;
    return f;
  }
  inline constexpr EnumFlags operator|(EnumFlags iValue) const noexcept { return (EnumFlags)(value | iValue.value); }
  inline constexpr EnumFlags operator&(EnumFlags iValue) const noexcept { return (EnumFlags)(value & iValue.value); }
  inline constexpr EnumFlags operator~() const noexcept { return (EnumFlags)(~value); }
  inline constexpr operator bool() const noexcept { return value != 0; }
  template <typename Enum2>
  inline constexpr bool operator()(Enum2 iValue) const noexcept
  {
    return has(iValue);
  }
  template <typename Enum2>
  inline constexpr bool has(Enum2 iValue) const noexcept
  {
    return (value & toFlag(iValue)) != 0;
  }
  template <typename... Enum2>
  inline constexpr bool anyOf(Enum2... iValue) const noexcept
  {
    return (value & ((toFlag(iValue)) | ...)) != 0;
  }
  template <typename... Enum2>
  inline constexpr bool allOff(Enum2... iValue) const noexcept
  {
    return (value & ((toFlag(iValue)) | ...)) != ((toFlag(iValue)) | ...);
  }

  template <typename... Enum2>
  inline constexpr void set(bool iControl, Enum2... iValue) noexcept
  {
    value = (iControl ? (value | (toFlag(iValue) | ...)) : (value & ((~toFlag(iValue)) & ...)));
  }
  template <typename... Enum2>
  inline constexpr void set(Enum2... iValue) noexcept
  {
    set(true, iValue...);
  }
  template <typename... Enum2>
  inline constexpr void unset(Enum2... iValue) noexcept
  {
    set(false, iValue...);
  }

  inline constexpr auto operator<=>(EnumFlags const& other) const noexcept = default;

  inline constexpr StorageType get() const noexcept { return value.value(); }

  inline constexpr void set(StorageType iVal) const noexcept { value = iVal; }

  std::uint64_t toU64() const noexcept { return static_cast<std::uint64_t>(value); }

  void fromU64(std::uint64_t v) noexcept { value = (static_cast<StorageType>(v)); }

private:
  StorageType value;
};

#define DECLARE_MASK_FLAGS(EnumClass)                                                                                  \
  inline constexpr EnumClass operator|(EnumClass lhs, EnumClass rhs)                                                   \
  {                                                                                                                    \
    using T = std::underlying_type_t<EnumClass>;                                                                       \
    return static_cast<EnumClass>(static_cast<T>(lhs) | static_cast<T>(rhs));                                          \
  }                                                                                                                    \
  inline EnumClass& operator|=(EnumClass& lhs, EnumClass rhs)                                                \
  {                                                                                                                    \
    lhs = lhs | rhs;                                                                                                   \
    return lhs;                                                                                                        \
  }                                                                                                                    \
  inline constexpr EnumClass operator~(EnumClass lhs)                                                                  \
  {                                                                                                                    \
    using T = std::underlying_type_t<EnumClass>;                                                                       \
    return static_cast<EnumClass>(~static_cast<T>(lhs));                                                               \
  }                                                                                                                    \
  inline constexpr EnumClass operator&(EnumClass lhs, EnumClass rhs)                                                   \
  {                                                                                                                    \
    using T = std::underlying_type_t<EnumClass>;                                                                       \
    return static_cast<EnumClass>(static_cast<T>(lhs) & static_cast<T>(rhs));                                          \
  }                                                                                                                    \
  inline auto operator&=(EnumClass& lhs, EnumClass rhs)                                                      \
  {                                                                                                                    \
    lhs = lhs & rhs;                                                                                                   \
    return lhs;                                                                                                        \
  }                                                                                                                    \
  inline constexpr bool operator!(EnumClass lhs)                                                                       \
  {                                                                                                                    \
    using T = std::underlying_type_t<EnumClass>;                                                                       \
    return static_cast<T>(lhs) == 0;                                                                                   \
  }

template <int i, typename Int = std::uint32_t>
constexpr static inline Int defineFlag(Int v)
{
  return static_cast<Int>(1) << v;
}

#define DEFINE_FLAG(Flag, Value) inline static constexpr auto BC_TOKEN_PASTE(f, Flag) = defineFlag(Value)

// Prefer enum flags
#define DECLARE_SCOPED_MASK_FLAGS(Scope, Type)                                                                         \
  DECLARE_MASK_FLAGS(Scope::Type)                                                                                      \
  using BC_TOKEN_PASTE(Scope, Flags) = Scope::Type

} // namespace @default_namespace_name@
