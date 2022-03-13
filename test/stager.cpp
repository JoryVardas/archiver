#include "additional_matchers.hpp"
#include "database_connector.hpp"
#include <catch2/catch_all.hpp>
#include <concepts>
#include <ranges>
#include <span>
#include <src/app/stager.hpp>
#include <src/app/util/get_file_read_buffer.hpp>
#include <src/config/config.h>
#include <test/test_constant.hpp>

using Catch::Matchers::StartsWith;
namespace ranges = std::ranges;

TEST_CASE("Staging files and directories", "[stager]") {
  Config config("./config/test_config.json");

  auto [dataPointer, size] = getFileReadBuffer(config);
  std::span readBuffer{dataPointer.get(), size};

  DatabaseConnector<MockDatabase> databaseConnector;
  auto [stagedDatabase, archivedDatabase] =
    databaseConnector.connect(config, readBuffer);

  const ArchivedDirectory archivedRootDirectory =
    archivedDatabase->getRootDirectory();

  Stager stager{stagedDatabase, stagedDatabase, readBuffer,
                config.stager.stage_directory};

  // Require that all the databases are empty
  REQUIRE(stagedDatabase->listAllFiles().empty());
  REQUIRE(stagedDatabase->listAllDirectories().empty());
  REQUIRE(
    archivedDatabase->listChildDirectories(archivedRootDirectory).empty());
  REQUIRE(archivedDatabase->listChildFiles(archivedRootDirectory).empty());

  REQUIRE_NOTHROW(stager.stage({{"./test_data/"}}, "/test_data"));

  auto stagedDirectories = stagedDatabase->listAllDirectories();
  auto stagedFiles = stagedDatabase->listAllFiles();
  REQUIRE(stagedDirectories.size() == 2);
  REQUIRE(stagedFiles.size() == 5);

  REQUIRE(stagedDirectories.at(0).name == StagedDirectory::RootDirectoryName);
  REQUIRE(stagedDirectories.at(1).name == "test_data");

  auto testData =
    ranges::find(stagedFiles, "TestData1.test", &StagedFile::name);
  REQUIRE(testData != ranges::end(stagedFiles));
  REQUIRE(testData->name == "TestData1.test");
  REQUIRE(testData->parent == stagedDirectories.at(1).id);
  REQUIRE(testData->hash == ArchiverTest::TestData1::hash);
  REQUIRE(testData->size == ArchiverTest::TestData1::size);
  REQUIRE(std::filesystem::exists(
    {FORMAT_LIB::format("./test_data/{}", testData->id)}));

  auto testDataCopy =
    ranges::find(stagedFiles, "TestData_Copy.test", &StagedFile::name);
  REQUIRE(testDataCopy != ranges::end(stagedFiles));
  REQUIRE(testDataCopy->name == "TestData_Copy.test");
  REQUIRE(testDataCopy->parent == stagedDirectories.at(1).id);
  REQUIRE(testDataCopy->hash == ArchiverTest::TestDataCopy::hash);
  REQUIRE(testDataCopy->size == ArchiverTest::TestDataCopy::size);
  REQUIRE(std::filesystem::exists(
    {FORMAT_LIB::format("./test_data/{}", testDataCopy->id)}));

  auto testDataNotSingle =
    ranges::find(stagedFiles, "TestData_Not_Single.test", &StagedFile::name);
  REQUIRE(testDataNotSingle != ranges::end(stagedFiles));
  REQUIRE(testDataNotSingle->name == "TestData_Not_Single.test");
  REQUIRE(testDataNotSingle->parent == stagedDirectories.at(1).id);
  REQUIRE(testDataNotSingle->hash == ArchiverTest::TestDataNotSingle::hash);
  REQUIRE(testDataNotSingle->size == ArchiverTest::TestDataNotSingle::size);
  REQUIRE(std::filesystem::exists(
    {FORMAT_LIB::format("./test_data/{}", testDataNotSingle->id)}));

  auto testDataSingle =
    ranges::find(stagedFiles, "TestData_Single.test", &StagedFile::name);
  REQUIRE(testDataSingle != ranges::end(stagedFiles));
  REQUIRE(testDataSingle->name == "TestData_Single.test");
  REQUIRE(testDataSingle->parent == stagedDirectories.at(1).id);
  REQUIRE(testDataSingle->hash == ArchiverTest::TestDataSingle::hash);
  REQUIRE(testDataSingle->size == ArchiverTest::TestDataSingle::size);
  REQUIRE(std::filesystem::exists(
    {FORMAT_LIB::format("./test_data/{}", testDataSingle->id)}));

  auto testDataSingleExact =
    ranges::find(stagedFiles, "TestData_Single_Exact.test", &StagedFile::name);
  REQUIRE(testDataSingleExact != ranges::end(stagedFiles));
  REQUIRE(testDataSingleExact->name == "TestData_Single_Exact.test");
  REQUIRE(testDataSingleExact->parent == stagedDirectories.at(1).id);
  REQUIRE(testDataSingleExact->hash == ArchiverTest::TestDataSingleExact::hash);
  REQUIRE(testDataSingleExact->size == ArchiverTest::TestDataSingleExact::size);
  REQUIRE(std::filesystem::exists(
    {FORMAT_LIB::format("./test_data/{}", testDataSingleExact->id)}));

  auto sortedDirectories = stager.getDirectoriesSorted();
  REQUIRE(sortedDirectories.at(0).id == stagedDatabase->getRootDirectory()->id);
  REQUIRE(sortedDirectories.at(1).id > sortedDirectories.at(0).id);
  REQUIRE(sortedDirectories.at(1).parent == sortedDirectories.at(0).id);

  auto sortedFiles = stager.getFilesSorted();
  for (auto file = ranges::begin(sortedFiles); file != ranges::end(sortedFiles);
       ++file) {
    REQUIRE(
      ranges::find_if(file + 1, ranges::end(sortedFiles), [&](const auto& cur) {
        return cur.id == file->parent;
      }) == ranges::end(sortedFiles));
  }

  REQUIRE(
    archivedDatabase->listChildDirectories(archivedRootDirectory).empty());
  REQUIRE(archivedDatabase->listChildFiles(archivedRootDirectory).empty());
}