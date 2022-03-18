#include "additional_matchers.hpp"
#include <catch2/catch_all.hpp>
#include <span>
#include <src/app/raw_file.hpp>
#include <src/app/util/get_file_read_buffer.hpp>
#include <src/config/config.h>
#include <test/test_constant.hpp>

using Catch::Matchers::Message;
using Catch::Matchers::StartsWith;

TEST_CASE("Raw file") {
  Config config("./config/test_config.json");

  auto [dataPointer, size] = getFileReadBuffer(config);
  std::span readBuffer{dataPointer.get(), size};

  SECTION("Reading a file that exists") {
    using namespace ArchiverTest;
    auto file = GENERATE(
      values({std::pair<std::string, std::pair<std::string, uint64_t>>{
                "test_data/TestData1.test", {TestData1::hash, TestData1::size}},
              {"test_data/TestData_Copy.test",
               {TestDataCopy::hash, TestDataCopy::size}},
              {"test_data/TestData_Not_Single.test",
               {TestDataNotSingle::hash, TestDataNotSingle::size}},
              {"test_data/TestData_Single.test",
               {TestDataSingle::hash, TestDataSingle::size}},
              {"test_data/TestData_Single_Exact.test",
               {TestDataSingleExact::hash, TestDataSingleExact::size}}}));

    RawFile rawFile{{file.first}, readBuffer};

    REQUIRE(rawFile.hash == file.second.first);
    REQUIRE(rawFile.size == file.second.second);
  }
  SECTION("Opening a file that doesn't exist throws an exception") {
    REQUIRE_THROWS_MATCHES(
      (RawFile{"test_data/non_existent.test", readBuffer}), FileDoesNotExist,
      Message("The file \"test_data/non_existent.test\" could not be found"));
  }
  SECTION("Opening a directory as a file throws an exception") {
    REQUIRE_THROWS_MATCHES((RawFile{"test_data/", readBuffer}), NotAFile,
                           Message("The path \"test_data/\" is not a file"));
  }
}