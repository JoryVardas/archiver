#include "additional_matchers.hpp"

MessageStartsWithMatcher::MessageStartsWithMatcher(
  const std::string& prefixToMatch)
  : prefix(prefixToMatch) {}
bool MessageStartsWithMatcher::match(const std::exception& err) const {
  std::string_view message = err.what();
  return message.starts_with(prefix);
}
auto MessageStartsWithMatcher::describe() const -> std::string {
  return "Exception message starts with: " + prefix;
}

auto MessageStartsWith(const std::string& prefixToMatch)
  -> MessageStartsWithMatcher {
  return MessageStartsWithMatcher(prefixToMatch);
}