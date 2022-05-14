#include "additional_matchers.hpp"
#include "database_connector.hpp"
#include "test_common.hpp"
#include <catch2/catch_all.hpp>
#include <span>
#include <src/app/util/get_file_read_buffer.hpp>
#include <src/config/config.h>
#include <test/test_constant.hpp>

using Catch::Matchers::StartsWith;

TEMPLATE_TEST_CASE("Connecting to, modifying, and retrieving data from the "
                   "implementations of the archived database",
                   "[database]", MysqlDatabase, MockDatabase) {
  Config config("./config/test_config.json");

  auto [dataPointer, size] = getFileReadBuffer(config);
  std::span readBuffer{dataPointer.get(), size};

  DatabaseConnector<TestType> databaseConnector;
  auto [stagedDatabase, archivedDatabase] =
    databaseConnector.connect(config, readBuffer);

  const ArchivedDirectory archivedRootDirectory =
    archivedDatabase->getRootDirectory();

  REQUIRE(archivedRootDirectory.parent == archivedRootDirectory.id);
  REQUIRE(archivedRootDirectory.name == ArchivedDirectory::RootDirectoryName);

  REQUIRE(databasesAreEmpty(stagedDatabase, archivedDatabase));

  stagedDatabase->startTransaction();
  stagedDatabase->add("/");
  stagedDatabase->add("/dirTest");
  stagedDatabase->add("/dirTest2/");
  stagedDatabase->add("/dirTest/a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p");

  RawFile testFile1{"test_data/TestData1.test", readBuffer};
  RawFile testFileSingle{"test_data/TestData_Single.test", readBuffer};
  RawFile testFileSingleExact{"test_data/TestData_Single_Exact.test",
                              readBuffer};
  RawFile testFileCopy{"test_data/TestData_Copy.test", readBuffer};
  RawFile testFileNotSingle{"test_data/TestData_Not_Single.test", readBuffer};

  stagedDatabase->add(testFile1, "/TestData1.test");
  stagedDatabase->add(testFileSingle, "/dirTest/TestData_Single.test");
  stagedDatabase->add(testFileSingleExact,
                      "/dirTest/TestData_Single_Exact.test1");
  stagedDatabase->add(
    testFileCopy,
    "/dirTest/a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/TestData_Copy.test1");
  stagedDatabase->add(testFileNotSingle, "/dirTest2/TestData_Not_Single.test");

  auto stagedDirectories = stagedDatabase->listAllDirectories();
  auto stagedFiles = stagedDatabase->listAllFiles();

  REQUIRE_NOTHROW(archivedDatabase->startTransaction());
  auto archiveOperation = archivedDatabase->createArchiveOperation();

  SECTION("Adding and listing directories") {
    auto archivedDirectory2 = archivedDatabase->addDirectory(
      stagedDirectories.at(1), archivedRootDirectory, archiveOperation);
    REQUIRE(archivedDirectory2.id > archivedRootDirectory.id);
    REQUIRE(archivedDirectory2.name == stagedDirectories.at(1).name);
    REQUIRE(archivedDirectory2.parent == archivedRootDirectory.id);

    auto archivedDirectory3 = archivedDatabase->addDirectory(
      stagedDirectories.at(2), archivedRootDirectory, archiveOperation);
    REQUIRE(archivedDirectory3.id > archivedDirectory2.id);
    REQUIRE(archivedDirectory3.name == stagedDirectories.at(2).name);
    REQUIRE(archivedDirectory3.parent == archivedRootDirectory.id);

    auto archivedDirectory4 = archivedDatabase->addDirectory(
      stagedDirectories.at(3), archivedDirectory2, archiveOperation);
    REQUIRE(archivedDirectory4.id > archivedDirectory3.id);
    REQUIRE(archivedDirectory4.name == stagedDirectories.at(3).name);
    REQUIRE(archivedDirectory4.parent == archivedDirectory2.id);

    SECTION("Adding a directory which already exists shouldn't do anything") {
      auto archivedRootDirectoryCopy = archivedDatabase->addDirectory(
        stagedDirectories.at(0), archivedRootDirectory, archiveOperation);
      REQUIRE(archivedRootDirectoryCopy.id == archivedRootDirectory.id);
      REQUIRE(archivedRootDirectoryCopy.name == archivedRootDirectory.name);
      REQUIRE(archivedRootDirectoryCopy.parent == archivedRootDirectory.parent);

      auto archivedDirectory2Copy = archivedDatabase->addDirectory(
        stagedDirectories.at(1), archivedRootDirectory, archiveOperation);
      REQUIRE(archivedDirectory2Copy.id == archivedDirectory2.id);
      REQUIRE(archivedDirectory2Copy.name == archivedDirectory2.name);
      REQUIRE(archivedDirectory2Copy.parent == archivedDirectory2.parent);
    }
    SECTION("Adding a directory to a parent it wasn't staged to doesn't "
            "produce an error") {
      auto archivedDirectory18 = archivedDatabase->addDirectory(
        stagedDirectories.at(18), archivedRootDirectory, archiveOperation);
      REQUIRE(archivedDirectory18.id > archivedDirectory4.id);
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
      REQUIRE(directoriesUnderSecond.at(0).parent == archivedDirectory4.parent);
    }
  }

  SECTION("Adding and listing files and archives") {
    SECTION("Getting archive for file") {
      auto archive1 = archivedDatabase->getArchiveForFile(stagedFiles.at(0));
      REQUIRE(archive1.extension == ".test");

      auto archive2 = archivedDatabase->getArchiveForFile(stagedFiles.at(1));
      REQUIRE(archive2.extension == ".test");
      REQUIRE(archive2.id == archive1.id);

      auto archive3 = archivedDatabase->getArchiveForFile(stagedFiles.at(2));
      REQUIRE(archive3.extension == ".test1");
      REQUIRE(archive3.id != archive2.id);

      auto archive4 = archivedDatabase->getArchiveForFile(stagedFiles.at(3));
      REQUIRE(archive4.extension == ".test1");
      REQUIRE(archive4.id == archive3.id);

      auto archive5 = archivedDatabase->getArchiveForFile(stagedFiles.at(4));
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
    auto archivedDirectory1 = archivedDatabase->addDirectory(
      stagedDirectories.at(1), archivedRootDirectory, archiveOperation);
    auto archivedDirectory2 = archivedDatabase->addDirectory(
      stagedDirectories.at(2), archivedRootDirectory, archiveOperation);
    auto archivedDirectoryA = archivedDatabase->addDirectory(
      stagedDirectories.at(3), archivedDirectory2, archiveOperation);
    auto archivedDirectoryB = archivedDatabase->addDirectory(
      stagedDirectories.at(4), archivedDirectoryA, archiveOperation);
    auto archivedDirectoryC = archivedDatabase->addDirectory(
      stagedDirectories.at(5), archivedDirectoryB, archiveOperation);
    auto archivedDirectoryD = archivedDatabase->addDirectory(
      stagedDirectories.at(6), archivedDirectoryC, archiveOperation);
    auto archivedDirectoryE = archivedDatabase->addDirectory(
      stagedDirectories.at(7), archivedDirectoryD, archiveOperation);
    auto archivedDirectoryF = archivedDatabase->addDirectory(
      stagedDirectories.at(8), archivedDirectoryE, archiveOperation);
    auto archivedDirectoryG = archivedDatabase->addDirectory(
      stagedDirectories.at(9), archivedDirectoryF, archiveOperation);
    auto archivedDirectoryH = archivedDatabase->addDirectory(
      stagedDirectories.at(10), archivedDirectoryG, archiveOperation);
    auto archivedDirectoryI = archivedDatabase->addDirectory(
      stagedDirectories.at(11), archivedDirectoryH, archiveOperation);
    auto archivedDirectoryJ = archivedDatabase->addDirectory(
      stagedDirectories.at(12), archivedDirectoryI, archiveOperation);
    auto archivedDirectoryK = archivedDatabase->addDirectory(
      stagedDirectories.at(13), archivedDirectoryJ, archiveOperation);
    auto archivedDirectoryL = archivedDatabase->addDirectory(
      stagedDirectories.at(14), archivedDirectoryK, archiveOperation);
    auto archivedDirectoryM = archivedDatabase->addDirectory(
      stagedDirectories.at(15), archivedDirectoryL, archiveOperation);
    auto archivedDirectoryN = archivedDatabase->addDirectory(
      stagedDirectories.at(16), archivedDirectoryM, archiveOperation);
    auto archivedDirectoryO = archivedDatabase->addDirectory(
      stagedDirectories.at(17), archivedDirectoryN, archiveOperation);
    auto archivedDirectoryP = archivedDatabase->addDirectory(
      stagedDirectories.at(18), archivedDirectoryO, archiveOperation);

    auto archive1 = archivedDatabase->getArchiveForFile(stagedFiles.at(0));
    auto added1 = archivedDatabase->addFile(
      stagedFiles.at(0), archivedRootDirectory, archive1, archiveOperation);
    REQUIRE(added1.first == ArchivedFileAddedType::NewRevision);

    auto archive2 = archivedDatabase->getArchiveForFile(stagedFiles.at(1));
    auto added2 = archivedDatabase->addFile(
      stagedFiles.at(1), archivedDirectory1, archive2, archiveOperation);
    REQUIRE(added2.first == ArchivedFileAddedType::NewRevision);

    auto archive3 = archivedDatabase->getArchiveForFile(stagedFiles.at(2));
    auto added3 = archivedDatabase->addFile(
      stagedFiles.at(2), archivedDirectory1, archive3, archiveOperation);
    REQUIRE(added3.first == ArchivedFileAddedType::NewRevision);

    auto archive5 = archivedDatabase->getArchiveForFile(stagedFiles.at(4));
    auto added5 = archivedDatabase->addFile(
      stagedFiles.at(4), archivedDirectory2, archive5, archiveOperation);
    REQUIRE(added5.first == ArchivedFileAddedType::NewRevision);

    auto archive4 = archivedDatabase->getArchiveForFile(stagedFiles.at(3));
    auto added4 = archivedDatabase->addFile(
      stagedFiles.at(3), archivedDirectoryP, archive4, archiveOperation);
    REQUIRE(added4.first == ArchivedFileAddedType::DuplicateRevision);

    SECTION("Adding a file to a directory it wasn't staged to doesn't "
            "throw an error") {
      auto archiveD = archivedDatabase->getArchiveForFile(stagedFiles.at(4));
      REQUIRE_NOTHROW(archivedDatabase->addFile(
        stagedFiles.at(4), archivedDirectoryD, archiveD, archiveOperation));

      auto archivedFilesD =
        archivedDatabase->listChildFiles(archivedDirectoryD);
      REQUIRE(archivedFilesD.size() == 1);
      REQUIRE(archivedFilesD.at(0).name == stagedFiles.at(4).name);
      REQUIRE(archivedFilesD.at(0).parentDirectory.id == archivedDirectoryD.id);
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
        stagedFiles.at(4), archivedDirectory2, archiveTwice2, archiveOperation);
      REQUIRE(added6.first == ArchivedFileAddedType::DuplicateRevision);

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
    REQUIRE(archivedFilesDirectory.at(0).revisions.at(0).containingArchiveId ==
            archive2.id);
    REQUIRE(archivedFilesDirectory.at(0).revisions.at(0).size ==
            stagedFiles.at(1).size);
    REQUIRE(archivedFilesDirectory.at(0).revisions.at(0).hash ==
            stagedFiles.at(1).hash);
    REQUIRE(archivedFilesDirectory.at(1).name == stagedFiles.at(2).name);
    REQUIRE(archivedFilesDirectory.at(1).parentDirectory.id ==
            archivedDirectory1.id);
    REQUIRE(archivedFilesDirectory.at(1).revisions.size() == 1);
    REQUIRE(archivedFilesDirectory.at(1).revisions.at(0).containingArchiveId ==
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
    REQUIRE(archivedFilesDirectory2.at(0).revisions.at(0).containingArchiveId ==
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
    REQUIRE(archivedFilesDirectoryP.at(0).revisions.at(0).containingArchiveId ==
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

  REQUIRE_NOTHROW(archivedDatabase->rollback());
  REQUIRE_NOTHROW(stagedDatabase->rollback());

  REQUIRE(databasesAreEmpty(stagedDatabase, archivedDatabase));
}