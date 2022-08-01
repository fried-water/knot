#pragma once

#include "knot/map.h"
#include "knot/traversals.h"
#include "knot/type_category.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

namespace knot {

template <typename T>
std::vector<std::byte> serialize(const T&);

template <typename T, typename IT>
IT serialize(const T&, IT out);

// In addition to as_tie(), deserialize requires that structs be constructible
// from the types returned in as_tie().
// Furthermore raw pointers and references aren't supported.
template <typename T, typename IT>
std::optional<T> deserialize(IT begin, IT end);

template <typename T, typename IT>
std::optional<std::pair<T, IT>> deserialize_partial(IT begin, IT end);

namespace details {

// Generic maybe type utilities (optional, pointers)

template <typename Result, typename T>
Result from_optional(Type<Result> result_type, std::optional<T>&& op) {
  if constexpr (is_optional(result_type)) {
    return std::move(op);
  } else if constexpr (is_pointer(result_type)) {
    return Result{static_cast<bool>(op) ? new T{std::move(*op)} : nullptr};
  }
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
    return make_monad(opt ? std::optional(f(std::move(opt->first), std::move(opt->second))) : std::nullopt);
  }
};

// deserialize helpers

template <typename... Ts, typename IT>
std::optional<std::pair<std::tuple<Ts...>, IT>> tuple_deserialize(Type<std::tuple<Ts...>>, IT begin, IT end) {
  if constexpr (sizeof...(Ts) == 0) {
    static_cast<void>(end);  // To suppress warnings about not touching end on this branch
    return std::pair(std::tuple(), begin);
  } else {
    constexpr auto tl = typelist(Type<Ts>{}...);

    return make_monad(deserialize_partial(head(tl), begin, end))
      .and_then([&](auto&& first, IT begin) {
        return make_monad(tuple_deserialize(as_tuple(tail(tl)), begin, end))
          .map([&](auto&& rest, IT begin) {
            return std::pair(std::tuple<Ts...>{std::tuple_cat(std::make_tuple(std::move(first)), std::move(rest))},
                            begin);
          })
          .opt;
      })
      .opt;
  }
}

template <typename T, typename IT>
std::optional<std::pair<T, IT>> tuple_deserialize(Type<T> type, IT begin, IT end) {
  return details::make_monad(deserialize_partial(as_tuple(as_typelist(type)), begin, end))
      .map([](auto&& tuple, IT begin) { return std::pair(map<T>(std::move(tuple)), begin); })
      .opt;
}

template <typename IT, typename... Ts>
std::optional<std::pair<std::variant<Ts...>, IT>> variant_deserialize(Type<std::variant<Ts...>>, IT begin, IT end,
                                                                      std::size_t index) {
  static constexpr auto options = std::array{+[](IT begin, IT end) {
    return make_monad(deserialize_partial(Type<Ts>{}, begin, end))
        .map([](auto&& ele, IT begin) { return std::pair(std::variant<Ts...>{std::move(ele)}, begin); })
        .opt;
  }...};
  return index >= sizeof...(Ts) ? std::nullopt : options[index](begin, end);
}

}  // namespace details

template <typename T>
std::vector<std::byte> serialize(const T& t) {
  std::vector<std::byte> buf;
  serialize(t, std::back_inserter(buf));
  return buf;
}

template <typename T, typename IT>
IT serialize(const T& t, IT it) {
  constexpr Type<T> type = {};

  static_assert(is_supported(type), "Unsupported type in serialize");

  if constexpr (is_tieable(type)) {
    return serialize(as_tie(t), it);
  } else if constexpr (category(type) == TypeCategory::Primative) {
    std::array<std::byte, sizeof(T)> array;
    std::memcpy(array.data(), &t, sizeof(T));
    return std::transform(array.begin(), array.end(), it, [](std::byte b) {
      if constexpr (is_valid([](auto&& it) -> decltype(*it = std::byte{}) {})(Type<IT>{})) {
        return b;
      } else {
        return static_cast<uint8_t>(b);
      }
    });
  } else if constexpr (category(type) == TypeCategory::Sum) {
    return accumulate<IT>(
        t, [&](IT it, const auto& ele) { return serialize(ele, it); }, serialize(t.index(), it));
  } else if constexpr (category(type) == TypeCategory::Maybe) {
    return accumulate<IT>(
        t, [&](IT it, const auto& ele) { return serialize(ele, it); }, serialize(static_cast<bool>(t), it));
  } else if constexpr (category(type) == TypeCategory::Range) {
    return accumulate<IT>(
        t, [&](IT it, const auto& ele) { return serialize(ele, it); }, serialize(t.size(), it));
  } else if constexpr (category(type) == TypeCategory::Product) {
    return accumulate<IT>(
        t, [&](IT it, const auto& ele) { return serialize(ele, it); }, it);
  } else {
    return it;
  }
}

template <typename T, typename IT>
std::optional<T> deserialize(IT begin, IT end) {
  auto opt = deserialize_partial<T>(begin, end);

  if (!opt || opt->second != end) return std::nullopt;

  return std::move(opt->first);
}

template <typename Outer, typename IT>
std::optional<std::pair<Outer, IT>> deserialize_partial(Type<Outer> outer_type, IT begin, IT end) {
  using T = std::remove_const_t<Outer>;

  constexpr Type<T> type = {};
  constexpr auto it_type = decay(Type<decltype(*begin)>{});

  static_assert(is_supported(type) && !is_ref(type) && !is_raw_pointer(type));
  static_assert(it_type == Type<uint8_t>{} || it_type == Type<int8_t>{} || it_type == Type<std::byte>{});

  if constexpr (is_tieable(type)) {
    return details::make_monad(deserialize_partial(tie_type(type), begin, end))
        .map([](auto tied_type, IT begin) { return std::pair(map<T>(std::move(tied_type)), begin); })
        .opt;
  } else if constexpr (category(type) == TypeCategory::Primative) {
    if (std::distance(begin, end) < sizeof(T)) return std::nullopt;

    std::array<std::byte, sizeof(T)> array;
    std::transform(begin, begin + sizeof(T), array.begin(), [](auto b) { return std::byte{static_cast<uint8_t>(b)}; });

    T t;
    std::memcpy(&t, array.data(), sizeof(T));

    return std::pair<T, IT>{t, begin + sizeof(T)};
  } else if constexpr (category(type) == TypeCategory::Sum) {
    return details::make_monad(deserialize_partial(Type<std::size_t>{}, begin, end))
        .and_then([&](std::size_t index, IT begin) { return details::variant_deserialize(type, begin, end, index); })
        .opt;
  } else if constexpr (category(type) == TypeCategory::Maybe) {
    using optional_t = std::optional<std::decay_t<decltype(*std::declval<T>())>>;
    return details::make_monad(deserialize_partial(Type<bool>{}, begin, end))
        .and_then([end](bool has_value, IT begin) -> std::optional<std::pair<optional_t, IT>> {
          if (has_value) {
            return details::make_monad(deserialize_partial(Type<typename optional_t::value_type>{}, begin, end))
                .map([](auto&& inner, IT begin) { return std::pair(std::make_optional(std::move(inner)), begin); })
                .opt;
          } else {
            return std::pair(std::nullopt, begin);
          }
        })
        .map([&](optional_t&& optional, IT begin) {
          return std::pair(details::from_optional(type, std::move(optional)), begin);
        })
        .opt;
  } else if constexpr (category(type) == TypeCategory::Range) {
    return details::make_monad(deserialize_partial(Type<std::size_t>{}, begin, end))
        .map([&](std::size_t size, IT begin) -> std::optional<std::pair<T, IT>> {
          T range{};
          if constexpr (is_valid([](auto&& t) -> decltype(t.reserve(0)) {})(type)) range.reserve(size);

          for (std::size_t i = 0; i < size; i++) {
            auto ele_opt = deserialize_partial(Type<typename T::value_type>{}, begin, end);
            if (!ele_opt) return std::nullopt;
            if constexpr (is_array(type)) {
              range[i] = std::move(ele_opt->first);
            } else {
              range.insert(range.end(), std::move(ele_opt->first));
            }
            begin = ele_opt->second;
          }

          return std::pair<T, IT>{std::move(range), begin};
        })
        .opt;
  } else if constexpr (category(type) == TypeCategory::Product) {
    return details::tuple_deserialize(type, begin, end);
  } else {
    return std::nullopt;
  }
}

template <typename Outer, typename IT>
std::optional<std::pair<Outer, IT>> deserialize_partial(IT begin, IT end) {
  return deserialize_partial(Type<Outer>{}, begin, end);
}

}  // namespace knot