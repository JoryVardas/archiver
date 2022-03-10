#include "additional_matchers.hpp"
#include <catch2/catch_all.hpp>
#include <src/app/util/get_file_read_buffer.hpp>
#include <src/config/config.h>
#include <src/database/mysql_implementation/archived_database.hpp>
#include <src/database/mysql_implementation/staged_database.hpp>
#include <test/test_constant.hpp>

using Catch::Matchers::StartsWith;
using namespace database::mysql;

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
    SECTION("Archived file database and archive database") {
      RawFile testFile1{"TestData1.test", readBuffer};
      RawFile testFileSingle{"TestData_Single.test", readBuffer};
      RawFile testFileSingleExact{"TestData_Single_Exact.test", readBuffer};
      RawFile testFileCopy{"TestData_Copy.test", readBuffer};
      RawFile testFileNotSingle{"TestData_Not_Single.test", readBuffer};

      stagedDatabase->add(testFile1, "/TestData1.test");
      stagedDatabase->add(testFileSingle, "/dirTest/TestData_Single.test");
      stagedDatabase->add(testFileSingleExact,
                          "/dirTest/TestData_Single_Exact.test1");
      stagedDatabase->add(
        testFileCopy,
        "/dirTest/a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/TestData_Copy.test1");
      stagedDatabase->add(testFileNotSingle,
                          "/dirTest2/TestData_Not_Single.test");

      auto stagedFiles = stagedDatabase->listAllFiles();

      SECTION("Archive database") {
        SECTION("Getting archive for file") {
          auto archive1 =
            archivedDatabase->getArchiveForFile(stagedFiles.at(0));
          REQUIRE(archive1.extension == ".test");

          auto archive2 =
            archivedDatabase->getArchiveForFile(stagedFiles.at(1));
          REQUIRE(archive2.extension == ".test");
          REQUIRE(archive2.id == archive1.id);

          auto archive3 =
            archivedDatabase->getArchiveForFile(stagedFiles.at(2));
          REQUIRE(archive3.extension == ".test1");
          REQUIRE(archive3.id != archive2.id);

          auto archive4 =
            archivedDatabase->getArchiveForFile(stagedFiles.at(3));
          REQUIRE(archive4.extension == ".test1");
          REQUIRE(archive4.id == archive3.id);

          auto archive5 =
            archivedDatabase->getArchiveForFile(stagedFiles.at(4));
          REQUIRE(archive5.extension == ".test");
          REQUIRE(archive5.id == archive1.id);
        }
        SECTION("Getting and incrementing archive part number") {
          auto archive = archivedDatabase->getArchiveForFile(stagedFiles.at(1));
          REQUIRE(archivedDatabase->getNextArchivePartNumber(archive) == 1);
          REQUIRE_NOTHROW(
            archivedDatabase->incrementNextArchivePartNumber(archive));
          REQUIRE(archivedDatabase->getNextArchivePartNumber(archive) == 2);
        }
      }
      SECTION("Archived file database") {
        auto archivedDirectory1 = archivedDatabase->addDirectory(
          stagedDirectories.at(1), archivedRootDirectory);
        auto archivedDirectory2 = archivedDatabase->addDirectory(
          stagedDirectories.at(2), archivedRootDirectory);
        auto archivedDirectoryA = archivedDatabase->addDirectory(
          stagedDirectories.at(3), archivedDirectory2);
        auto archivedDirectoryB = archivedDatabase->addDirectory(
          stagedDirectories.at(4), archivedDirectoryA);
        auto archivedDirectoryC = archivedDatabase->addDirectory(
          stagedDirectories.at(5), archivedDirectoryB);
        auto archivedDirectoryD = archivedDatabase->addDirectory(
          stagedDirectories.at(6), archivedDirectoryC);
        auto archivedDirectoryE = archivedDatabase->addDirectory(
          stagedDirectories.at(7), archivedDirectoryD);
        auto archivedDirectoryF = archivedDatabase->addDirectory(
          stagedDirectories.at(8), archivedDirectoryE);
        auto archivedDirectoryG = archivedDatabase->addDirectory(
          stagedDirectories.at(9), archivedDirectoryF);
        auto archivedDirectoryH = archivedDatabase->addDirectory(
          stagedDirectories.at(10), archivedDirectoryG);
        auto archivedDirectoryI = archivedDatabase->addDirectory(
          stagedDirectories.at(11), archivedDirectoryH);
        auto archivedDirectoryJ = archivedDatabase->addDirectory(
          stagedDirectories.at(12), archivedDirectoryI);
        auto archivedDirectoryK = archivedDatabase->addDirectory(
          stagedDirectories.at(13), archivedDirectoryJ);
        auto archivedDirectoryL = archivedDatabase->addDirectory(
          stagedDirectories.at(14), archivedDirectoryK);
        auto archivedDirectoryM = archivedDatabase->addDirectory(
          stagedDirectories.at(15), archivedDirectoryL);
        auto archivedDirectoryN = archivedDatabase->addDirectory(
          stagedDirectories.at(16), archivedDirectoryM);
        auto archivedDirectoryO = archivedDatabase->addDirectory(
          stagedDirectories.at(17), archivedDirectoryN);
        auto archivedDirectoryP = archivedDatabase->addDirectory(
          stagedDirectories.at(18), archivedDirectoryO);

        auto archive1 = archivedDatabase->getArchiveForFile(stagedFiles.at(0));
        auto added1 = archivedDatabase->addFile(
          stagedFiles.at(0), archivedRootDirectory, archive1);
        REQUIRE(added1 == ArchivedFileAddedType::NewRevision);

        auto archive2 = archivedDatabase->getArchiveForFile(stagedFiles.at(1));
        auto added2 = archivedDatabase->addFile(stagedFiles.at(1),
                                                archivedDirectory1, archive2);
        REQUIRE(added2 == ArchivedFileAddedType::NewRevision);

        auto archive3 = archivedDatabase->getArchiveForFile(stagedFiles.at(2));
        auto added3 = archivedDatabase->addFile(stagedFiles.at(2),
                                                archivedDirectory1, archive3);
        REQUIRE(added3 == ArchivedFileAddedType::NewRevision);

        auto archive5 = archivedDatabase->getArchiveForFile(stagedFiles.at(4));
        auto added5 = archivedDatabase->addFile(stagedFiles.at(4),
                                                archivedDirectory2, archive5);
        REQUIRE(added5 == ArchivedFileAddedType::NewRevision);

        auto archive4 = archivedDatabase->getArchiveForFile(stagedFiles.at(3));
        auto added4 = archivedDatabase->addFile(stagedFiles.at(3),
                                                archivedDirectoryP, archive4);
        REQUIRE(added4 == ArchivedFileAddedType::DuplicateRevision);

        SECTION("Adding a file to a directory it wasn't staged to doesn't "
                "throw an error") {
          auto archiveD =
            archivedDatabase->getArchiveForFile(stagedFiles.at(4));
          REQUIRE_NOTHROW(archivedDatabase->addFile(
            stagedFiles.at(4), archivedDirectoryD, archiveD));

          auto archivedFilesD =
            archivedDatabase->listChildFiles(archivedDirectoryD);
          REQUIRE(archivedFilesD.size() == 1);
          REQUIRE(archivedFilesD.at(0).name == stagedFiles.at(4).name);
          REQUIRE(archivedFilesD.at(0).parentDirectory.id ==
                  archivedDirectoryD.id);
          REQUIRE(archivedFilesD.at(0).revisions.size() == 1);
          REQUIRE(archivedFilesD.at(0).revisions.at(0).containingArchiveId ==
                  archive5.id);
          REQUIRE(archivedFilesD.at(0).revisions.at(0).size ==
                  stagedFiles.at(4).size);
          REQUIRE(archivedFilesD.at(0).revisions.at(0).hash ==
                  stagedFiles.at(4).hash);
        }
        SECTION("Adding a file twice gives it an updated revision") {
          auto archiveTwice2 =
            archivedDatabase->getArchiveForFile(stagedFiles.at(4));
          auto added6 = archivedDatabase->addFile(
            stagedFiles.at(4), archivedDirectory2, archiveTwice2);
          REQUIRE(added6 == ArchivedFileAddedType::DuplicateRevision);

          auto archivedFilesDirectory2 =
            archivedDatabase->listChildFiles(archivedDirectory2);
          REQUIRE(archivedFilesDirectory2.size() == 1);
          REQUIRE(archivedFilesDirectory2.at(0).name == stagedFiles.at(4).name);
          REQUIRE(archivedFilesDirectory2.at(0).parentDirectory.id ==
                  archivedDirectory2.id);
          REQUIRE(archivedFilesDirectory2.at(0).revisions.size() == 2);
          REQUIRE(
            archivedFilesDirectory2.at(0).revisions.at(0).containingArchiveId ==
            archive5.id);
          REQUIRE(archivedFilesDirectory2.at(0).revisions.at(0).size ==
                  stagedFiles.at(4).size);
          REQUIRE(archivedFilesDirectory2.at(0).revisions.at(0).hash ==
                  stagedFiles.at(4).hash);
          REQUIRE(
            archivedFilesDirectory2.at(0).revisions.at(1).containingArchiveId ==
            archive5.id);
          REQUIRE(archivedFilesDirectory2.at(0).revisions.at(1).size ==
                  stagedFiles.at(4).size);
          REQUIRE(archivedFilesDirectory2.at(0).revisions.at(1).hash ==
                  stagedFiles.at(4).hash);
        }

        auto archivedFilesRoot =
          archivedDatabase->listChildFiles(archivedRootDirectory);
        REQUIRE(archivedFilesRoot.size() == 1);
        REQUIRE(archivedFilesRoot.at(0).name == stagedFiles.at(0).name);
        REQUIRE(archivedFilesRoot.at(0).parentDirectory.id ==
                archivedRootDirectory.id);
        REQUIRE(archivedFilesRoot.at(0).revisions.size() == 1);
        REQUIRE(archivedFilesRoot.at(0).revisions.at(0).containingArchiveId ==
                archive1.id);
        REQUIRE(archivedFilesRoot.at(0).revisions.at(0).size ==
                stagedFiles.at(0).size);
        REQUIRE(archivedFilesRoot.at(0).revisions.at(0).hash ==
                stagedFiles.at(0).hash);

        auto archivedFilesDirectory =
          archivedDatabase->listChildFiles(archivedDirectory1);
        REQUIRE(archivedFilesDirectory.size() == 2);
        REQUIRE(archivedFilesDirectory.at(0).name == stagedFiles.at(1).name);
        REQUIRE(archivedFilesDirectory.at(0).parentDirectory.id ==
                archivedDirectory1.id);
        REQUIRE(archivedFilesDirectory.at(0).revisions.size() == 1);
        REQUIRE(
          archivedFilesDirectory.at(0).revisions.at(0).containingArchiveId ==
          archive2.id);
        REQUIRE(archivedFilesDirectory.at(0).revisions.at(0).size ==
                stagedFiles.at(1).size);
        REQUIRE(archivedFilesDirectory.at(0).revisions.at(0).hash ==
                stagedFiles.at(1).hash);
        REQUIRE(archivedFilesDirectory.at(1).name == stagedFiles.at(2).name);
        REQUIRE(archivedFilesDirectory.at(1).parentDirectory.id ==
                archivedDirectory1.id);
        REQUIRE(archivedFilesDirectory.at(1).revisions.size() == 1);
        REQUIRE(
          archivedFilesDirectory.at(1).revisions.at(0).containingArchiveId ==
          archive3.id);
        REQUIRE(archivedFilesDirectory.at(1).revisions.at(0).size ==
                stagedFiles.at(2).size);
        REQUIRE(archivedFilesDirectory.at(1).revisions.at(0).hash ==
                stagedFiles.at(2).hash);

        auto archivedFilesDirectory2 =
          archivedDatabase->listChildFiles(archivedDirectory2);
        REQUIRE(archivedFilesDirectory2.size() == 1);
        REQUIRE(archivedFilesDirectory2.at(0).name == stagedFiles.at(4).name);
        REQUIRE(archivedFilesDirectory2.at(0).parentDirectory.id ==
                archivedDirectory2.id);
        REQUIRE(
          archivedFilesDirectory2.at(0).revisions.at(0).containingArchiveId ==
          archive5.id);
        REQUIRE(archivedFilesDirectory2.at(0).revisions.at(0).size ==
                stagedFiles.at(4).size);
        REQUIRE(archivedFilesDirectory2.at(0).revisions.at(0).hash ==
                stagedFiles.at(4).hash);

        auto archivedFilesDirectoryP =
          archivedDatabase->listChildFiles(archivedDirectoryP);
        REQUIRE(archivedFilesDirectoryP.size() == 1);
        REQUIRE(archivedFilesDirectoryP.at(0).name == stagedFiles.at(3).name);
        REQUIRE(archivedFilesDirectoryP.at(0).parentDirectory.id ==
                archivedDirectoryP.id);
        REQUIRE(
          archivedFilesDirectoryP.at(0).revisions.at(0).containingArchiveId ==
          archive5.id);
        REQUIRE(archivedFilesDirectoryP.at(0).revisions.at(0).size ==
                stagedFiles.at(3).size);
        REQUIRE(archivedFilesDirectoryP.at(0).revisions.at(0).hash ==
                stagedFiles.at(3).hash);

        SECTION("Get new archive for archives which are full") {
          REQUIRE(archivedDatabase->getArchiveForFile(stagedFiles.at(0)).id >
                  archive1.id);
        }
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