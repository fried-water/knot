#pragma once

#include <array>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace knot {
namespace details {

template <typename>
inline constexpr bool is_array_v = false;

template <typename T, std::size_t N>
inline constexpr bool is_array_v<std::array<T, N>> = true;

template <typename>
inline constexpr bool is_optional_v = false;
template <typename T>
inline constexpr bool is_optional_v<std::optional<T>> = true;

template <typename T>
inline constexpr bool is_pointer_v = std::is_pointer_v<T>;
template <typename T, typename Del>
inline constexpr bool is_pointer_v<std::unique_ptr<T, Del>> = true;
template <typename T>
inline constexpr bool is_pointer_v<std::shared_ptr<T>> = true;

template <typename>
inline constexpr bool is_tuple_v = false;
template <typename... Ts>
inline constexpr bool is_tuple_v<std::tuple<Ts...>> = true;

template <typename>
inline constexpr bool is_variant_v = false;
template <typename... Ts>
inline constexpr bool is_variant_v<std::variant<Ts...>> = true;

template <typename, typename = void>
struct is_range : std::false_type {};
template <typename T>
struct is_range<T, std::void_t<decltype(std::begin(std::declval<T>())), decltype(std::end(std::declval<T>()))>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_range_v = is_range<T>::value;

template <typename, typename = void>
struct is_reserveable : std::false_type {};
template <typename T>
struct is_reserveable<T, std::void_t<decltype(std::declval<T>().reserve(0))>> : std::true_type {};
template <typename T>
inline constexpr bool is_reserveable_v = is_reserveable<T>::value;

template <typename T, typename Enable = void>
struct output_it_value {
  using type = uint8_t;
};
template <typename T>
struct output_it_value<T, std::void_t<decltype(*std::declval<T>() = std::byte{})>> {
  using type = std::byte;
};
template <typename T>
using output_it_value_t = typename output_it_value<T>::type;

template <typename, typename = void>
struct is_std_hashable : std::false_type {};
template <typename T>
struct is_std_hashable<T, std::void_t<decltype(std::hash<T>{}(std::declval<T>()))>> : std::true_type {};
template <typename T>
inline constexpr bool is_std_hashable_v = is_std_hashable<T>::value;

}  // namespace details
}  // namespace knot
