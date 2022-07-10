#include <catch2/catch_all.hpp>
#include <src/app/util/string_helpers.hpp>
#include <string>

TEMPLATE_PRODUCT_TEST_CASE("removeSuffix", "[util]", std::tuple,
                           ((const char*, const char*),
                            (const char*, std::string),
                            (std::string, const char*),
                            (std::string, (std::string)))) {
  using StringType = std::tuple_element_t<0, TestType>;
  using SuffixType = std::tuple_element_t<1, TestType>;

  StringType emptyString = "";
  StringType singleCharacterString = "t";
  StringType testString = "test";

  SECTION("Nothing happens if the suffix is empty") {
    SuffixType suffix = "";
    REQUIRE(removeSuffix(emptyString, suffix) == emptyString);
    REQUIRE(removeSuffix(singleCharacterString, suffix) ==
            singleCharacterString);
    REQUIRE(removeSuffix(testString, suffix) == testString);
  }
  SECTION("Nothing happens if the suffix doesn't exist in the input") {
    SuffixType suffix = GENERATE("a", "testing");
    REQUIRE(removeSuffix(emptyString, suffix) == emptyString);
    REQUIRE(removeSuffix(singleCharacterString, suffix) ==
            singleCharacterString);
    REQUIRE(removeSuffix(testString, suffix) == testString);
  }
  SECTION("Nothing happens if the input string is empty") {
    SuffixType suffix = GENERATE("", "a", "testing", "t", "test");
    REQUIRE(removeSuffix(emptyString, suffix) == emptyString);
  }
  SECTION("Nothing happens if the suffix is in the string but not at the end") {
    SuffixType suffix = "e";
    REQUIRE(removeSuffix(testString, suffix) == testString);
  }
  SECTION("Suffix is in the input string") {
    REQUIRE(removeSuffix(singleCharacterString, SuffixType("t")) ==
            emptyString);
    REQUIRE(removeSuffix(testString, SuffixType("t")) == "tes");
    REQUIRE(removeSuffix(testString, SuffixType("test")) == "");
  }
}