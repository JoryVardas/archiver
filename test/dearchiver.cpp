#include "additional_matchers.hpp"
#include "database_connector.hpp"
#include "test_common.hpp"
#include <catch2/catch_all.hpp>
#include <concepts>
#include <fstream>
#include <ranges>
#include <span>
#include <src/app/archiver.hpp>
#include <src/app/dearchiver.hpp>
#include <src/app/stager.hpp>
#include <src/app/util/get_file_read_buffer.hpp>
#include <src/config/config.h>
#include <test/test_constant.hpp>

using Catch::Matchers::StartsWith;
namespace ranges = std::ranges;

bool fileByteCompare(const std::filesystem::path& path1,
                     const std::filesystem::path& path2,
                     std::span<char> buffer1, std::span<char> buffer2) {
  if (buffer1.size() >
      static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
    throw std::logic_error("Could not compare files as buffer1 is larger than "
                           "the maximum input read stream input size.");
  if (buffer2.size() >
      static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
    throw std::logic_error("Could not compare files as buffer2 is larger than "
                           "the maximum input read stream input size.");
  if (!std::filesystem::exists(path1))
    throw FileDoesNotExist(
      FORMAT_LIB::format("The file \"{}\" could not be found", path1));
  if (!std::filesystem::exists(path2))
    throw FileDoesNotExist(
      FORMAT_LIB::format("The file \"{}\" could not be found", path2));
  if (!std::filesystem::is_regular_file(path1))
    throw NotAFile(FORMAT_LIB::format("The path \"{}\" is not a file", path1));
  if (!std::filesystem::is_regular_file(path2))
    throw NotAFile(FORMAT_LIB::format("The path \"{}\" is not a file", path2));

  std::basic_ifstream<char> inputStream1(path1, std::ios_base::binary);
  if (inputStream1.bad() || !inputStream1.is_open()) {
    throw FileException(FORMAT_LIB::format(
      "There was an error opening \"{}\" for reading", path1));
  }

  std::basic_ifstream<char> inputStream2(path2, std::ios_base::binary);
  if (inputStream2.bad() || !inputStream2.is_open()) {
    throw FileException(FORMAT_LIB::format(
      "There was an error opening \"{}\" for reading", path2));
  }

  auto readSize =
    static_cast<std::streamsize>(std::min(buffer1.size(), buffer2.size()));

  while (!inputStream1.eof() && !inputStream2.eof()) {
    inputStream1.read(buffer1.data(), readSize);
    inputStream2.read(buffer2.data(), readSize);

    if (inputStream1.bad()) {
      throw FileException(
        FORMAT_LIB::format("There was an error reading \"{}\"", path1));
    }
    if (inputStream2.bad()) {
      throw FileException(
        FORMAT_LIB::format("There was an error reading \"{}\"", path2));
    }

    Size read1 = static_cast<Size>(inputStream1.gcount());
    Size read2 = static_cast<Size>(inputStream2.gcount());

    if (read1 != read2)
      return false;

    std::span<char> readBuffer1 = {buffer1.data(), read1};
    std::span<char> readBuffer2 = {buffer2.data(), read2};

    if (!ranges::equal(readBuffer1, readBuffer2))
      return false;
  }

  return true;
}

TEST_CASE("Dearchiving files and directories", "[dearchiver][.]") {
  Config config("./config/test_config.json");

  auto [dataPointer1, size1] = getFileReadBuffer(config);
  std::span readBuffer1{dataPointer1.get(), size1};
  auto [dataPointer2, size2] = getFileReadBuffer(config);
  std::span readBuffer2{dataPointer2.get(), size2};

  DatabaseConnector<MockDatabase> databaseConnector;
  auto [stagedDatabase, archivedDatabase] =
    databaseConnector.connect(config, readBuffer1);

  const ArchivedDirectory archivedRootDirectory =
    archivedDatabase->getRootDirectory();

  Stager stager{stagedDatabase, readBuffer1, config.stager.stage_directory};
  Archiver archiver{archivedDatabase, config.stager.stage_directory,
                    config.archive.archive_directory,
                    config.archive.single_archive_size};

  Dearchiver dearchiver{archivedDatabase, config.archive.archive_directory,
                        config.archive.temp_archive_directory, readBuffer1};

  REQUIRE(std::filesystem::is_empty(config.stager.stage_directory));
  REQUIRE(std::filesystem::is_empty(config.archive.archive_directory));
  REQUIRE(std::filesystem::is_empty(config.archive.archive_directory));
  REQUIRE(std::filesystem::is_empty("./dearchive"));

  REQUIRE(databasesAreEmpty(stagedDatabase, archivedDatabase));

  stager.stage({{"./test_data/"}, {"./test_data_additional/"}}, ".");
  archiver.archive(stager.getDirectoriesSorted(), stager.getFilesSorted());

  dearchiver.dearchive("/test_data", "./dearchive");

  SECTION(
    "Having multiple dearchivers sharing the same dearchive directory and "
    "database") {
    Dearchiver dearchiver2{archivedDatabase, config.archive.archive_directory,
                           config.archive.temp_archive_directory, readBuffer1};
    dearchiver2.dearchive("/test_data_additional", "./dearchive");

    REQUIRE(std::filesystem::exists(
      "./dearchive/test_data_additional/TestData_Additional_1.additional"));
    REQUIRE(
      std::filesystem::exists("./dearchive/test_data_additional/"
                              "TestData_Additional_Single_Exact.additional"));
    REQUIRE(fileByteCompare(
      "./test_data_additional/TestData_Additional_1.additional",
      "./dearchive/test_data_additional/TestData_Additional_1.additional",
      readBuffer1, readBuffer2));
    REQUIRE(fileByteCompare(
      "./test_data_additional/TestData_Additional_Single_Exact.additional",
      "./dearchive/test_data_additional/"
      "TestData_Additional_Single_Exact.additional",
      readBuffer1, readBuffer2));
  }

  REQUIRE(std::filesystem::exists("./dearchive/test_data/TestData1.test"));
  REQUIRE(std::filesystem::exists("./dearchive/test_data/TestData_Copy.test"));
  REQUIRE(
    std::filesystem::exists("./dearchive/test_data/TestData_Not_Single.test"));
  REQUIRE(
    std::filesystem::exists("./dearchive/test_data/TestData_Single.test"));
  REQUIRE(std::filesystem::exists(
    "./dearchive/test_data/TestData_Single_Exact.test"));

  REQUIRE(fileByteCompare("./test_data/TestData1.test",
                          "./dearchive/test_data/TestData1.test", readBuffer1,
                          readBuffer2));
  REQUIRE(fileByteCompare("./test_data/TestData_Copy.test",
                          "./dearchive/test_data/TestData_Copy.test",
                          readBuffer1, readBuffer2));
  REQUIRE(fileByteCompare("./test_data/TestData_Not_Single.test",
                          "./dearchive/test_data/TestData_Not_Single.test",
                          readBuffer1, readBuffer2));
  REQUIRE(fileByteCompare("./test_data/TestData_Single.test",
                          "./dearchive/test_data/TestData_Single.test",
                          readBuffer1, readBuffer2));
  REQUIRE(fileByteCompare("./test_data/TestData_Single_Exact.test",
                          "./dearchive/test_data/TestData_Single_Exact.test",
                          readBuffer1, readBuffer2));

  // Remove staged, archived, and dearchived files.
  for (auto const& file :
       std::filesystem::directory_iterator{config.stager.stage_directory}) {
    std::filesystem::remove(file);
  }
  for (auto const& dir :
       std::filesystem::directory_iterator{config.archive.archive_directory}) {
    std::filesystem::remove_all(dir);
  }
  for (auto const& dir : std::filesystem::directory_iterator{"./dearchive"}) {
    std::filesystem::remove_all(dir);
  }
  for (auto const& dir :
       std::filesystem::directory_iterator{"./archive_temp"}) {
    std::filesystem::remove_all(dir);
  }
}