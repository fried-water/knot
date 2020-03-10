#ifndef KNOT_TYPE_TRAITS
#define KNOT_TYPE_TRAITS

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

template <typename, typename = void>
struct is_std_hashable : std::false_type {};
template <typename T>
struct is_std_hashable<T, std::void_t<decltype(std::hash<T>{}(std::declval<T>()))>> : std::true_type {};
template <typename T>
inline constexpr bool is_std_hashable_v = is_std_hashable<T>::value;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-internal"

struct filler {
  // Exception for std::optional<T> is needed because optional has a constructor
  // from U if U is convertable to T and filler is convertable to everything
  template <typename T, typename = std::enable_if_t<!is_optional_v<T>>>
  operator T();
};

#pragma clang diagnostic pop

template <typename T, typename Seq = std::index_sequence<>, typename = void>
struct aggregate_arity : Seq {};

template <typename T, std::size_t... Is>
struct aggregate_arity<T, std::index_sequence<Is...>,
                       std::void_t<decltype(T{std::declval<filler>(), (Is, std::declval<filler>())...})>>
    : aggregate_arity<T, std::index_sequence<Is..., sizeof...(Is)>> {};

template <typename T>
constexpr std::enable_if_t<std::is_aggregate_v<T>, std::size_t> arity() {
  return aggregate_arity<std::decay_t<T>>::size();
}

}  // namespace details
}  // namespace knot

#endif