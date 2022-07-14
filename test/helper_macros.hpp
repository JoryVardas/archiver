#ifndef ARCHIVER_TEST_HELPER_MATCHERS_HPP
#define ARCHIVER_TEST_HELPER_MATCHERS_HPP

#include <catch2/catch_test_macros.hpp>

#define LAMBDA_WRAPPER(expression, result) [&]() { result = expression; }()
#define REQUIRE_NOTHROW_RETURN(expression)                                     \
  [&]() {                                                                      \
    decltype(expression) ret;                                                  \
    REQUIRE_NOTHROW(LAMBDA_WRAPPER(expression, ret));                          \
    return ret;                                                                \
  }()

#endif
