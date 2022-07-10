#include "../additional_matchers.hpp"
#include "../helper_macros.hpp"
#include <catch2/catch_all.hpp>
#include <src/app/util/get_file_read_buffer.hpp>

TEST_CASE("getFileReadBuffer", "[util]") {
  SECTION("The first size is always taken if it isn't too big") {
    std::vector<Size> sizeList = GENERATE(take(
      10, chunk(4, map([](const auto val) { return static_cast<Size>(val); },
                       random(1, 10000000)))));

    const auto [pointer, size] =
      REQUIRE_NOTHROW_RETURN(getFileReadBuffer(sizeList));

    REQUIRE(size == sizeList[0]);
    REQUIRE(pointer != nullptr);
  }
  SECTION("If the first size is too big, the second is taken") {
    std::vector<Size> sizeList = GENERATE(take(
      10, chunk(4, map([](const auto val) { return static_cast<Size>(val); },
                       random(1, 10000000)))));
    // Insert a value that is obviously way too large
    sizeList.insert(std::begin(sizeList),
                    static_cast<Size>(1000000000000000000));

    const auto [pointer, size] =
      REQUIRE_NOTHROW_RETURN(getFileReadBuffer(sizeList));

    REQUIRE(size == sizeList[1]);
    REQUIRE(pointer != nullptr);
  }
  SECTION("If all the sizes are too large an error should be thrown") {
    std::vector<Size> sizeList = GENERATE(take(
      10, chunk(4, map([](const auto val) { return static_cast<Size>(val); },
                       random(1000000000000000000, 100000000000000000)))));

    REQUIRE_THROWS_MATCHES(
      getFileReadBuffer(sizeList), std::runtime_error,
      MessageStartsWith(
        "Unable to allocate a file read buffer of any of the given sizes"));
  }
  SECTION("If all the size vector is empty then an error should be thrown") {
    std::vector<Size> sizeList = {};

    REQUIRE_THROWS_MATCHES(
      getFileReadBuffer(sizeList), std::runtime_error,
      MessageStartsWith(
        "Unable to allocate a file read buffer of any of the given sizes"));
  }
}