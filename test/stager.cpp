#include "additional_matchers.hpp"
#include "database_connector.hpp"
#include "test_common.hpp"
#include <catch2/catch_all.hpp>
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

  Stager stager{stagedDatabase, readBuffer, config.stager.stage_directory};

  REQUIRE(std::filesystem::is_empty(config.stager.stage_directory));

  REQUIRE(databasesAreEmpty(stagedDatabase, archivedDatabase));

  REQUIRE_NOTHROW(stager.stage({{"./test_data/"}}, "."));

  auto initialStagedDirectories = stagedDatabase->listAllDirectories();
  auto initialStagedFiles = stagedDatabase->listAllFiles();
  REQUIRE(initialStagedDirectories.size() == 2);
  REQUIRE(initialStagedFiles.size() == 5);

  SECTION(
    "Having multiple stagers sharing the same stage directory and database") {
    Stager stager2{stagedDatabase, readBuffer, config.stager.stage_directory};
    REQUIRE_NOTHROW(stager2.stage({{"./test_data_additional/"}}, "."));

    auto secondStagedDirectories = stagedDatabase->listAllDirectories();
    auto secondStagedFiles = stagedDatabase->listAllFiles();
    REQUIRE(secondStagedDirectories.size() == 3);
    REQUIRE(secondStagedFiles.size() == 7);

    auto testDataAdditional1 = ranges::find(
      secondStagedFiles, "TestData_Additional_1.additional", &StagedFile::name);
    REQUIRE(testDataAdditional1 != ranges::end(secondStagedFiles));
    REQUIRE(testDataAdditional1->name == "TestData_Additional_1.additional");
    REQUIRE(testDataAdditional1->parent == secondStagedDirectories.at(2).id);
    REQUIRE(testDataAdditional1->hash ==
            ArchiverTest::TestDataAdditional1::hash);
    REQUIRE(testDataAdditional1->size ==
            ArchiverTest::TestDataAdditional1::size);
    REQUIRE(std::filesystem::exists({FORMAT_LIB::format(
      "{}/{}", config.stager.stage_directory, testDataAdditional1->id)}));

    auto testDataAdditionalSingleExact = ranges::find(
      secondStagedFiles, "TestData_Additional_Single_Exact.additional",
      &StagedFile::name);
    REQUIRE(testDataAdditionalSingleExact != ranges::end(secondStagedFiles));
    REQUIRE(testDataAdditionalSingleExact->name ==
            "TestData_Additional_Single_Exact.additional");
    REQUIRE(testDataAdditionalSingleExact->parent ==
            secondStagedDirectories.at(2).id);
    REQUIRE(testDataAdditionalSingleExact->hash ==
            ArchiverTest::TestDataAdditionalSingleExact::hash);
    REQUIRE(testDataAdditionalSingleExact->size ==
            ArchiverTest::TestDataAdditionalSingleExact::size);
    REQUIRE(std::filesystem::exists(
      {FORMAT_LIB::format("{}/{}", config.stager.stage_directory,
                          testDataAdditionalSingleExact->id)}));
  }

  auto stagedDirectories = stagedDatabase->listAllDirectories();
  auto stagedFiles = stagedDatabase->listAllFiles();

  REQUIRE(stagedDirectories.at(0).name == StagedDirectory::RootDirectoryName);
  REQUIRE(stagedDirectories.at(1).name == "test_data");

  auto testData =
    ranges::find(stagedFiles, "TestData1.test", &StagedFile::name);
  REQUIRE(testData != ranges::end(stagedFiles));
  REQUIRE(testData->name == "TestData1.test");
  REQUIRE(testData->parent == stagedDirectories.at(1).id);
  REQUIRE(testData->hash == ArchiverTest::TestData1::hash);
  REQUIRE(testData->size == ArchiverTest::TestData1::size);
  REQUIRE(std::filesystem::exists({FORMAT_LIB::format(
    "{}/{}", config.stager.stage_directory, testData->id)}));

  auto testDataCopy =
    ranges::find(stagedFiles, "TestData_Copy.test", &StagedFile::name);
  REQUIRE(testDataCopy != ranges::end(stagedFiles));
  REQUIRE(testDataCopy->name == "TestData_Copy.test");
  REQUIRE(testDataCopy->parent == stagedDirectories.at(1).id);
  REQUIRE(testDataCopy->hash == ArchiverTest::TestDataCopy::hash);
  REQUIRE(testDataCopy->size == ArchiverTest::TestDataCopy::size);
  REQUIRE(std::filesystem::exists({FORMAT_LIB::format(
    "{}/{}", config.stager.stage_directory, testDataCopy->id)}));

  auto testDataNotSingle =
    ranges::find(stagedFiles, "TestData_Not_Single.test", &StagedFile::name);
  REQUIRE(testDataNotSingle != ranges::end(stagedFiles));
  REQUIRE(testDataNotSingle->name == "TestData_Not_Single.test");
  REQUIRE(testDataNotSingle->parent == stagedDirectories.at(1).id);
  REQUIRE(testDataNotSingle->hash == ArchiverTest::TestDataNotSingle::hash);
  REQUIRE(testDataNotSingle->size == ArchiverTest::TestDataNotSingle::size);
  REQUIRE(std::filesystem::exists({FORMAT_LIB::format(
    "{}/{}", config.stager.stage_directory, testDataNotSingle->id)}));

  auto testDataSingle =
    ranges::find(stagedFiles, "TestData_Single.test", &StagedFile::name);
  REQUIRE(testDataSingle != ranges::end(stagedFiles));
  REQUIRE(testDataSingle->name == "TestData_Single.test");
  REQUIRE(testDataSingle->parent == stagedDirectories.at(1).id);
  REQUIRE(testDataSingle->hash == ArchiverTest::TestDataSingle::hash);
  REQUIRE(testDataSingle->size == ArchiverTest::TestDataSingle::size);
  REQUIRE(std::filesystem::exists({FORMAT_LIB::format(
    "{}/{}", config.stager.stage_directory, testDataSingle->id)}));

  auto testDataSingleExact =
    ranges::find(stagedFiles, "TestData_Single_Exact.test", &StagedFile::name);
  REQUIRE(testDataSingleExact != ranges::end(stagedFiles));
  REQUIRE(testDataSingleExact->name == "TestData_Single_Exact.test");
  REQUIRE(testDataSingleExact->parent == stagedDirectories.at(1).id);
  REQUIRE(testDataSingleExact->hash == ArchiverTest::TestDataSingleExact::hash);
  REQUIRE(testDataSingleExact->size == ArchiverTest::TestDataSingleExact::size);
  REQUIRE(std::filesystem::exists({FORMAT_LIB::format(
    "{}/{}", config.stager.stage_directory, testDataSingleExact->id)}));

  auto sortedDirectories = stager.getDirectoriesSorted();
  REQUIRE(sortedDirectories.at(0).id == stagedDatabase->getRootDirectory()->id);
  REQUIRE(sortedDirectories.at(1).id > sortedDirectories.at(0).id);
  REQUIRE(sortedDirectories.at(1).parent == sortedDirectories.at(0).id);

  REQUIRE(ranges::is_sorted(stager.getFilesSorted(), {}, &StagedFile::parent));

  REQUIRE(
    archivedDatabase->listChildDirectories(archivedRootDirectory).empty());
  REQUIRE(archivedDatabase->listChildFiles(archivedRootDirectory).empty());

  // Remove staged files.
  for (auto const& file :
       std::filesystem::directory_iterator{config.stager.stage_directory}) {
    std::filesystem::remove(file);
  }
}