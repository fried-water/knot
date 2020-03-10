#ifndef KNOT_H
#define KNOT_H

#include "knot_auto_as_tie.h"
#include "knot_ops.h"
#include "knot_type_traits.h"
#include "knot_visit_variant.h"

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
void visit(const T&, F);

template <typename Result, typename T, typename F>
Result accumulate(const T& t, F f, Result acc = {});

// Map must be a copy for now unless objects supply a non-const tie
template <typename Result, typename T, typename F = std::tuple<>>
Result map(const T&, F f = {});

enum class SearchResult {
  Explore,  // Continue exploring children
  Reject,   // Don't explore children
  Finish    // End the search
};

// Returns true if SearchResult::Finish was returned
// before exhausting the search space
template <typename T, typename F>
bool depth_first_search(const T&, F);

namespace details {

// std as_tie() overload
template <typename First, typename Second>
auto as_tie(const std::pair<First, Second>& pair) {
  return std::tie(pair.first, pair.second);
}

template <typename, typename = void>
struct is_tieable : std::false_type {};
template <typename T>
struct is_tieable<T, std::void_t<decltype(as_tie(std::declval<T>()))>> : std::true_type {};
template <typename T>
inline constexpr bool is_tieable_v = is_tieable<T>::value;

// We treat tuples, pairs, and tieable structs as product types
template <typename T>
inline constexpr bool is_product_type_v = is_tieable_v<T> || is_tuple_v<T>;

// We treat optionals, and ptrs as maybe types
template <typename T>
inline constexpr bool is_maybe_type_v = is_optional_v<T> || is_pointer_v<T>;

template <typename T>
inline constexpr bool is_primitive_type_v = std::is_arithmetic_v<T> || std::is_enum_v<T>;

template <typename T>
inline constexpr bool is_knot_supported_type_v =
    is_product_type_v<T> || is_variant_v<T> || is_maybe_type_v<T> || is_range_v<T> || is_primitive_type_v<T>;

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

// Generic maybe type utilities (optional, pointers)

template <typename T>
struct as_optional_type {
  using type = std::optional<std::decay_t<decltype(*std::declval<T>())>>;
};

template <typename T>
using as_optional_type_t = typename as_optional_type<T>::type;

template <typename Result>
std::enable_if_t<is_optional_v<Result>, Result> from_optional(Result&& op) {
  return std::move(op);
}

template <typename Result, typename T>
std::enable_if_t<is_pointer_v<Result>, Result> from_optional(std::optional<T>&& op) {
  return Result{static_cast<bool>(op) ? new T{std::move(*op)} : nullptr};
}

// Monadic optional wrapper for help with deserialize

template <typename T>
struct Monad;

template <typename T>
auto make_monad(T&& t) {
  return Monad<std::decay_t<T>>{std::forward<T>(t)};
}

template <typename First, typename Second>
struct Monad<std::optional<std::pair<First, Second>>> {
  std::optional<std::pair<First, Second>> opt;

  template <typename F>
  auto and_then(F f) && {
    return make_monad(opt ? f(std::move(opt->first), std::move(opt->second)) : std::nullopt);
  }

  template <typename F>
  auto map(F f) && {
    return make_monad(opt ? std::make_optional(f(std::move(opt->first), std::move(opt->second))) : std::nullopt);
  }

  operator std::optional<std::pair<First, Second>>() && { return std::move(opt); }
  std::optional<std::pair<First, Second>> get() && { return std::move(opt); }
};

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
std::optional<std::pair<Variant, IT>> variant_deserialize(IT begin, IT end, std::size_t index,
                                                          std::index_sequence<Is...>) {
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
}

template <typename Result, typename T, std::size_t... Is>
Result map_tuple(const T& t, std::index_sequence<Is...>) {
  using ResultTuple = std::decay_t<decltype(as_tie(std::declval<Result>()))>;
  auto tuple = as_tie(t);
  return Result{map<std::decay_t<std::tuple_element_t<Is, ResultTuple>>>(std::get<Is>(tuple))...};
}

template <typename T, typename = void>
struct can_visit : std::false_type {};

template <typename Visitor, typename... Args>
struct can_visit<std::tuple<Visitor, Args...>, std::void_t<decltype(std::declval<Visitor>()(std::declval<Args>()...))>>
    : std::true_type {};

template <typename Visitor, typename... Args>
inline constexpr bool can_visit_v = can_visit<std::tuple<Visitor, Args...>>::value;

template <typename Result, typename Visitor, typename T, typename = void>
struct evaluate_result {
  using type = Result;
};

template <typename Result, typename Visitor, typename T>
using evaluate_result_t = typename evaluate_result<Result, Visitor, T>::type;

template <typename Result, typename Visitor, typename T>
struct evaluate_result<Result, Visitor, T, std::enable_if_t<can_visit_v<Visitor, T>>> {
  using type = std::decay_t<decltype(std::declval<Visitor>()(std::declval<T>()))>;
};

template <typename Result, typename Visitor, typename T>
struct evaluate_result<Result, Visitor, std::optional<T>, std::enable_if_t<!can_visit_v<Visitor, std::optional<T>>>> {
  using type = std::optional<evaluate_result_t<Result, Visitor, T>>;
};

template <typename Result, typename Visitor, typename T1, typename T2>
struct evaluate_result<Result, Visitor, std::pair<T1, T2>, std::enable_if_t<!can_visit_v<Visitor, std::pair<T1, T2>>>> {
  using type = std::tuple<evaluate_result_t<Result, Visitor, T1>, evaluate_result_t<Result, Visitor, T2>>;
};

template <typename Result, typename Visitor, typename... Ts>
struct evaluate_result<Result, Visitor, std::tuple<Ts...>, std::enable_if_t<!can_visit_v<Visitor, std::tuple<Ts...>>>> {
  using type = std::tuple<evaluate_result_t<Result, Visitor, Ts>...>;
};

template <typename Result, typename Visitor, typename T>
struct evaluate_result<Result, Visitor, T, std::enable_if_t<is_pointer_v<T> && !can_visit_v<Visitor, T>>> {
  using type = evaluate_result_t<Result, Visitor, std::decay_t<decltype(*std::declval<T>())>>;
};

template <typename Result, typename Visitor, typename T>
struct evaluate_result<Result, Visitor, T, std::enable_if_t<is_range_v<T> && !can_visit_v<Visitor, T>>> {
  using type = std::vector<evaluate_result_t<Result, Visitor, typename T::value_type>>;
};

template <typename Result, typename Visitor, typename... Ts>
struct evaluate_result<Result, Visitor, std::variant<Ts...>,
                       std::enable_if_t<!can_visit_v<Visitor, std::variant<Ts...>>>> {
  using type = std::common_type_t<evaluate_result_t<Result, Visitor, Ts>...>;
};

template <typename Result, typename T, typename Visitor, typename... Ts, std::size_t... Is>
auto evaluate_expand_tuple(const T& t, Visitor visitor, std::tuple<Ts...>&& tuple, std::index_sequence<Is...>) {
  if constexpr (can_visit_v<Visitor, T, Ts...>) {
    return visitor(t, std::get<Is>(std::move(tuple))...);
  } else {
    return std::move(tuple);
  }
}

template <typename Result, typename T, typename Visitor, typename Eval, std::size_t... Is>
auto evaluate_tuple(const T& t, Visitor visitor, Eval eval, std::index_sequence<Is...>) {
  const auto& tuple = as_tuple(t);
  return evaluate_expand_tuple<Result>(t, visitor, std::make_tuple(eval(std::get<Is>(tuple))...),
                                       std::make_index_sequence<sizeof...(Is)>());
}

template <typename T, typename F>
std::pair<bool, int> dfs_internal(const T& t, F f, int id, int parent);

template <typename T, typename F, std::size_t... Is>
std::pair<bool, int> dfs_tuple(const T& tuple, F f, int id, int parent, std::index_sequence<Is...>) {
  static constexpr std::array<std::pair<bool, int> (*)(const T&, F, int, int), sizeof...(Is)> funcs{
      +[](const T& tuple, F f, int id, int parent) { return dfs_internal(std::get<Is>(tuple), f, id, parent); }...};
  std::pair<bool, int> result{false, id};
  for (int i = 0; i < sizeof...(Is); i++) {
    result = funcs[i](tuple, f, result.second, parent);
    if (result.first) return result;
  }
  return result;
}

template <typename T, typename F>
std::pair<bool, int> dfs_internal(const T& t, F f, int id, int parent) {
  if constexpr (can_visit_v<F, T, int, int>) {
    SearchResult result = f(t, id++, parent);

    if (result == SearchResult::Finish)
      return {true, id};
    else if (result == SearchResult::Reject)
      return {false, id};

    parent = id - 1;
  }

  if constexpr (is_product_type_v<T>) {
    return dfs_tuple(as_tuple(t), f, id, parent, std::make_index_sequence<std::tuple_size_v<as_tuple_type_t<T>>>());
  } else if constexpr (is_variant_v<T>) {
    return std::visit([&](const auto& val) { return dfs_internal(val, f, id, parent); }, t);
  } else if constexpr (is_maybe_type_v<T>) {
    return static_cast<bool>(t) ? dfs_internal(*t, f, id, parent) : std::pair<bool, int>{false, id};
  } else if constexpr (is_range_v<T>) {
    std::pair<bool, int> result{false, id};
    for (const auto& val : t) {
      result = dfs_internal(val, f, result.second, parent);
      if (result.first) return result;
    }
    return result;
  } else {
    return {false, id};
  }
}

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
                    [&](IT out, const auto& value) {
                      using T = std::decay_t<decltype(value)>;
                      if constexpr (details::is_primitive_type_v<T>) {
                        return serialize_primitive(value, out);
                      } else if constexpr (details::is_variant_v<T>) {
                        return serialize_primitive(value.index(), out);
                      } else if constexpr (details::is_maybe_type_v<T>) {
                        return serialize_primitive(static_cast<bool>(value), out);
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

  static_assert(!std::is_reference_v<Outer>);
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
  } else if constexpr (details::is_variant_v<T>) {
    return details::make_monad(deserialize_partial<std::size_t>(begin, end))
        .and_then([end](std::size_t index, IT begin) {
          return details::variant_deserialize<T>(begin, end, index, std::make_index_sequence<std::variant_size_v<T>>());
        });
  } else if constexpr (details::is_maybe_type_v<T>) {
    using optional_t = details::as_optional_type_t<T>;
    return details::make_monad(deserialize_partial<bool>(begin, end))
        .and_then([end](bool has_value, IT begin) -> std::optional<std::pair<optional_t, IT>> {
          if (has_value) {
            return details::make_monad(deserialize_partial<typename optional_t::value_type>(begin, end))
                .map(
                    [](auto&& inner, IT begin) { return std::make_pair(std::make_optional(std::move(inner)), begin); });
          } else {
            return std::make_pair(std::nullopt, begin);
          }
        })
        .map([](optional_t&& optional, IT begin) {
          return std::make_pair(details::from_optional<T>(std::move(optional)), begin);
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
  } else if constexpr (details::is_variant_v<T>) {
    os << "<";
    std::visit([&os](const auto& inner) { debug_string(os, inner); }, t);
    return os << ">";
  } else if constexpr (details::is_maybe_type_v<T>) {
    return static_cast<bool>(t) ? debug_string(os, *t) : os << "none";
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
  return accumulate<std::size_t>(t, [&](const std::size_t hash, const auto& value) {
    using T = std::decay_t<decltype(value)>;
    if constexpr (details::is_primitive_type_v<T> ||
                  (!details::is_knot_supported_type_v<T> && details::is_std_hashable_v<T>)) {
      return details::hash_combine(hash, std::hash<T>{}(value));
    } else if constexpr (details::is_variant_v<T>) {
      return details::hash_combine(hash, value.index());
    } else if constexpr (details::is_maybe_type_v<T>) {
      return details::hash_combine(hash, static_cast<bool>(value));
    } else {
      return hash;
    }
  });
}

template <typename T, typename Visitor>
void visit(const T& t, Visitor visitor) {
  if constexpr (details::can_visit_v<Visitor, T>) {
    // visit the value, if the function returns a value, stop recursing on false
    if constexpr (std::is_same_v<void, decltype(visitor(t))>) {
      visitor(t);
    } else {
      if (!visitor(t)) return;
    }
  }

  // Call visit recursively depending on type
  if constexpr (details::is_product_type_v<T>) {
    details::visit_tuple(details::as_tuple(t), visitor,
                         std::make_index_sequence<std::tuple_size_v<details::as_tuple_type_t<T>>>());
  } else if constexpr (details::is_variant_v<T>) {
    std::visit([&](const auto& val) { visit(val, visitor); }, t);
  } else if constexpr (details::is_maybe_type_v<T>) {
    if (static_cast<bool>(t)) visit(*t, visitor);
  } else if constexpr (details::is_range_v<T>) {
    for (const auto& val : t) visit(val, visitor);
  }
}

template <typename Result, typename T, typename F>
Result accumulate(const T& t, F f, Result acc) {
  visit(t, [&](const auto& value) {
    if constexpr (details::can_visit_v<F, Result, decltype(value)>) {
      acc = f(std::move(acc), value);
    }
  });
  return acc;
}

template <typename Result, typename T, typename F>
Result map(const T& t, F f) {
  // Either these is an override or type category needs to align
  static_assert(details::can_visit_v<F, T> || (
    details::is_primitive_type_v<Result> == details::is_primitive_type_v<T>
    && details::is_product_type_v<Result> == details::is_product_type_v<T>
    && details::is_variant_v<Result> == details::is_variant_v<T>
    && details::is_maybe_type_v<Result> == details::is_maybe_type_v<T>
    && details::is_range_v<Result> == details::is_range_v<T>));

  // Visitors cannot take object by non-const lvalue
  if constexpr (details::can_visit_v<F, T>) {
    return static_cast<Result>(f(t));
  } else if constexpr (details::is_primitive_type_v<Result>) {
    return static_cast<Result>(t);
  } else if constexpr (details::is_product_type_v<Result>) {
    constexpr std::size_t SrcSize = std::tuple_size_v<std::decay_t<decltype(details::as_tuple(std::declval<Result>()))>>;
    constexpr std::size_t DstSize = std::tuple_size_v<std::decay_t<decltype(details::as_tuple(std::declval<T>()))>>;

    static_assert(SrcSize == DstSize);

    return details::map_tuple<Result>(t, std::make_index_sequence<SrcSize>());
  } else if constexpr (details::is_variant_v<Result>) {
    // can only map from equivalent variants or if the Result variant is a superset of types
    return std::visit([](const auto& val){
      return Result{val};
    }, t);
  } else if constexpr (details::is_maybe_type_v<Result>) { // TODO
    static_assert(!details::is_maybe_type_v<Result>);
  } else if constexpr (details::is_range_v<Result>) {
    Result range{};
    if constexpr (details::is_reserveable_v<Result>) range.reserve(std::distance(std::begin(t), std::end(t)));

    using DstType = typename Result::value_type;

    int i = 0;
    // TODO bound check result arrays
    for(const auto& val : t) {
      if constexpr (details::is_array_v<Result>) {
        range[i++] = map<DstType>(val);
      } else {
        range.insert(range.end(), map<DstType>(val));
      }
    }

    return range;
  }
}

template <typename Result, typename T, typename Visitor>
details::evaluate_result_t<Result, Visitor, T> evaluate(const T& t, Visitor visitor) {
  static_assert(details::is_knot_supported_type_v<T> || details::can_visit_v<Visitor, T>);

  auto eval = [&visitor](const auto& t) { return evaluate<Result>(t, visitor); };

  // Allow visitor to override and prevent recursion for specific types (even knot unsupported types)
  if constexpr (details::can_visit_v<Visitor, T>) {
    return visitor(t);
  } else if constexpr (details::can_visit_v<Visitor, T, decltype(eval)>) {
    return visitor(t, eval);
  } else if constexpr (details::is_primitive_type_v<T>) {
    static_assert(std::is_same_v<T, Result>);
    return t;
  } else if constexpr (details::is_product_type_v<T>) {
    return details::evaluate_tuple<Result>(
        t, visitor, eval, std::make_index_sequence<std::tuple_size_v<std::decay_t<decltype(details::as_tuple(t))>>>{});
  } else if constexpr (details::is_variant_v<T>) {
    return visit_variant(t,
                         [&](const auto& val) -> details::evaluate_result_t<Result, Visitor, T> { return eval(val); });
  } else if constexpr (details::is_optional_v<T>) {
    return t ? std::make_optional(eval(*t)) : std::nullopt;
  } else if constexpr (details::is_pointer_v<T>) {
    // Special case pointers to not act like maybe types in evaluate() (aka assume not null)
    // Recursive structs that you would want to evaluate require indirection to implement
    // having every pointer require a null check would be less performant and less
    // ergonomic since you would have to take an optional<Result> everytime
    return evaluate<Result>(*t, visitor);
  } else if constexpr (details::is_range_v<T>) {
    details::evaluate_result_t<Result, Visitor, T> result_vec;
    result_vec.reserve(std::distance(std::begin(t), std::end(t)));

    for (const auto& val : t) {
      result_vec.push_back(eval(val));
    }

    return result_vec;

  } else {
    return details::evaluate_result_t<Result, Visitor, T>{};
  }
}

template <typename T, typename F>
bool depth_first_search(const T& t, F f) {
  return details::dfs_internal(t, f, 0, -1).first;
}

// std::hash<T> replacement for tieable types
struct Hash {
  template <typename T>
  std::size_t operator()(const T& t) const {
    return knot::hash_value(t);
  }
};

}  // namespace knot

#endif
