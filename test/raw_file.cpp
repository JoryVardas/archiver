#include "additional_matchers.hpp"
#include <catch2/catch_all.hpp>
#include <span>
#include <src/app/raw_file.hpp>
#include <src/app/util/get_file_read_buffer.hpp>
#include <src/config/config.h>
#include <string>

using Catch::Matchers::Message;
using Catch::Matchers::StartsWith;

using namespace std::string_literals;

TEST_CASE("Raw file") {
  Config config("./config/test_config.json");

  auto [dataPointer, size] = getFileReadBuffer(config.general.fileReadSizes);
  std::span readBuffer{dataPointer.get(), size};

  SECTION("Reading a file that exists does not cause an error") {
    REQUIRE_NOTHROW(RawFile{"test_data/TestData1.test", readBuffer});
  }
  SECTION("The hash of different files is not the same") {
    const auto file1Name =
      GENERATE(values({"TestData1", "TestData_Not_Single", "TestData_Single",
                       "TestData_Single_Exact"}));
    const auto file2Name =
      GENERATE(values({"TestData1", "TestData_Not_Single", "TestData_Single",
                       "TestData_Single_Exact"}));

    if (file1Name != file2Name) {
      const auto file1Path = "test_data/"s + file1Name + ".test"s;
      const auto file2Path = "test_data/"s + file2Name + ".test"s;

      RawFile file1{file1Path, readBuffer};
      RawFile file2{file2Path, readBuffer};

      REQUIRE(file1.hash != "");
      REQUIRE(file2.hash != "");
      REQUIRE(file1.hash != file2.hash);
    }
  }
  SECTION("The hash of the same file, or a copy of the file, is the same") {
    const auto file1Name =
      GENERATE(values({"TestData_Not_Single", "TestData_Copy"}));
    const auto file2Name =
      GENERATE(values({"TestData_Not_Single", "TestData_Copy"}));

    const auto file1Path = "test_data/"s + file1Name + ".test"s;
    const auto file2Path = "test_data/"s + file2Name + ".test"s;

    RawFile file1{file1Path, readBuffer};
    RawFile file2{file2Path, readBuffer};

    REQUIRE(file1.hash != "");
    REQUIRE(file2.hash != "");
    REQUIRE(file1.hash == file2.hash);
  }
  SECTION("The size of a read file matches the filesystem value") {
    const auto fileName =
      GENERATE(values({"TestData1", "TestData_Copy", "TestData_Not_Single",
                       "TestData_Single", "TestData_Single_Exact"}));

    const auto filePath = "test_data/"s + fileName + ".test"s;

    RawFile file{filePath, readBuffer};

    REQUIRE(file.size != 0);
    REQUIRE(file.size == std::filesystem::file_size(filePath));
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