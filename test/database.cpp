#include "additional_matchers.hpp"
#include <catch2/catch_all.hpp>
#include <src/app/util/get_file_read_buffer.hpp>
#include <src/config/config.h>
#include <src/database/default_implementation/archived_database.hpp>
#include <src/database/default_implementation/staged_database.hpp>
#include <test/test_constant.hpp>

using Catch::Matchers::StartsWith;

TEST_CASE("Connecting to, modifying, and retrieving data from the default "
          "implementations of the staged/archived directory/file database") {
  Config config("./config/test_config.json");

  auto databaseConnectionConfig =
    std::make_shared<StagedDatabase::ConnectionConfig>();
  databaseConnectionConfig->database = config.database.location.schema;
  databaseConnectionConfig->host = config.database.location.host;
  databaseConnectionConfig->port = config.database.location.port;
  databaseConnectionConfig->user = config.database.user;
  databaseConnectionConfig->password = config.database.password;

  auto [dataPointer, size] = getFileReadBuffer(config);
  std::span readBuffer{dataPointer.get(), size};

  auto stagedDatabase =
    std::make_shared<StagedDatabase>(databaseConnectionConfig);
  auto archivedDatabase = std::make_shared<ArchivedDatabase>(
    databaseConnectionConfig, config.archive.target_size);
  const ArchivedDirectory archivedRootDirectory =
    archivedDatabase->getRootDirectory();

  // Require that all the databases are empty
  REQUIRE(stagedDatabase->listAllFiles().empty());
  REQUIRE(stagedDatabase->listAllDirectories().empty());
  REQUIRE(
    archivedDatabase->listChildDirectories(archivedRootDirectory).empty());
  REQUIRE(archivedDatabase->listChildFiles(archivedRootDirectory).empty());

  SECTION("Staged directory database") {
    SECTION(
      "Adding and listing entries to/from the staged directory database") {
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

      // Require that none of the other database were modified
      REQUIRE(stagedDatabase->listAllFiles().empty());
      REQUIRE(
        archivedDatabase->listChildDirectories(archivedRootDirectory).empty());
      REQUIRE(archivedDatabase->listChildFiles(archivedRootDirectory).empty());

      REQUIRE_NOTHROW(stagedDatabase->rollback());
    }
    SECTION("Removing all entries from the staged directory database") {
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
          StagedDirectoryDatabaseException,
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
  SECTION("Staged file database") {
    REQUIRE_NOTHROW(stagedDatabase->startTransaction());

    REQUIRE_NOTHROW(stagedDatabase->add("/"));
    REQUIRE_NOTHROW(stagedDatabase->add("/dirTest"));
    REQUIRE_NOTHROW(stagedDatabase->add("/dirTest2/"));

    RawFile testFile1{"TestData1.test", readBuffer};
    RawFile testFile2{"TestData_Single.test", readBuffer};

    SECTION("Adding and listing entries to/from the staged file database") {
      SECTION(
        "Adding a file which is in a directory that has not yet been staged") {
        REQUIRE_THROWS_MATCHES(
          stagedDatabase->add(testFile1, "/test/TestData1.test"),
          StagedFileDatabaseException,
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
      REQUIRE(stagedFiles.at(0).size == ArchiverTest::TestData1::size);
      REQUIRE(stagedFiles.at(0).hash == ArchiverTest::TestData1::hash);
      REQUIRE(stagedFiles.at(0).name == "TestData1.test");
      REQUIRE(stagedFiles.at(1).parent == stagedDirectories.at(2).id);
      REQUIRE(stagedFiles.at(1).size == ArchiverTest::TestDataSingle::size);
      REQUIRE(stagedFiles.at(1).hash == ArchiverTest::TestDataSingle::hash);
      REQUIRE(stagedFiles.at(1).name == "TestData2.test");
      REQUIRE(stagedFiles.at(1).parent != stagedFiles.at(0).parent);
      REQUIRE(stagedFiles.at(1).id > stagedFiles.at(0).id);
    }
    SECTION("Removing staged files from the staged file database") {
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

    REQUIRE_NOTHROW(stagedDatabase->rollback());
  }

  SECTION("Archived directory, file, and archive databases") {
    REQUIRE_NOTHROW(archivedDatabase->startTransaction());
    REQUIRE_NOTHROW(stagedDatabase->startTransaction());

    REQUIRE_NOTHROW(stagedDatabase->add("/"));
    REQUIRE_NOTHROW(stagedDatabase->add("/dirTest"));
    REQUIRE_NOTHROW(stagedDatabase->add("/dirTest2/"));
    REQUIRE_NOTHROW(
      stagedDatabase->add("/dirTest/a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p"));

    auto stagedDirectories = stagedDatabase->listAllDirectories();

    SECTION("Archived directory database") {
      REQUIRE(archivedDatabase->getRootDirectory().name ==
              ArchivedDirectory::RootDirectoryName);

      SECTION("Adding a directory which already exists shouldn't do anything") {
        auto archivedDirectory1 = archivedDatabase->addDirectory(
          stagedDirectories.at(0), archivedRootDirectory);
        REQUIRE(archivedDirectory1.id == archivedRootDirectory.id);
        REQUIRE(archivedDirectory1.name == archivedRootDirectory.name);
        REQUIRE(archivedDirectory1.parent == archivedRootDirectory.parent);
      }

      auto archivedDirectory2 = archivedDatabase->addDirectory(
        stagedDirectories.at(1), archivedRootDirectory);
      REQUIRE(archivedDirectory2.id >= archivedRootDirectory.id);
      REQUIRE(archivedDirectory2.name == stagedDirectories.at(1).name);
      REQUIRE(archivedDirectory2.parent == archivedRootDirectory.id);

      auto archivedDirectory3 = archivedDatabase->addDirectory(
        stagedDirectories.at(2), archivedRootDirectory);
      REQUIRE(archivedDirectory2.id >= archivedDirectory2.id);
      REQUIRE(archivedDirectory2.name == stagedDirectories.at(1).name);
      REQUIRE(archivedDirectory2.parent == archivedRootDirectory.id);

      auto archivedDirectory4 = archivedDatabase->addDirectory(
        stagedDirectories.at(3), archivedDirectory2);
      REQUIRE(archivedDirectory4.id >= archivedDirectory3.id);
      REQUIRE(archivedDirectory4.name == stagedDirectories.at(3).name);
      REQUIRE(archivedDirectory4.parent == archivedDirectory2.id);

      SECTION("Adding a directory to a parent it wasn't staged to doesn't "
              "produce an error") {
        auto archivedDirectory18 = archivedDatabase->addDirectory(
          stagedDirectories.at(18), archivedRootDirectory);
        REQUIRE(archivedDirectory18.id >= archivedDirectory4.id);
        REQUIRE(archivedDirectory18.name == stagedDirectories.at(18).name);
        REQUIRE(archivedDirectory18.parent == archivedRootDirectory.id);
      }

      SECTION("Listing archived directories") {
        auto directoriesUnderRoot =
          archivedDatabase->listChildDirectories(archivedRootDirectory);

        REQUIRE(directoriesUnderRoot.size() == 2);
        REQUIRE(directoriesUnderRoot.at(0).id == archivedDirectory2.id);
        REQUIRE(directoriesUnderRoot.at(0).name == archivedDirectory2.name);
        REQUIRE(directoriesUnderRoot.at(0).parent == archivedDirectory2.parent);
        REQUIRE(directoriesUnderRoot.at(1).id == archivedDirectory3.id);
        REQUIRE(directoriesUnderRoot.at(1).name == archivedDirectory3.name);
        REQUIRE(directoriesUnderRoot.at(1).parent == archivedDirectory3.parent);

        auto directoriesUnderSecond =
          archivedDatabase->listChildDirectories(archivedDirectory2);

        REQUIRE(directoriesUnderSecond.size() == 1);
        REQUIRE(directoriesUnderSecond.at(0).id == archivedDirectory4.id);
        REQUIRE(directoriesUnderSecond.at(0).name == archivedDirectory4.name);
        REQUIRE(directoriesUnderSecond.at(0).parent ==
                archivedDirectory4.parent);
      }
    }
  }
  REQUIRE_NOTHROW(archivedDatabase->rollback());
  REQUIRE_NOTHROW(stagedDatabase->rollback());

  // Require that all the databases are empty
  REQUIRE(stagedDatabase->listAllFiles().empty());
  REQUIRE(stagedDatabase->listAllDirectories().empty());
  REQUIRE(
    archivedDatabase->listChildDirectories(archivedRootDirectory).empty());
  REQUIRE(archivedDatabase->listChildFiles(archivedRootDirectory).empty());
}