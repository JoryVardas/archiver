#ifndef ARCHIVER_TEST_ADDITIONAL_GENERATORS_HPP
#define ARCHIVER_TEST_ADDITIONAL_GENERATORS_HPP

#include <catch2/generators/catch_generators.hpp>
#include <catch2/internal/catch_context.hpp>
#include <catch2/internal/catch_random_number_generator.hpp>
#include <filesystem>
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

  ~RandomStringGenerator() = default;
  RandomStringGenerator() = delete;
  RandomStringGenerator(const RandomStringGenerator&) = delete;
  RandomStringGenerator(RandomStringGenerator&&) = default;
  RandomStringGenerator& operator=(RandomStringGenerator&&) = default;
  RandomStringGenerator& operator=(const RandomStringGenerator&) = delete;
};

Catch::Generators::GeneratorWrapper<std::string>
randomString(uint64_t minLength, uint64_t maxLength,
             const std::vector<char>& validChars);

Catch::Generators::GeneratorWrapper<std::string>
randomFilename(uint64_t minLength, uint64_t maxLength);

extern const std::vector<char> allStringCharacters;
extern const std::vector<char> filenameCharacters;

class RandomFilePathGenerator final
  : public Catch::Generators::IGenerator<std::filesystem::path> {
  Catch::SimplePcg32 rng;
  uint64_t minComponents;
  uint64_t maxComponents;
  std::uniform_int_distribution<uint64_t> lengthDistribution;
  std::filesystem::path currentPath;
  Catch::Generators::GeneratorWrapper<std::string> componentGenerator;

public:
  RandomFilePathGenerator(
    uint64_t minNumberComponents, uint64_t maxNumberComponents,
    Catch::Generators::GeneratorWrapper<std::string>&& componentStringGenerator,
    std::uint32_t seed);

  std::filesystem::path const& get() const override;
  bool next() override;

  ~RandomFilePathGenerator() = default;
  RandomFilePathGenerator() = delete;
  RandomFilePathGenerator(const RandomFilePathGenerator&) = delete;
  RandomFilePathGenerator(RandomFilePathGenerator&&) = default;
  RandomFilePathGenerator& operator=(RandomFilePathGenerator&&) = default;
  RandomFilePathGenerator& operator=(const RandomFilePathGenerator&) = delete;
};

Catch::Generators::GeneratorWrapper<std::filesystem::path> randomFilePath(
  uint64_t minNumberComponents, uint64_t maxNumberComponents,
  Catch::Generators::GeneratorWrapper<std::string>&& componentStringGenerator);
#endif
