#include "../additional_matchers.hpp"
#include "database_helpers.hpp"
#include <catch2/catch_all.hpp>
#include <span>
#include <src/app/util/get_file_read_buffer.hpp>
#include <src/config/config.h>
#include <test/test_constant.hpp>

using Catch::Matchers::StartsWith;

TEMPLATE_TEST_CASE("Connecting to, modifying, and retrieving data from the "
                   "implementations of the staged database",
                   "[database]", MysqlDatabase, MockDatabase) {
  Config config("./config/test_config.json");

  auto [dataPointer, size] = getFileReadBuffer(config.general.fileReadSizes);
  std::span readBuffer{dataPointer.get(), size};

  DatabaseConnector<TestType> databaseConnector;
  auto [stagedDatabase, archivedDatabase] =
    databaseConnector.connect(config, readBuffer);

  const ArchivedDirectory archivedRootDirectory =
    archivedDatabase->getRootDirectory();

  REQUIRE(databasesAreEmpty(stagedDatabase, archivedDatabase));

  SECTION("Adding, listing, and removing directories") {
    SECTION("Adding and listing directories") {
      REQUIRE_NOTHROW(stagedDatabase->startTransaction());

      REQUIRE_NOTHROW(stagedDatabase->add("/"));
      REQUIRE_NOTHROW(stagedDatabase->add("/dirTest"));
      REQUIRE_NOTHROW(stagedDatabase->add("/dirTest2/"));
      REQUIRE_NOTHROW(
        stagedDatabase->add("/dirTest/a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p"));

      auto addedDirectories = stagedDatabase->listAllDirectories();
      REQUIRE(addedDirectories.size() == 19);

      REQUIRE(addedDirectories.at(0).parent == addedDirectories.at(0).id);

      REQUIRE(addedDirectories.at(1).id > addedDirectories.at(0).id);
      REQUIRE(addedDirectories.at(1).name == "dirTest");
      REQUIRE(addedDirectories.at(1).parent == addedDirectories.at(0).id);

      REQUIRE(addedDirectories.at(2).id > addedDirectories.at(1).id);
      REQUIRE(addedDirectories.at(2).name == "dirTest2");
      REQUIRE(addedDirectories.at(2).parent == addedDirectories.at(0).id);

      REQUIRE(addedDirectories.at(18).id > addedDirectories.at(17).id);
      REQUIRE(addedDirectories.at(18).name == "p");
      REQUIRE(addedDirectories.at(18).parent == addedDirectories.at(17).id);

      REQUIRE(addedDirectories.at(8).id > addedDirectories.at(7).id);
      REQUIRE(addedDirectories.at(8).name == "f");
      REQUIRE(addedDirectories.at(8).parent == addedDirectories.at(7).id);
    }

    SECTION("Removing directories") {
      REQUIRE_NOTHROW(stagedDatabase->startTransaction());

      REQUIRE_NOTHROW(stagedDatabase->add("/"));
      REQUIRE_NOTHROW(stagedDatabase->add("/dirTest"));
      REQUIRE_NOTHROW(stagedDatabase->add("/dirTest2/"));
      REQUIRE_NOTHROW(
        stagedDatabase->add("/dirTest/a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p"));

      REQUIRE(stagedDatabase->listAllDirectories().size() == 19);

      REQUIRE_NOTHROW(stagedDatabase->commit());

      auto stagedDirectories = stagedDatabase->listAllDirectories();
      REQUIRE(stagedDirectories.size() == 19);

      SECTION("Can't remove a staged directory which is the parent of other "
              "staged directories") {
        REQUIRE_THROWS_MATCHES(
          stagedDatabase->remove(stagedDirectories.at(0)),
          StagedDatabaseException,
          MessageStartsWith("Can not remove staged directory which is the "
                            "parent of other staged directories."));
        REQUIRE(stagedDatabase->listAllDirectories().size() == 19);
      }

      REQUIRE_NOTHROW(stagedDatabase->remove(stagedDirectories.at(18)));
      REQUIRE(stagedDatabase->listAllDirectories().size() == 18);

      SECTION("Removal of a staged directory which has already been removed "
              "does nothing") {
        REQUIRE_NOTHROW(stagedDatabase->remove(stagedDirectories.at(18)));
        REQUIRE(stagedDatabase->listAllDirectories().size() == 18);
      }

      REQUIRE_NOTHROW(stagedDatabase->removeAllDirectories());
    }
  }

  SECTION("Adding, listing, and removing files") {
    REQUIRE_NOTHROW(stagedDatabase->startTransaction());

    REQUIRE_NOTHROW(stagedDatabase->add("/"));
    REQUIRE_NOTHROW(stagedDatabase->add("/dirTest"));
    REQUIRE_NOTHROW(stagedDatabase->add("/dirTest2/"));

    RawFile testFile1{"test_data/TestData1.test", readBuffer};
    RawFile testFile2{"test_data/TestData_Single.test", readBuffer};

    SECTION("Adding and listing files") {
      SECTION("Adding a file which is in a directory that has not yet been "
              "staged should throw an error") {
        REQUIRE_THROWS_MATCHES(
          stagedDatabase->add(testFile1, "/test/TestData1.test"),
          StagedDatabaseException,
          MessageStartsWith(
            "Could not add file to staged file database as the parent "
            "directory "
            "hasn't been added to the staged directory database"));
      }

      REQUIRE_NOTHROW(stagedDatabase->add(testFile1, "/TestData1.test"));
      REQUIRE_NOTHROW(
        stagedDatabase->add(testFile2, "/dirTest2/TestData2.test"));

      auto stagedFiles = stagedDatabase->listAllFiles();
      auto stagedDirectories = stagedDatabase->listAllDirectories();

      REQUIRE(stagedFiles.size() == 2);
      REQUIRE(stagedFiles.at(0).parent == stagedDirectories.at(0).id);
      REQUIRE(stagedFiles.at(0).size == testFile1.size);
      REQUIRE(stagedFiles.at(0).hash == testFile1.hash);
      REQUIRE(stagedFiles.at(0).name == "TestData1.test");
      REQUIRE(stagedFiles.at(1).parent == stagedDirectories.at(2).id);
      REQUIRE(stagedFiles.at(1).size == testFile2.size);
      REQUIRE(stagedFiles.at(1).hash == testFile2.hash);
      REQUIRE(stagedFiles.at(1).name == "TestData2.test");
      REQUIRE(stagedFiles.at(1).parent != stagedFiles.at(0).parent);
      REQUIRE(stagedFiles.at(1).id > stagedFiles.at(0).id);
    }

    SECTION("Removing files") {
      stagedDatabase->add(testFile1, "/TestData1.test");
      stagedDatabase->add(testFile2, "/dirTest2/TestData2.test");
      stagedDatabase->add(testFile2, "/dirTest2/TestData3.test");
      auto stagedFiles = stagedDatabase->listAllFiles();

      REQUIRE(stagedFiles.size() == 3);

      SECTION("Remove a single staged file") {
        REQUIRE_NOTHROW(stagedDatabase->remove(stagedFiles.at(0)));

        auto updatedStagedFiles = stagedDatabase->listAllFiles();

        REQUIRE(updatedStagedFiles.size() == 2);
        REQUIRE(updatedStagedFiles.at(0).id == stagedFiles.at(1).id);
      }
      SECTION("Can't remove a staged file that is already removed") {
        REQUIRE_NOTHROW(stagedDatabase->remove(stagedFiles.at(0)));
        REQUIRE(stagedFiles.size() == 3);
      }
      SECTION("Removing all staged files") {
        REQUIRE_NOTHROW(stagedDatabase->removeAllFiles());
        REQUIRE(stagedDatabase->listAllFiles().empty());
      }
    }
  }

  REQUIRE_NOTHROW(stagedDatabase->rollback());

  REQUIRE(databasesAreEmpty(stagedDatabase, archivedDatabase));
}