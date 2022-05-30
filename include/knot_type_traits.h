#pragma once

#include <array>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace knot {

template <typename T>
struct Type {
  using type = T;
};

template <typename T1, typename T2>
constexpr bool operator==(Type<T1>, Type<T2>) {
  return std::is_same_v<T1, T2>;
}

template <typename T1, typename T2>
constexpr bool operator!=(Type<T1> t1, Type<T2> t2) {
  return !(t1 == t2);
}

template <typename T>
using type_t = typename T::type;

template <typename... Ts>
struct TypeList {};

template <typename... Ts>
constexpr auto typelist(Type<Ts>...) {
  return TypeList<Ts...>{};
}

template <typename... Ts>
constexpr size_t size(TypeList<Ts...>) {
  return sizeof...(Ts);
}

template <typename T, typename... Ts>
constexpr auto head(TypeList<T, Ts...>) {
  return Type<T>{};
}

template <typename T, typename... Ts>
constexpr auto tail(TypeList<T, Ts...>) {
  return TypeList<Ts...>{};
}

template <typename... Ts, typename F>
constexpr auto map(TypeList<Ts...>, F f) {
  return typelist(f(Type<Ts>{})...);
}

template <typename... Ts>
constexpr auto as_tuple(TypeList<Ts...>) {
  return Type<std::tuple<Ts...>>{};
}

template <typename... Ts>
constexpr auto as_typelist(Type<std::tuple<Ts...>>) {
  return TypeList<Ts...>{};
}

template <typename T>
constexpr bool is_enum(Type<T>) {
  return std::is_enum_v<T>;
}

template <typename T>
constexpr bool is_arithmetic(Type<T>) {
  return std::is_arithmetic_v<T>;
}

template <typename T>
constexpr bool is_array(Type<T>) {
  return false;
}

template <typename T, std::size_t N>
constexpr bool is_array(Type<std::array<T, N>>) {
  return true;
}

template <typename T>
constexpr bool is_optional(Type<T>) {
  return false;
}

template <typename T>
constexpr bool is_optional(Type<std::optional<T>>) {
  return true;
}

template <typename T>
constexpr bool is_raw_pointer(Type<T>) {
  return std::is_pointer_v<T>;
}

template <typename T>
constexpr bool is_pointer(Type<T> t) {
  return is_raw_pointer(t);
}

template <typename T, typename D>
constexpr bool is_pointer(Type<std::unique_ptr<T, D>>) {
  return true;
}

template <typename T>
constexpr bool is_pointer(Type<std::shared_ptr<T>>) {
  return true;
}

template <typename T>
constexpr bool is_tuple(Type<T>) {
  return false;
}

template <typename... Ts>
constexpr bool is_tuple(Type<std::tuple<Ts...>>) {
  return true;
}

template <typename... Ts>
constexpr size_t size(Type<std::tuple<Ts...>>) {
  return sizeof...(Ts);
}

template <typename T>
constexpr bool is_variant(Type<T>) {
  return false;
}

template <typename... Ts>
constexpr bool is_variant(Type<std::variant<Ts...>>) {
  return true;
}

template <typename T>
constexpr auto decay(Type<T>) {
  return Type<std::decay_t<T>>{};
}

template <typename T>
constexpr bool is_decayed(Type<T> t) {
  return t == decay(t);
}

template <typename T>
constexpr bool is_ref(Type<T>) {
  return std::is_reference_v<T>;
}

namespace details {

template <typename F, typename... Ts>
struct is_valid_helper {
  template <typename U, typename... Args>
  static decltype(std::declval<U>()(std::declval<Ts>()...), std::true_type{}) f(std::remove_reference_t<U>*);

  template <typename U, typename... Args>
  static std::false_type f(...);

  static constexpr bool value = decltype(f<F, Ts...>(nullptr))::value;
};

template <typename F, typename... Ts>
struct is_valid_helper<F, Type<Ts>...> : is_valid_helper<F, Ts...> {};

}  // namespace details

template <typename F>
constexpr auto is_valid(F) {
  return [](auto... ts) { return details::is_valid_helper<F, decltype(ts)...>::value; };
}

template <typename T>
constexpr bool is_range(Type<T> t) {
  return is_valid([](auto&& t) -> decltype(t.begin()) {})(t) && is_valid([](auto&& t) -> decltype(t.end()) {})(t);
}

}  // namespace knot
