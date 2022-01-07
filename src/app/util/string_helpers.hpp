#ifndef ARCHIVER_STRING_HELPERS_HPP
#define ARCHIVER_STRING_HELPERS_HPP

#include <string_view>

namespace {
std::string_view removePrefix(std::string_view str,
                              const std::string_view prefix) {
  if (str.starts_with(prefix))
    str.remove_prefix(prefix.size());
  return str;
}
std::string_view removeSuffix(std::string_view str,
                              const std::string_view suffix) {
  if (str.ends_with(suffix))
    str.remove_suffix(suffix.size());
  return str;
}
}

#endif
