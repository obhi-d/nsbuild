
#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

namespace enums
{

namespace detail
{

class enum_hash
{
public:
  template <std::size_t N>
  constexpr enum_hash(char const (&a)[N]) noexcept : p(a), sz(N - 1)
  {}
  constexpr enum_hash(char const* a, std::size_t const N) noexcept : p(a), sz(N) {}

  constexpr char operator[](std::size_t n) const noexcept { return n < sz ? p[n] : 0; }

  constexpr std::size_t size() const noexcept { return sz; }
  constexpr char const* data() const noexcept { return p; }
  constexpr enum_hash   substring(const std::size_t start, const std::size_t end) const noexcept
  {
    return enum_hash(p + start, size() - (end + start));
  }

  constexpr std::uint32_t hash(const std::uint32_t seed = 0) const noexcept { return compute_murmur3(seed); }

  constexpr std::uint32_t fnv1a_hash() const noexcept { return compute_fnv1a(p, static_cast<std::int32_t>(sz)); }

  inline constexpr operator std::string_view() const noexcept { return std::string_view(p, sz); }

  inline auto operator<=>(enum_hash const&) const noexcept = default;

private:
  static constexpr std::uint32_t compute_fnv1a(char const* const s, std::int32_t count) noexcept
  {
    return ((count ? compute_fnv1a(s, count - 1) : 2166136261u) ^ static_cast<std::uint8_t>(s[count])) * 16777619u;
  }

  constexpr uint32_t get_block(int idx) const noexcept
  {
    int      i  = (static_cast<int>(block_size()) + idx) * 4;
    uint32_t b0 = p[i];
    uint32_t b1 = p[i + 1];
    uint32_t b2 = p[i + 2];
    uint32_t b3 = p[i + 3];
    return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
  }

  constexpr std::size_t block_size() const noexcept { return sz / 4; }

  constexpr char tail(const int n) const noexcept
  {
    int tail_size = sz % 4;
    return p[sz - tail_size + n];
  }

  constexpr std::uint32_t compute_murmur3(const std::uint32_t seed = 0) const noexcept
  {
    std::uint32_t h1 = seed;

    const std::uint32_t c1 = 0xcc9e2d51;
    const std::uint32_t c2 = 0x1b873593;

    const std::int32_t nblocks = static_cast<std::int32_t>(block_size());
    for (std::int32_t i = -nblocks; i; i++)
    {
      std::uint32_t k1 = get_block(i);

      k1 *= c1;
      k1 = (k1 << 15) | (k1 >> (32 - 15));
      k1 *= c2;

      h1 ^= k1;
      h1 = (h1 << 13) | (h1 >> (32 - 13));
      h1 = h1 * 5 + 0xe6546b64;
    }

    std::uint32_t k1 = 0;

    switch (size() & 3)
    {
    case 3:
      k1 ^= tail(2) << 16;
    case 2:
      k1 ^= tail(1) << 8;
    case 1:
      k1 ^= tail(0);
      k1 *= c1;
      k1 = (k1 << 15) | (k1 >> (32 - 15));
      k1 *= c2;
      h1 ^= k1;
    };

    h1 ^= static_cast<std::uint32_t>(size());

    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;
    return h1;
  }

  char const*       p;
  std::size_t const sz;
};

} // namespace detail

struct Key
{
  template <std::size_t N>
  constexpr Key(char const (&a)[N]) noexcept : value(detail::enum_hash(a).hash())
  {}

  constexpr Key(std::string_view view) noexcept : value(detail::enum_hash(view.data(), view.length()).hash()) {}

  inline auto operator<=>(Key const&) const noexcept = default;
  uint32_t    value;
};

// Returns the string fragment without the prefix
template <typename EnumType, typename ArrayType, typename StringType>
inline std::tuple<EnumType, StringType> StringStartsWith(ArrayType const& iList, StringType iValue,
                                                             EnumType defaultValue)
{
  // clang-format off
  for (auto const& e : iList)
  {
    StringType v = std::get<0>(e);
    if(iValue.starts_with(v))
      return std::tuple<EnumType, StringType>{std::get<1>(e), iValue.substr(v.length())};
  }
  return std::tuple<EnumType, StringType>{defaultValue, iValue};
}

// Returns the string fragment without the suffix
template <typename EnumType, typename ArrayType, typename StringType>
inline std::tuple<EnumType, StringType> StringEndsWith(ArrayType const& iList, StringType iValue, EnumType defaultValue)
{
  // clang-format off
  for (auto const& e : iList)
  {
    StringType v = std::get<0>(e);
    if(iValue.ends_with(v))
      return std::tuple<EnumType, StringType>{std::get<1>(e), iValue.substr(0, iValue.length() - v.length())};
  }
  return std::tuple<EnumType, StringType>{defaultValue, iValue};
}

template <typename EnumType, typename ArrayType, typename StringType, typename Linear = std::false_type>
inline EnumType FromString(ArrayType const& iList, StringType iValue, EnumType defaultValue, Linear l = {})
{
  constexpr std::uint32_t kMinLinearSearch = 16;
  // clang-format off
  auto             it               = std::size(iList) <= kMinLinearSearch || Linear::value
                                        ? std::find_if(std::begin(iList), std::end(iList),
                                           [&iValue](auto const& iComp)
                                           {
                                             return std::get<0>(iComp) == iValue;
                                           })
                                        : std::lower_bound(std::begin(iList), std::end(iList), iValue,
                                           [](auto const& iFirst, StringType const& iOther)
                                           {
                                             return std::get<0>(iFirst).compare(iOther) < 0;
                                           });
  // clang-format on
  return ((it != (std::end(iList))) && (std::get<0>(*it) == iValue)) ? std::get<1>(*it) : defaultValue;
}

template <typename Array, unsigned kTupleIndex>
inline constexpr auto SortTuple(Array arr)
{
  auto        result  = arr;
  std::size_t n       = arr.size();
  bool        swapped = false;
  do
  {
    for (std::size_t i = 1; i < n; ++i)
    {
      if (std::get<kTupleIndex>(result[i - 1]) > std::get<kTupleIndex>(result[i]))
      {
        auto t        = result[i - 1];
        result[i - 1] = result[i];
        result[i]     = t;
        swapped       = true;
      }
    }
  } while (!swapped);
  return result;
}

} // namespace enums