#ifndef KNOT_VISIT_VARIANT
#define KNOT_VISIT_VARIANT

#include <variant>

namespace knot {

// Unfortunately std::visit isn't able to generate code as optimal as hand rolled switch statements
template <typename... Ts, typename F>
decltype(auto) visit_variant(const std::variant<Ts...>& variant, F f) {
  if constexpr (sizeof...(Ts) == 1) {
    return f(*std::get_if<0>(&variant));
  } else if constexpr (sizeof...(Ts) == 2) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
    }
  } else if constexpr (sizeof...(Ts) == 3) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
      case 2:
        return f(*std::get_if<2>(&variant));
    }
  } else if constexpr (sizeof...(Ts) == 4) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
      case 2:
        return f(*std::get_if<2>(&variant));
      case 3:
        return f(*std::get_if<3>(&variant));
    }
  } else if constexpr (sizeof...(Ts) == 5) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
      case 2:
        return f(*std::get_if<2>(&variant));
      case 3:
        return f(*std::get_if<3>(&variant));
      case 4:
        return f(*std::get_if<4>(&variant));
    }
  } else if constexpr (sizeof...(Ts) == 6) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
      case 2:
        return f(*std::get_if<2>(&variant));
      case 3:
        return f(*std::get_if<3>(&variant));
      case 4:
        return f(*std::get_if<4>(&variant));
      case 5:
        return f(*std::get_if<5>(&variant));
    }
  } else if constexpr (sizeof...(Ts) == 7) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
      case 2:
        return f(*std::get_if<2>(&variant));
      case 3:
        return f(*std::get_if<3>(&variant));
      case 4:
        return f(*std::get_if<4>(&variant));
      case 5:
        return f(*std::get_if<5>(&variant));
      case 6:
        return f(*std::get_if<6>(&variant));
    }
  } else if constexpr (sizeof...(Ts) == 8) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
      case 2:
        return f(*std::get_if<2>(&variant));
      case 3:
        return f(*std::get_if<3>(&variant));
      case 4:
        return f(*std::get_if<4>(&variant));
      case 5:
        return f(*std::get_if<5>(&variant));
      case 6:
        return f(*std::get_if<6>(&variant));
      case 7:
        return f(*std::get_if<7>(&variant));
    }
  } else if constexpr (sizeof...(Ts) == 9) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
      case 2:
        return f(*std::get_if<2>(&variant));
      case 3:
        return f(*std::get_if<3>(&variant));
      case 4:
        return f(*std::get_if<4>(&variant));
      case 5:
        return f(*std::get_if<5>(&variant));
      case 6:
        return f(*std::get_if<6>(&variant));
      case 7:
        return f(*std::get_if<7>(&variant));
      case 8:
        return f(*std::get_if<8>(&variant));
    }
  } else if constexpr (sizeof...(Ts) == 10) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
      case 2:
        return f(*std::get_if<2>(&variant));
      case 3:
        return f(*std::get_if<3>(&variant));
      case 4:
        return f(*std::get_if<4>(&variant));
      case 5:
        return f(*std::get_if<5>(&variant));
      case 6:
        return f(*std::get_if<6>(&variant));
      case 7:
        return f(*std::get_if<7>(&variant));
      case 8:
        return f(*std::get_if<8>(&variant));
      case 9:
        return f(*std::get_if<9>(&variant));
    }
  } else if constexpr (sizeof...(Ts) == 11) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
      case 2:
        return f(*std::get_if<2>(&variant));
      case 3:
        return f(*std::get_if<3>(&variant));
      case 4:
        return f(*std::get_if<4>(&variant));
      case 5:
        return f(*std::get_if<5>(&variant));
      case 6:
        return f(*std::get_if<6>(&variant));
      case 7:
        return f(*std::get_if<7>(&variant));
      case 8:
        return f(*std::get_if<8>(&variant));
      case 9:
        return f(*std::get_if<9>(&variant));
      case 10:
        return f(*std::get_if<10>(&variant));
    }
  } else if constexpr (sizeof...(Ts) == 12) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
      case 2:
        return f(*std::get_if<2>(&variant));
      case 3:
        return f(*std::get_if<3>(&variant));
      case 4:
        return f(*std::get_if<4>(&variant));
      case 5:
        return f(*std::get_if<5>(&variant));
      case 6:
        return f(*std::get_if<6>(&variant));
      case 7:
        return f(*std::get_if<7>(&variant));
      case 8:
        return f(*std::get_if<8>(&variant));
      case 9:
        return f(*std::get_if<9>(&variant));
      case 10:
        return f(*std::get_if<10>(&variant));
      case 11:
        return f(*std::get_if<11>(&variant));
    }
  } else if constexpr (sizeof...(Ts) == 13) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
      case 2:
        return f(*std::get_if<2>(&variant));
      case 3:
        return f(*std::get_if<3>(&variant));
      case 4:
        return f(*std::get_if<4>(&variant));
      case 5:
        return f(*std::get_if<5>(&variant));
      case 6:
        return f(*std::get_if<6>(&variant));
      case 7:
        return f(*std::get_if<7>(&variant));
      case 8:
        return f(*std::get_if<8>(&variant));
      case 9:
        return f(*std::get_if<9>(&variant));
      case 10:
        return f(*std::get_if<10>(&variant));
      case 11:
        return f(*std::get_if<11>(&variant));
      case 12:
        return f(*std::get_if<12>(&variant));
    }
  } else if constexpr (sizeof...(Ts) == 14) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
      case 2:
        return f(*std::get_if<2>(&variant));
      case 3:
        return f(*std::get_if<3>(&variant));
      case 4:
        return f(*std::get_if<4>(&variant));
      case 5:
        return f(*std::get_if<5>(&variant));
      case 6:
        return f(*std::get_if<6>(&variant));
      case 7:
        return f(*std::get_if<7>(&variant));
      case 8:
        return f(*std::get_if<8>(&variant));
      case 9:
        return f(*std::get_if<9>(&variant));
      case 10:
        return f(*std::get_if<10>(&variant));
      case 11:
        return f(*std::get_if<11>(&variant));
      case 12:
        return f(*std::get_if<12>(&variant));
      case 13:
        return f(*std::get_if<13>(&variant));
    }
  } else if constexpr (sizeof...(Ts) == 15) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
      case 2:
        return f(*std::get_if<2>(&variant));
      case 3:
        return f(*std::get_if<3>(&variant));
      case 4:
        return f(*std::get_if<4>(&variant));
      case 5:
        return f(*std::get_if<5>(&variant));
      case 6:
        return f(*std::get_if<6>(&variant));
      case 7:
        return f(*std::get_if<7>(&variant));
      case 8:
        return f(*std::get_if<8>(&variant));
      case 9:
        return f(*std::get_if<9>(&variant));
      case 10:
        return f(*std::get_if<10>(&variant));
      case 11:
        return f(*std::get_if<11>(&variant));
      case 12:
        return f(*std::get_if<12>(&variant));
      case 13:
        return f(*std::get_if<13>(&variant));
      case 14:
        return f(*std::get_if<14>(&variant));
    }
  } else if constexpr (sizeof...(Ts) == 16) {
    switch (variant.index()) {
      case 0:
        return f(*std::get_if<0>(&variant));
      case 1:
        return f(*std::get_if<1>(&variant));
      case 2:
        return f(*std::get_if<2>(&variant));
      case 3:
        return f(*std::get_if<3>(&variant));
      case 4:
        return f(*std::get_if<4>(&variant));
      case 5:
        return f(*std::get_if<5>(&variant));
      case 6:
        return f(*std::get_if<6>(&variant));
      case 7:
        return f(*std::get_if<7>(&variant));
      case 8:
        return f(*std::get_if<8>(&variant));
      case 9:
        return f(*std::get_if<9>(&variant));
      case 10:
        return f(*std::get_if<10>(&variant));
      case 11:
        return f(*std::get_if<11>(&variant));
      case 12:
        return f(*std::get_if<12>(&variant));
      case 13:
        return f(*std::get_if<13>(&variant));
      case 14:
        return f(*std::get_if<14>(&variant));
      case 15:
        return f(*std::get_if<15>(&variant));
    }
  } else {
    return std::visit(f, variant);
  }

  // if index == variant::npos
  std::terminate();
}

}  // namespace knot

#endif