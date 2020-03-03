#ifndef KNOT_H
#define KNOT_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace knot {

template <typename T>
std::vector<uint8_t> serialize(const T&);

template <typename T, typename IT>
IT serialize(const T&, IT out);

// In addition to as_tie(), deserialize requires that structs be constructible
// from the types returned in as_tie().
// Furthermore raw pointers and references aren't supported.
template <typename T, typename IT>
std::optional<std::remove_const_t<T>> deserialize(IT begin, IT end);

template <typename T, typename IT>
std::optional<std::pair<std::remove_const_t<T>, IT>> deserialize_partial(IT begin, IT end);

template <typename T>
std::string debug_string(const T&);

template <typename T>
std::ostream& debug_string(std::ostream&, const T&);

template <typename T>
std::size_t hash_value(const T&);

template <typename T, typename F>
void visit(const T&, F f);

template <typename Result, typename T, typename F>
Result accumulate(const T& t, F f, Result acc = {}) {
  visit(t, [&](const auto& value) {
    acc = f(value, std::move(acc));
    return true;
  });
  return acc;
}

namespace details {

// std as_tie() overloads
template <typename T>
auto as_tie(const std::reference_wrapper<T>& t) {
  return std::tie(t.get());
}

template <typename First, typename Second>
auto as_tie(const std::pair<First, Second>& pair) {
  return std::tie(pair.first, pair.second);
}

// Type Traits
template <typename, typename = void>
struct is_tieable : std::false_type {};
template <typename T>
struct is_tieable<T, std::void_t<decltype(as_tie(std::declval<T>()))>> : std::true_type {};
template <typename T>
inline constexpr bool is_tieable_v = is_tieable<T>::value;

template <typename>
inline constexpr bool is_array_v = false;
template <typename T, std::size_t N>
inline constexpr bool is_array_v<std::array<T, N>> = true;

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

template <typename>
inline constexpr bool is_optional_v = false;
template <typename T>
inline constexpr bool is_optional_v<std::optional<T>> = true;

template <typename, typename = void>
struct is_range : std::false_type {};
template <typename T>
struct is_range<T, std::void_t<decltype(std::begin(std::declval<T>())), decltype(std::end(std::declval<T>()))>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_range_v = is_range<T>::value;

// We treat tuples, pairs, and tieable structs as product types
template <typename T>
inline constexpr bool is_product_type_v = is_tieable_v<T> || is_tuple_v<T>;

// We treat variants, optionals, and smart ptrs as sum types
template <typename T>
inline constexpr bool is_sum_type_v = is_optional_v<T> || is_variant_v<T> || is_pointer_v<T>;

template <typename T>
inline constexpr bool is_primitive_type_v = std::is_arithmetic_v<T> || std::is_enum_v<T>;

template <typename T>
inline constexpr bool is_knot_supported_type_v =
    is_product_type_v<T> || is_sum_type_v<T> || is_range_v<T> || is_primitive_type_v<T>;

template <typename>
inline constexpr bool is_ref_wrapper_v = false;
template <typename T>
inline constexpr bool is_ref_wrapper_v<std::reference_wrapper<T>> = true;

template <typename, typename = void>
struct is_reserveable : std::false_type {};
template <typename T>
struct is_reserveable<T, std::void_t<decltype(std::declval<T>().reserve(0))>> : std::true_type {};
template <typename T>
inline constexpr bool is_reserveable_v = is_reserveable<T>::value;

template <typename T, typename T2 = void>
using tie_enable = std::enable_if_t<is_tieable_v<T>, T2>;

// Generic product type utilities (tuple, pair, tieable structs)

template <typename, typename = void>
struct as_tuple_type;

template <typename... Ts>
struct as_tuple_type<std::tuple<Ts...>> {
  using type = std::tuple<Ts...>;
};

template <typename T>
struct tuple_from_tie;

template <typename... Ts>
struct tuple_from_tie<std::tuple<const Ts&...>> {
  using type = typename std::tuple<Ts...>;
};

template <typename T>
struct as_tuple_type<T, tie_enable<T>> {
  using type = typename tuple_from_tie<decltype(as_tie(std::declval<T>()))>::type;
};

template <typename T>
using as_tuple_type_t = typename as_tuple_type<T>::type;

template <typename... Ts>
const auto& as_tuple(const std::tuple<Ts...>& tuple) {
  return tuple;
}

template <typename T>
auto as_tuple(const T& tieable) {
  return as_tie(tieable);
}

template <typename T, typename Tuple, std::size_t... Is>
T from_tuple_helper(Tuple&& t, std::index_sequence<Is...>) {
  return T{std::get<Is>(std::forward<Tuple>(t))...};
}

template <typename Result, typename... Ts>
Result from_tuple(std::tuple<Ts...>&& t) {
  return from_tuple_helper<Result>(std::move(t), std::make_index_sequence<sizeof...(Ts)>{});
}

// Generic sum type utilities (variant, optional, pointers)

template <typename, typename = void>
struct as_variant_type;

template <typename... Ts>
struct as_variant_type<std::variant<Ts...>> {
  using type = std::variant<Ts...>;
};

template <typename T>
struct as_variant_type<T, std::enable_if_t<is_pointer_v<T> || is_optional_v<T>>> {
  using type = std::variant<std::tuple<>, std::decay_t<decltype(*std::declval<T>())>>;
};

template <typename T>
using as_variant_type_t = typename as_variant_type<T>::type;

template <typename... Ts>
const auto& as_variant_ref(const std::variant<Ts...>& variant) {
  return variant;
}

template <typename T>
auto as_variant_ref(const T& t) {
  using variant_t = std::variant<std::tuple<>, std::reference_wrapper<const std::decay_t<decltype(*t)>>>;
  return t ? variant_t{std::cref(*t)} : variant_t{std::make_tuple()};
}

template <typename Result>
std::enable_if_t<is_variant_v<Result>, Result> from_variant(Result&& var) {
  return std::move(var);
}

template <typename Result, typename T>
std::enable_if_t<is_optional_v<Result>, Result> from_variant(std::variant<std::tuple<>, T>&& var) {
  return var.index() == 0 ? std::nullopt : Result{std::move(*std::get_if<1>(&var))};
}

template <typename Result, typename T>
std::enable_if_t<is_pointer_v<Result>, Result> from_variant(std::variant<std::tuple<>, T>&& var) {
  return Result{var.index() == 0 ? nullptr : new T{std::move(*std::get_if<1>(&var))}};
}

// Monadic optional wrapper for help with deserialize

template <typename T>
struct Monad;

template <typename First, typename Second>
struct Monad<std::optional<std::pair<First, Second>>> {
  std::optional<std::pair<First, Second>> opt;

  template <typename F>
  auto and_then(F f) && {
    using result_type = decltype(f(std::move(opt->first), std::move(opt->second)));
    return Monad<result_type>{opt ? f(std::move(opt->first), std::move(opt->second)) : std::nullopt};
  }

  template <typename F>
  auto map(F f) && {
    using result_type = std::optional<decltype(f(std::move(opt->first), std::move(opt->second)))>;
    return Monad<result_type>{opt ? std::make_optional(f(std::move(opt->first), std::move(opt->second)))
                                  : std::nullopt};
  }

  operator std::optional<std::pair<First, Second>>() && { return std::move(opt); }

  std::optional<std::pair<First, Second>> get() && { return std::move(opt); }
};

template <typename T>
Monad<T> make_monad(T t) {
  return Monad<T>{std::move(t)};
}

// deserialize helpers

template <typename>
struct tuple_rest;

template <typename T, typename... Rest>
struct tuple_rest<std::tuple<T, Rest...>> {
  using type = std::tuple<Rest...>;
};

template <typename T, typename IT>
std::optional<std::pair<T, IT>> tuple_deserialize(IT begin, IT end) {
  if constexpr (std::tuple_size_v<T> == 0) {
    return std::make_pair(std::make_tuple(), begin);
  } else {
    using first_element = std::remove_const_t<std::tuple_element_t<0, T>>;
    using tuple_rest = typename tuple_rest<T>::type;

    return make_monad(deserialize_partial<first_element>(begin, end)).and_then([end](first_element&& first, IT begin) {
      return make_monad(tuple_deserialize<tuple_rest>(begin, end))
          .map([&first](tuple_rest&& rest, IT begin) {
            return std::make_pair(T{std::tuple_cat(std::make_tuple(std::move(first)), std::move(rest))}, begin);
          })
          .get();
    });
  }
}

template <typename Variant, typename IT, std::size_t... Is>
std::optional<std::pair<Variant, IT>> variant_deserialize(IT begin, IT end, uint8_t index, std::index_sequence<Is...>) {
  static constexpr auto options = std::array{+[](IT begin, IT end) {
    return make_monad(deserialize_partial<std::variant_alternative_t<Is, Variant>>(begin, end))
        .map([](auto&& ele, IT begin) { return std::make_pair(Variant{std::move(ele)}, begin); })
        .get();
  }...};
  return index >= sizeof...(Is) ? std::nullopt : options[index](begin, end);
}

// Taken from boost::hash_combine
inline std::size_t hash_combine(std::size_t seed, std::size_t hash) {
  return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

template <typename T, std::size_t... Is>
void tuple_debug_string(std::ostream& os, const T& tuple, std::index_sequence<Is...>) {
  (debug_string(os << ", ", std::get<Is + 1>(tuple)), ...);
}

template <typename T, typename Visitor, std::size_t... Is>
void visit_tuple(const T& tuple, Visitor visitor, std::index_sequence<Is...>) {
  ((visit(std::get<Is>(tuple), visitor)), ...);
};

}  // namespace details

template <typename T>
std::vector<uint8_t> serialize(const T& t) {
  std::vector<uint8_t> buf;
  serialize(t, std::back_inserter(buf));
  return buf;
}

template <typename Outer, typename IT>
IT serialize(const Outer& t, IT out) {
  auto serialize_primitive = [](auto value, auto it) {
    std::array<uint8_t, sizeof(value)> array{};
    std::memcpy(&array, &value, sizeof(value));
    return std::copy(array.begin(), array.end(), it);
  };

  return accumulate(t,
                    [&](const auto& value, IT out) {
                      using T = std::decay_t<decltype(value)>;
                      if constexpr (details::is_primitive_type_v<T>) {
                        return serialize_primitive(value, out);
                      } else if constexpr (details::is_sum_type_v<T>) {
                        return serialize_primitive(static_cast<uint8_t>(details::as_variant_ref(value).index()), out);
                      } else if constexpr (details::is_range_v<T>) {
                        return serialize_primitive(value.size(), out);
                      } else {
                        return out;
                      }
                    },
                    out);
}

template <typename T, typename IT>
std::optional<std::remove_const_t<T>> deserialize(IT begin, IT end) {
  auto opt = deserialize_partial<T>(begin, end);

  if (!opt || opt->second != end) return std::nullopt;

  return std::move(opt->first);
}

template <typename Outer, typename IT>
std::optional<std::pair<std::remove_const_t<Outer>, IT>> deserialize_partial(IT begin, IT end) {
  using T = std::remove_const_t<Outer>;

  static_assert(!std::is_reference_v<Outer> && !details::is_ref_wrapper_v<Outer>);
  static_assert(!std::is_pointer_v<T>);
  static_assert(std::is_same_v<typename IT::value_type, uint8_t>);
  static_assert(details::is_knot_supported_type_v<T>);

  if constexpr (details::is_primitive_type_v<T>) {
    if (std::distance(begin, end) < sizeof(T)) return std::nullopt;

    std::array<uint8_t, sizeof(T)> array;
    std::copy(begin, begin + sizeof(T), array.begin());

    T t;
    std::memcpy(&t, &array, sizeof(T));

    return std::pair<T, IT>{t, begin + sizeof(T)};
  } else if constexpr (details::is_sum_type_v<T>) {
    using variant_t = details::as_variant_type_t<T>;
    return details::make_monad(deserialize_partial<uint8_t>(begin, end)).and_then([end](uint8_t index, IT begin) {
      return details::make_monad(details::variant_deserialize<variant_t>(
                                     begin, end, index, std::make_index_sequence<std::variant_size_v<variant_t>>()))
          .map([](variant_t&& variant, IT begin) {
            return std::make_pair(details::from_variant<T>(std::move(variant)), begin);
          })
          .get();
    });
  } else if constexpr (details::is_range_v<T>) {
    return details::make_monad(deserialize_partial<std::size_t>(begin, end))
        .and_then([end](std::size_t size, IT begin) -> std::optional<std::pair<T, IT>> {
          T range{};
          if constexpr (details::is_reserveable_v<T>) range.reserve(size);

          for (std::size_t i = 0; i < size; i++) {
            auto ele_opt = deserialize_partial<typename T::value_type>(begin, end);
            if (!ele_opt) return std::nullopt;
            if constexpr (details::is_array_v<T>) {
              range[i] = std::move(ele_opt->first);
            } else {
              range.insert(range.end(), std::move(ele_opt->first));
            }
            begin = ele_opt->second;
          }

          return std::pair<T, IT>{std::move(range), begin};
        });
  } else if constexpr (details::is_product_type_v<T>) {
    return details::make_monad(details::tuple_deserialize<details::as_tuple_type_t<T>>(begin, end))
        .map([](auto&& tuple, IT begin) { return std::make_pair(details::from_tuple<T>(std::move(tuple)), begin); });
  } else {
    return std::nullopt;
  }
}

template <typename T>
std::string debug_string(const T& t) {
  std::ostringstream os;
  debug_string(os, t);
  return std::move(os).str();
}

template <typename T>
std::ostream& debug_string(std::ostream& os, const T& t) {
  static_assert(details::is_knot_supported_type_v<T>);

  if constexpr (std::is_arithmetic_v<T>) {
    return os << t;
  } else if constexpr (std::is_enum_v<T>) {
    return os << static_cast<std::underlying_type_t<T>>(t);
  } else if constexpr (details::is_sum_type_v<T>) {
    os << "<";
    std::visit(
        [&os](const auto& inner) {
          using inner_t = std::decay_t<decltype(inner)>;
          if constexpr (std::is_same_v<inner_t, std::tuple<>>) {
            os << "null";
          } else if constexpr (details::is_ref_wrapper_v<inner_t>) {
            debug_string(os, inner.get());
          } else {
            debug_string(os, inner);
          }
        },
        details::as_variant_ref(t));
    return os << ">";
  } else if constexpr (std::is_same_v<T, std::string>) {
    // Special case string
    return os << "\"" << t << "\"";
  } else if constexpr (details::is_range_v<T>) {
    os << "[";
    auto it = t.begin();
    if (it != t.end()) {
      debug_string(os, *it);
      std::for_each(++it, t.end(), [&os](const auto& ele) { debug_string(os << ", ", ele); });
    }
    return os << "]";
  } else if constexpr (details::is_product_type_v<T>) {
    const auto& tuple = details::as_tuple(t);
    os << "(";
    if constexpr (std::tuple_size_v<details::as_tuple_type_t<T>>> 0) {
      debug_string(os, std::get<0>(tuple));
      details::tuple_debug_string(os, tuple,
                                  std::make_index_sequence<std::tuple_size_v<details::as_tuple_type_t<T>> - 1>());
    }
    return os << ")";
  } else {
    return os;
  }
}

template <typename Outer>
std::size_t hash_value(const Outer& t) {
  return accumulate<std::size_t>(t, [&](const auto& value, const std::size_t hash) {
    using T = std::decay_t<decltype(value)>;
    if constexpr (details::is_primitive_type_v<T>) {
      return details::hash_combine(hash, std::hash<T>{}(value));
    } else if constexpr (details::is_sum_type_v<T>) {
      return details::hash_combine(hash, details::as_variant_ref(value).index());
    } else {
      return hash;
    }
  });
}

template <typename T, typename Visitor>
void visit(const T& t, Visitor visitor) {
  static_assert(details::is_knot_supported_type_v<T>);

  // visit the value, if the function returns a value, stop recursing on false
  if constexpr (std::is_same_v<void, decltype(visitor(t))>) {
    visitor(t);
  } else {
    if (!visitor(t)) return;
  }

  // Call visit recursively depending on type
  if constexpr (details::is_product_type_v<T>) {
    details::visit_tuple(details::as_tuple(t), visitor,
                         std::make_index_sequence<std::tuple_size_v<details::as_tuple_type_t<T>>>());
  } else if constexpr (details::is_sum_type_v<T>) {
    std::visit([&](const auto& val) { return visit(val, visitor); }, details::as_variant_ref(t));
  } else if constexpr (details::is_range_v<T>) {
    for (const auto& val : t) visit(val, visitor);
  }
}

// std::hash<T> replacement for tieable types
struct Hash {
  template <typename T>
  std::size_t operator()(const T& t) const {
    return knot::hash_value(t);
  }
};

//
struct Compareable {
  template <typename T>
  friend details::tie_enable<T, bool> operator==(const T& lhs, const T& rhs) {
    return as_tie(lhs) == as_tie(rhs);
  }

  template <typename T>
  friend details::tie_enable<T, bool> operator!=(const T& lhs, const T& rhs) {
    return as_tie(lhs) != as_tie(rhs);
  }
};

struct Ordered : Compareable {
  template <typename T>
  friend details::tie_enable<T, bool> operator<(const T& lhs, const T& rhs) {
    return as_tie(lhs) < as_tie(rhs);
  }

  template <typename T>
  friend details::tie_enable<T, bool> operator<=(const T& lhs, const T& rhs) {
    return as_tie(lhs) <= as_tie(rhs);
  }

  template <typename T>
  friend details::tie_enable<T, bool> operator>(const T& lhs, const T& rhs) {
    return as_tie(lhs) > as_tie(rhs);
  }

  template <typename T>
  friend details::tie_enable<T, bool> operator>=(const T& lhs, const T& rhs) {
    return as_tie(lhs) >= as_tie(rhs);
  }
};

}  // namespace knot

#endif
