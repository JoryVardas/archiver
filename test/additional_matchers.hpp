#ifndef ARCHIVER_TEST_ADDITIONAL_MATCHERS_HPP
#define ARCHIVER_TEST_ADDITIONAL_MATCHERS_HPP

#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <string>

struct MessageStartsWithMatcher : Catch::Matchers::MatcherGenericBase {
  MessageStartsWithMatcher(const std::string& prefixToMatch);
  bool match(const std::exception& err) const;
  auto describe() const -> std::string;

private:
  std::string prefix;
};

auto MessageStartsWith(const std::string& prefixToMatch)
  -> MessageStartsWithMatcher;

#endif
