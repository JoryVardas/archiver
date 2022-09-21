#include "additional_generators.hpp"
#include <concepts>
#include <ranges>

RandomStringGenerator::RandomStringGenerator(
  uint64_t minLength, uint64_t maxLength, const std::vector<char>& validChars,
  std::uint32_t seed)
  : rng(seed), charDistribution(0, 128),
    lengthDistribution(minLength, maxLength), validCharacters(validChars) {
  currentString.reserve(maxLength);
  static_cast<void>(next());
}

std::string const& RandomStringGenerator::get() const { return currentString; }
bool RandomStringGenerator::next() {
  auto length = lengthDistribution(rng);
  currentString.clear();

  for (uint64_t i = 0; i < length; ++i) {
    char cur = '\n';
    while (std::ranges::find(validCharacters, cur) ==
           std::ranges::end(validCharacters)) {
      cur = static_cast<char>(charDistribution(rng));
    }

    currentString.push_back(cur);
  }

  return true;
}

Catch::Generators::GeneratorWrapper<std::string>
randomString(uint64_t minLength, uint64_t maxLength,
             const std::vector<char>& validChars) {
  return Catch::Generators::GeneratorWrapper<std::string>(
    Catch::Detail::make_unique<RandomStringGenerator>(
      minLength, maxLength, validChars, Catch::sharedRng()()));
}
Catch::Generators::GeneratorWrapper<std::string>
randomFilename(uint64_t minLength, uint64_t maxLength) {
  return Catch::Generators::GeneratorWrapper<std::string>(
    Catch::Detail::make_unique<RandomStringGenerator>(
      minLength, maxLength, filenameCharacters, Catch::sharedRng()()));
}

const std::vector<char> allStringCharacters = {
  ' ', '!', '"', '#', '$',  '%', '&', '\'', '(', ')', '*', '+', ',', '-',
  '.', '/', '0', '1', '2',  '3', '4', '5',  '6', '7', '8', '9', ':', ';',
  '<', '=', '>', '?', '@',  'A', 'B', 'C',  'D', 'E', 'F', 'G', 'H', 'I',
  'J', 'K', 'L', 'M', 'N',  'O', 'P', 'Q',  'R', 'S', 'T', 'U', 'V', 'W',
  'X', 'Y', 'Z', '[', '\\', ']', '^', '_',  '`', 'a', 'b', 'c', 'd', 'e',
  'f', 'g', 'h', 'i', 'j',  'k', 'l', 'm',  'n', 'o', 'p', 'q', 'r', 's',
  't', 'u', 'v', 'w', 'x',  'y', 'z', '{',  '|', '}', '~'};
const std::vector<char> filenameCharacters = {
  ' ', '!', '#', '$', '%', '&', '\'', '(', ')', '+', ',', '-', '.', '0', '1',
  '2', '3', '4', '5', '6', '7', '8',  '9', ';', '=', '@', 'A', 'B', 'C', 'D',
  'E', 'F', 'G', 'H', 'I', 'J', 'K',  'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
  'T', 'U', 'V', 'W', 'X', 'Y', 'Z',  '[', ']', '^', '_', '`', 'a', 'b', 'c',
  'd', 'e', 'f', 'g', 'h', 'i', 'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
  's', 't', 'u', 'v', 'w', 'x', 'y',  'z', '{', '}', '~'};