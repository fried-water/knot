#pragma once

#include <array>
#include <optional>
#include <type_traits>
#include <utility>

namespace knot {

template <typename T>
struct Type {
  using type = T;
};

struct NotAType {};

template <typename T1, typename T2>
constexpr bool operator==(Type<T1>, Type<T2>) {
  return std::is_same_v<T1, T2>;
}

template <typename T1, typename T2>
constexpr bool operator!=(Type<T1> t1, Type<T2> t2) {
  return !(t1 == t2);
}

constexpr bool operator==(NotAType, NotAType) { return false; }
constexpr bool operator!=(NotAType, NotAType) { return true; }

template <typename T>
constexpr bool operator==(Type<T>, NotAType) {
  return false;
}

template <typename T>
constexpr bool operator==(NotAType, Type<T>) {
  return false;
}

template <typename T>
constexpr bool operator!=(Type<T>, NotAType) {
  return true;
}

template <typename T>
constexpr bool operator!=(NotAType, Type<T>) {
  return true;
}

template <typename T>
using type_t = typename T::type;

template <typename... Ts>
struct TypeList {};

template <typename... Ts>
constexpr auto typelist(Type<Ts>...) {
  return TypeList<Ts...>{};
}

template <typename T>
constexpr auto type_of(const T&) {
  return Type<T>{};
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

template <int I, typename T>
constexpr std::optional<int> idx_of_helper(TypeList<>, Type<T>) {
  return std::nullopt;
}

template <int I, typename T, typename... Ts>
constexpr std::optional<int> idx_of_helper(TypeList<Ts...> ts, Type<T> t) {
  return t == head(ts) ? std::optional(I) : idx_of_helper<I + 1>(tail(ts), t);
}

template <typename T, typename... Ts>
constexpr std::optional<int> idx_of(TypeList<Ts...> ts, Type<T> t) {
  return idx_of_helper<0>(ts, t);
}

template <typename T, typename... Ts>
constexpr bool contains(TypeList<Ts...> ts, Type<T> t) {
  return idx_of(ts, t).has_value();
}

template <int I>
constexpr auto get(TypeList<>) {
  return NotAType{};
}

template <int I, typename... Ts>
constexpr auto get(TypeList<Ts...> ts) {
  if constexpr (I == 0) {
    return head(ts);
  } else {
    return get<I - 1>(tail(ts));
  }
}

template <typename... Ts, typename F>
constexpr auto map(TypeList<Ts...>, F f) {
  return typelist(f(Type<Ts>{})...);
}

template <typename... Ts>
constexpr auto idx_seq(TypeList<Ts...>) {
  return std::make_index_sequence<sizeof...(Ts)>{};
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
constexpr bool is_raw_pointer(Type<T>) {
  return std::is_pointer_v<T>;
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
constexpr auto remove_const(Type<const T>) {
  return Type<T>{};
}

template <typename T>
constexpr auto remove_const(Type<T>) {
  return Type<T>{};
}

template <typename T>
constexpr bool is_ref(Type<T>) {
  return std::is_reference_v<T>;
}

template <typename T>
constexpr bool is_aggregate(Type<T>) {
  return std::is_aggregate_v<T>;
}

template <typename F, typename... Ts>
constexpr auto is_invocable(Type<F>, TypeList<Ts...>) {
  return std::is_invocable_v<F, Ts...>;
}

template <typename F, typename... Ts>
constexpr auto invoke_result(Type<F> f, TypeList<Ts...> ts) {
  if constexpr (is_invocable(f, ts)) {
    return Type<std::invoke_result_t<F, Ts...>>{};
  } else {
    return NotAType{};
  }
}

template <typename T>
constexpr auto value_type(Type<T> t) {
  return Type<typename T::value_type>{};
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

constexpr inline auto is_tuple_like = is_valid([](auto&& t) -> std::tuple_size<std::decay_t<decltype(t)>> {});

template <typename T>
constexpr auto tuple_size(Type<T>) {
  return std::tuple_size_v<T>;
}

template <size_t I, typename T>
constexpr auto tuple_element(Type<T>) {
  return Type<std::tuple_element_t<I, T>>{};
}

template <typename T, typename = std::enable_if_t<is_tuple_like(Type<T>{})>>
constexpr auto idx_seq(Type<T> t) {
  return std::make_index_sequence<tuple_size(t)>{};
}

template <typename... Ts>
constexpr auto as_tuple(TypeList<Ts...>) {
  return Type<std::tuple<Ts...>>{};
}

template <typename T, size_t... Is>
constexpr auto as_typelist(Type<T> t, std::index_sequence<Is...>) {
  return typelist(tuple_element<Is>(t)...);
}

template <typename T, typename = std::enable_if_t<is_tuple_like(Type<T>{})>>
constexpr auto as_typelist(Type<T> t) {
  return as_typelist(t, idx_seq(t));
}

template <typename... Ts>
constexpr auto as_typelist(Type<std::variant<Ts...>> t) {
  return TypeList<Ts...>{};
}

}  // namespace knot
