#ifndef ARCHIVER_TEST_ADDITIONAL_GENERATORS_HPP
#define ARCHIVER_TEST_ADDITIONAL_GENERATORS_HPP

#include <catch2/generators/catch_generators.hpp>
#include <catch2/internal/catch_context.hpp>
#include <catch2/internal/catch_random_number_generator.hpp>
#include <random>
#include <string>

class RandomStringGenerator final
  : public Catch::Generators::IGenerator<std::string> {
  Catch::SimplePcg32 rng;
  std::uniform_int_distribution<uint8_t> charDistribution;
  std::uniform_int_distribution<uint64_t> lengthDistribution;
  std::string currentString;
  std::vector<char> validCharacters;

public:
  RandomStringGenerator(uint64_t minLength, uint64_t maxLength,
                        const std::vector<char>& validChars,
                        std::uint32_t seed);

  std::string const& get() const override;
  bool next() override;
};

Catch::Generators::GeneratorWrapper<std::string>
randomString(uint64_t minLength, uint64_t maxLength,
             const std::vector<char>& validChars);

Catch::Generators::GeneratorWrapper<std::string>
randomFilename(uint64_t minLength, uint64_t maxLength);

extern const std::vector<char> allStringCharacters;
extern const std::vector<char> filenameCharacters;
#endif
