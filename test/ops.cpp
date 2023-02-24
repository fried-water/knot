#include "knot/operators.h"

#include "test_structs.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(Ops_ordered) {
  Pair<int, int> p1{1, 3};
  Pair<int, int> p2{2, 2};

  BOOST_CHECK(p1 == p1);
  BOOST_CHECK(p1 != p2);
  BOOST_CHECK(p1 < p2);
  BOOST_CHECK(p2 > p1);
  BOOST_CHECK(p1 <= p1);
  BOOST_CHECK(p2 >= p2);
}

BOOST_AUTO_TEST_CASE(Ops_rvalues) {
  BOOST_CHECK((Pair<int, int>{1, 1}) == (Pair<int, int>{1, 1}));
  BOOST_CHECK((Pair<int, int>{1, 1}) != (Pair<int, int>{1, 2}));
}
