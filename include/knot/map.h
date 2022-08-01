#pragma once

#include "knot/type_category.h"
#include "knot/type_traits.h"

namespace knot {

template <typename Result, typename T, typename F = std::tuple<>>
Result map(Type<Result>, T&&, F = {});

template <typename Result, typename T, typename F = std::tuple<>>
Result map(T&& t, F f = {}) {
  return map(Type<Result>{}, std::forward<T>(t), std::move(f));
}

namespace details {

template <typename Result, typename Tuple, typename F, typename T, std::size_t... Is>
Result map_tuple(Type<Tuple> tuple_type, T&& t, F f, std::index_sequence<Is...>) {
  static_assert(tuple_size(tuple_type) == sizeof...(Is));
  return Result{map(tuple_element<Is>(tuple_type), std::get<Is>(std::forward<T>(t)), f)...};
}

}  // namespace details

template <typename Result, typename T, typename F>
Result map(Type<Result> const_result_type, T&& t, F f) {
  constexpr auto type = decay(Type<T>{});
  constexpr auto result_type = remove_const(Type<Result>{});

  // Type category needs to align
  static_assert(result_type == invoke_result(Type<F>{}, typelist(type)) || category(result_type) == category(type));

  static_assert(is_decayed(result_type));
  static_assert(!is_raw_pointer(result_type));

  if constexpr (result_type == invoke_result(Type<F>{}, typelist(type))) {
    return f(std::forward<T>(t));
  } else if constexpr (type == result_type) {
    return std::forward<T>(t);
  } else if constexpr (is_tieable(type)) {
    return map(result_type, as_tie(std::forward<T>(t)), f);
  } else if constexpr (is_tieable(result_type)) {
    if constexpr (is_tuple_like(tie_type(result_type))) {
      return details::map_tuple<Result>(tie_type(result_type), std::forward<T>(t), f, idx_seq(tie_type(result_type)));
    } else {
      return Result{map(tie_type(result_type), std::forward<T>(t), f)};
    }
  } else if constexpr (category(result_type) == TypeCategory::Primative) {
    return static_cast<Result>(t);
  } else if constexpr (category(result_type) == TypeCategory::Product) {
    return details::map_tuple<Result>(result_type, std::forward<T>(t), f, idx_seq(result_type));
  } else if constexpr (category(result_type) == TypeCategory::Sum) {
    // can only map from equivalent variants or if the Result variant is a superset of types
    return std::visit(
        [&](auto&& val) -> Result {
          if constexpr (is_invocable(Type<F>{}, typelist(decay(Type<decltype(val)>{})))) {
            return f(std::forward<decltype(val)>(val));
          } else {
            return std::forward<decltype(val)>(val);
          }
        },
        std::forward<T>(t));
  } else if constexpr (is_pointer(result_type)) {
    using ElementT = typename Result::element_type;
    return t ? Result{new ElementT{map(Type<ElementT>{}, *std::forward<T>(t), f)}} : nullptr;
  } else if constexpr (is_optional(result_type)) {
    return t ? Result{map(value_type(result_type), *std::forward<T>(t), f)} : std::nullopt;
  } else if constexpr (category(result_type) == TypeCategory::Range) {
    Result range{};

    if constexpr (is_valid([](auto&& t) -> decltype(t.reserve(0)) {})(result_type)) {
      range.reserve(std::distance(std::begin(t), std::end(t)));
    }

    int i = 0;
    // TODO bound check result arrays
    if constexpr (is_ref(Type<T>{})) {
      for (const auto& val : t) {
        if constexpr (is_array(result_type)) {
          range[i++] = map(value_type(result_type), val, f);
        } else {
          range.insert(range.end(), map(value_type(result_type), val, f));
        }
      }
    } else {
      for (auto&& val : t) {
        if constexpr (is_array(result_type)) {
          range[i++] = map(value_type(result_type), std::move(val), f);
        } else {
          range.insert(range.end(), map(value_type(result_type), std::move(val), f));
        }
      }
    }

    return range;
  }
}

}  // namespace knot