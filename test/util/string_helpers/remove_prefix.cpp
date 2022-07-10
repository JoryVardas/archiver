#include <catch2/catch_all.hpp>
#include <src/app/util/string_helpers.hpp>
#include <string>

TEMPLATE_PRODUCT_TEST_CASE("removePrefix", "[util]", std::tuple,
                           ((const char*, const char*),
                            (const char*, std::string),
                            (std::string, const char*),
                            (std::string, (std::string)))) {
  using StringType = std::tuple_element_t<0, TestType>;
  using PrefixType = std::tuple_element_t<1, TestType>;

  StringType emptyString = "";
  StringType singleCharacterString = "t";
  StringType testString = "test";

  SECTION("Nothing happens if the prefix is empty") {
    PrefixType prefix = "";
    REQUIRE(removePrefix(emptyString, prefix) == emptyString);
    REQUIRE(removePrefix(singleCharacterString, prefix) ==
            singleCharacterString);
    REQUIRE(removePrefix(testString, prefix) == testString);
  }
  SECTION("Nothing happens if the prefix doesn't exist in the input") {
    PrefixType prefix = GENERATE("a", "testing");
    REQUIRE(removePrefix(emptyString, prefix) == emptyString);
    REQUIRE(removePrefix(singleCharacterString, prefix) ==
            singleCharacterString);
    REQUIRE(removePrefix(testString, prefix) == testString);
  }
  SECTION("Nothing happens if the input string is empty") {
    PrefixType prefix = GENERATE("", "a", "testing", "t", "test");
    REQUIRE(removePrefix(emptyString, prefix) == emptyString);
  }
  SECTION(
    "Nothing happens if the prefix is in the string but not at the start") {
    PrefixType prefix = "e";
    REQUIRE(removePrefix(testString, prefix) == testString);
  }
  SECTION("Prefix is in the input string") {
    REQUIRE(removePrefix(singleCharacterString, PrefixType("t")) ==
            emptyString);
    REQUIRE(removePrefix(testString, PrefixType("t")) == "est");
    REQUIRE(removePrefix(testString, PrefixType("test")) == "");
  }
}