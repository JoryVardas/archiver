#include "additional_matchers.hpp"
#include "database_connector.hpp"
#include "test_common.hpp"
#include <catch2/catch_all.hpp>
#include <concepts>
#include <ranges>
#include <span>
#include <src/app/archiver.hpp>
#include <src/app/stager.hpp>
#include <src/app/util/get_file_read_buffer.hpp>
#include <src/config/config.h>
#include <test/test_constant.hpp>

using Catch::Matchers::StartsWith;
namespace ranges = std::ranges;

TEST_CASE("Archiving files and directories", "[archiver]") {
  Config config("./config/test_config.json");

  auto [dataPointer, size] = getFileReadBuffer(config);
  std::span readBuffer{dataPointer.get(), size};

  DatabaseConnector<MockDatabase> databaseConnector;
  auto [stagedDatabase, archivedDatabase] =
    databaseConnector.connect(config, readBuffer);

  const ArchivedDirectory archivedRootDirectory =
    archivedDatabase->getRootDirectory();

  Stager stager{stagedDatabase, readBuffer, config.stager.stage_directory};
  Archiver archiver{archivedDatabase, config.stager.stage_directory,
                    config.archive.archive_directory,
                    config.archive.single_archive_size};

  REQUIRE(std::filesystem::is_empty(config.stager.stage_directory));
  REQUIRE(std::filesystem::is_empty(config.archive.archive_directory));
  REQUIRE(std::filesystem::is_empty(config.archive.archive_directory));

  REQUIRE(databasesAreEmpty(stagedDatabase, archivedDatabase));

  REQUIRE_NOTHROW(stager.stage({{"./test_data/"}}, "."));
  REQUIRE_NOTHROW(
    archiver.archive(stager.getDirectoriesSorted(), stager.getFilesSorted()));

  auto initialArchivedDirectories =
    archivedDatabase->listChildDirectories(archivedRootDirectory);
  REQUIRE(initialArchivedDirectories.size() == 1);

  SECTION("Having multiple archivers sharing the same archive directory and "
          "database") {
    auto initialStagedDirectories = stager.getDirectoriesSorted();
    auto initialStagedFiles = stager.getFilesSorted();

    stager.stage({{"./test_data_additional/"}}, ".");
    auto newlyStagedDirectories = stager.getDirectoriesSorted();
    auto newlyStagedFiles = stager.getFilesSorted();
    std::erase_if(newlyStagedDirectories, [&](auto& val) {
      return val.id <= initialStagedDirectories.back().id;
    });
    std::erase_if(newlyStagedFiles, [&](auto& val) {
      return val.id <= initialStagedFiles.back().id;
    });

    Archiver archiver2{archivedDatabase, config.stager.stage_directory,
                       config.archive.archive_directory,
                       config.archive.single_archive_size};

    REQUIRE_NOTHROW(archiver.archive(newlyStagedDirectories, newlyStagedFiles));

    auto archivedDirectories =
      archivedDatabase->listChildDirectories(archivedRootDirectory);
    REQUIRE(archivedDirectories.size() == 2);
    auto archivedFilesTestDataAdditional =
      archivedDatabase->listChildFiles(archivedDirectories.at(1));

    auto testDataAdditional1 =
      ranges::find(archivedFilesTestDataAdditional,
                   "TestData_Additional_1.additional", &ArchivedFile::name);
    REQUIRE(testDataAdditional1 !=
            ranges::end(archivedFilesTestDataAdditional));
    REQUIRE(testDataAdditional1->name == "TestData_Additional_1.additional");
    REQUIRE(testDataAdditional1->parentDirectory.id ==
            archivedDirectories.at(1).id);
    REQUIRE(testDataAdditional1->revisions.size() == 1);
    REQUIRE(testDataAdditional1->revisions.at(0).hash ==
            ArchiverTest::TestDataAdditional1::hash);
    REQUIRE(testDataAdditional1->revisions.at(0).size ==
            ArchiverTest::TestDataAdditional1::size);
    REQUIRE(std::filesystem::exists({FORMAT_LIB::format(
      "{}/{}/{}", config.archive.archive_directory,
      testDataAdditional1->revisions.at(0).containingArchiveId,
      testDataAdditional1->revisions.at(0).id)}));

    auto testDataAdditionalSingleExact = ranges::find(
      archivedFilesTestDataAdditional,
      "TestData_Additional_Single_Exact.additional", &ArchivedFile::name);
    REQUIRE(testDataAdditionalSingleExact !=
            ranges::end(archivedFilesTestDataAdditional));
    REQUIRE(testDataAdditionalSingleExact->name ==
            "TestData_Additional_Single_Exact.additional");
    REQUIRE(testDataAdditionalSingleExact->parentDirectory.id ==
            archivedDirectories.at(1).id);
    REQUIRE(testDataAdditionalSingleExact->revisions.size() == 1);
    REQUIRE(testDataAdditionalSingleExact->revisions.at(0).hash ==
            ArchiverTest::TestDataAdditionalSingleExact::hash);
    REQUIRE(testDataAdditionalSingleExact->revisions.at(0).size ==
            ArchiverTest::TestDataAdditionalSingleExact::size);
    REQUIRE(std::filesystem::exists({FORMAT_LIB::format(
      "{}/{}/{}", config.archive.archive_directory,
      testDataAdditionalSingleExact->revisions.at(0).containingArchiveId,
      testDataAdditionalSingleExact->revisions.at(0).id)}));
  }

  auto archivedDirectories =
    archivedDatabase->listChildDirectories(archivedRootDirectory);
  auto archivedFilesRoot =
    archivedDatabase->listChildFiles(archivedRootDirectory);
  auto archivedFilesTestData =
    archivedDatabase->listChildFiles(archivedDirectories.at(0));

  REQUIRE(archivedDirectories.at(0).name == "test_data");
  REQUIRE(archivedDirectories.at(0).parent == archivedRootDirectory.id);

  REQUIRE(archivedFilesRoot.size() == 0);

  REQUIRE(archivedFilesTestData.size() == 5);

  auto testData =
    ranges::find(archivedFilesTestData, "TestData1.test", &ArchivedFile::name);
  REQUIRE(testData != ranges::end(archivedFilesTestData));
  REQUIRE(testData->name == "TestData1.test");
  REQUIRE(testData->parentDirectory.id == archivedDirectories.at(0).id);
  REQUIRE(testData->revisions.size() == 1);
  REQUIRE(testData->revisions.at(0).hash == ArchiverTest::TestData1::hash);
  REQUIRE(testData->revisions.at(0).size == ArchiverTest::TestData1::size);
  REQUIRE(std::filesystem::exists(
    {FORMAT_LIB::format("{}/{}/{}", config.archive.archive_directory,
                        testData->revisions.at(0).containingArchiveId,
                        testData->revisions.at(0).id)}));

  auto testDataCopy = ranges::find(archivedFilesTestData, "TestData_Copy.test",
                                   &ArchivedFile::name);
  REQUIRE(testDataCopy != ranges::end(archivedFilesTestData));
  REQUIRE(testDataCopy->name == "TestData_Copy.test");
  REQUIRE(testDataCopy->parentDirectory.id == archivedDirectories.at(0).id);
  REQUIRE(testDataCopy->revisions.size() == 1);
  REQUIRE(testDataCopy->revisions.at(0).hash ==
          ArchiverTest::TestDataCopy::hash);
  REQUIRE(testDataCopy->revisions.at(0).size ==
          ArchiverTest::TestDataCopy::size);
  REQUIRE(std::filesystem::exists(
    {FORMAT_LIB::format("{}/{}/{}", config.archive.archive_directory,
                        testDataCopy->revisions.at(0).containingArchiveId,
                        testDataCopy->revisions.at(0).id)}));

  auto testDataNotSingle = ranges::find(
    archivedFilesTestData, "TestData_Not_Single.test", &ArchivedFile::name);
  REQUIRE(testDataNotSingle != ranges::end(archivedFilesTestData));
  REQUIRE(testDataNotSingle->name == "TestData_Not_Single.test");
  REQUIRE(testDataNotSingle->parentDirectory.id ==
          archivedDirectories.at(0).id);
  REQUIRE(testDataNotSingle->revisions.size() == 1);
  REQUIRE(testDataNotSingle->revisions.at(0).hash ==
          ArchiverTest::TestDataNotSingle::hash);
  REQUIRE(testDataNotSingle->revisions.at(0).size ==
          ArchiverTest::TestDataNotSingle::size);
  REQUIRE(std::filesystem::exists(
    {FORMAT_LIB::format("{}/{}/{}", config.archive.archive_directory,
                        testDataNotSingle->revisions.at(0).containingArchiveId,
                        testDataNotSingle->revisions.at(0).id)}));
  REQUIRE(testDataNotSingle->revisions.at(0).id ==
          testDataCopy->revisions.at(0).id);

  auto testDataSingle = ranges::find(
    archivedFilesTestData, "TestData_Single.test", &ArchivedFile::name);
  REQUIRE(testDataSingle != ranges::end(archivedFilesTestData));
  REQUIRE(testDataSingle->name == "TestData_Single.test");
  REQUIRE(testDataSingle->parentDirectory.id == archivedDirectories.at(0).id);
  REQUIRE(testDataSingle->revisions.size() == 1);
  REQUIRE(testDataSingle->revisions.at(0).hash ==
          ArchiverTest::TestDataSingle::hash);
  REQUIRE(testDataSingle->revisions.at(0).size ==
          ArchiverTest::TestDataSingle::size);
  REQUIRE(std::filesystem::exists(
    {FORMAT_LIB::format("{}/{}/{}", config.archive.archive_directory,
                        testDataNotSingle->revisions.at(0).containingArchiveId,
                        testDataNotSingle->revisions.at(0).id)}));
  REQUIRE(testDataSingle->revisions.at(0).containingArchiveId !=
          testDataNotSingle->revisions.at(0).containingArchiveId);

  auto testDataSingleExact = ranges::find(
    archivedFilesTestData, "TestData_Single_Exact.test", &ArchivedFile::name);
  REQUIRE(testDataSingleExact != ranges::end(archivedFilesTestData));
  REQUIRE(testDataSingleExact->name == "TestData_Single_Exact.test");
  REQUIRE(testDataSingleExact->parentDirectory.id ==
          archivedDirectories.at(0).id);
  REQUIRE(testDataSingleExact->revisions.size() == 1);
  REQUIRE(testDataSingleExact->revisions.at(0).hash ==
          ArchiverTest::TestDataSingleExact::hash);
  REQUIRE(testDataSingleExact->revisions.at(0).size ==
          ArchiverTest::TestDataSingleExact::size);
  REQUIRE(std::filesystem::exists(
    {FORMAT_LIB::format("{}/{}/{}", config.archive.archive_directory,
                        testDataSingle->revisions.at(0).containingArchiveId,
                        testDataSingle->revisions.at(0).id)}));
  REQUIRE(testDataSingle->revisions.at(0).containingArchiveId ==
          testDataSingle->revisions.at(0).containingArchiveId);

  // Remove staged and archived files.
  for (auto const& file :
       std::filesystem::directory_iterator{config.stager.stage_directory}) {
    std::filesystem::remove(file);
  }
  for (auto const& dir :
       std::filesystem::directory_iterator{config.archive.archive_directory}) {
    std::filesystem::remove_all(dir);
  }
}