#ifndef ARCHIVER_HELPER_FUNCTIONS_HPP
#define ARCHIVER_HELPER_FUNCTIONS_HPP

#include "helper_concepts.hpp"
#include <concepts>
#include <functional>
#include <numeric>
#include <ranges>
#include <string>

template <Container T>
std::string joinStrings(T& list, std::string_view joiner,
                        typename T::size_type skip = 0) {
  using namespace std::string_literals;

  if (list.size() == 0 || skip >= list.size())
    return ""s;
  return std::accumulate(
    std::begin(list) + 1 + skip, std::end(list), *(std::begin(list) + skip),
    [](std::string a, const std::string& b) { return std::move(a) + "/" + b; });
}

template <typename T>
auto const enumerateView =
  std::ranges::views::transform([i = 0](T val) mutable {
    return std::pair<int, T>{i++, val};
  });

#endif
