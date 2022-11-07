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

template <typename T> class EnumerateViewTransformer {
private:
  int i = 0;

public:
  auto operator()(T val) { return std::pair<int, T>{i++, val}; }
};
template <typename T>
auto const enumerateView =
  std::ranges::views::transform(EnumerateViewTransformer<T>{});

template <typename T, typename Func>
auto forEachNotLast(T container, Func&& function) {
  return std::for_each_n(std::begin(container), std::size(container) - 1,
                         std::forward<Func>(function));
}
template <typename T, typename Func>
auto forEachNotLastIter(T container, Func&& function) {
  auto iter = std::begin(container);
  auto end = std::begin(container);
  std::advance(end, std::size(container) - 1);
  for (; iter != end; ++iter) {
    function(iter);
  }
  return iter;
}

namespace {
std::string
getPathElement(const std::filesystem::path& path,
               const std::filesystem::path::iterator::difference_type index) {
  auto iter = std::begin(path);
  std::advance(iter, index);
  return *iter;
}

std::size_t getNumberPathElements(const std::filesystem::path& path) {
  return std::distance(std::cbegin(path), std::cend(path));
}
}

auto getLastElement(HasForwardIterator auto& container) {
  auto iter = std::begin(container);
  std::advance(iter,
               std::distance(std::begin(container), std::end(container)) - 1);
  return *iter;
}

#endif
