#include "../additional_generators.hpp"
#include "../additional_matchers.hpp"
#include "../helper_functions.hpp"
#include "../helper_macros.hpp"
#include "database_helpers.hpp"
#include <catch2/catch_all.hpp>
#include <span>
#include <src/app/util/get_file_read_buffer.hpp>
#include <src/config/config.h>
#include <test/test_constant.hpp>

using Catch::Matchers::StartsWith;

const std::vector<char> filenameExtensionCharacters = {
  ' ', '!', '#', '$', '%', '&', '\'', '(', ')', '+', ',', '-', '0', '1', '2',
  '3', '4', '5', '6', '7', '8', '9',  ';', '=', '@', 'A', 'B', 'C', 'D', 'E',
  'F', 'G', 'H', 'I', 'J', 'K', 'L',  'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
  'U', 'V', 'W', 'X', 'Y', 'Z', '[',  ']', '^', '_', '`', 'a', 'b', 'c', 'd',
  'e', 'f', 'g', 'h', 'i', 'j', 'k',  'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
  't', 'u', 'v', 'w', 'x', 'y', 'z',  '{', '}', '~'};

TEMPLATE_TEST_CASE("Connecting to, modifying, and retrieving data from the "
                   "implementations of the archived database",
                   "[database]", MysqlDatabase, MockDatabase) {
  // Load the config
  Config config("./config/test_config.json");

  // Get the read buffer
  auto [dataPointer, size] = getFileReadBuffer(config.general.fileReadSizes);
  std::span readBuffer{dataPointer.get(), size};

  // Get the database connections
  DatabaseConnector<TestType> databaseConnector;
  auto [stagedDatabase, archivedDatabase] =
    databaseConnector.connect(config, readBuffer);

  REQUIRE(databasesAreEmpty(stagedDatabase, archivedDatabase));

  // Start the database transactions
  stagedDatabase->startTransaction();
  REQUIRE_NOTHROW(archivedDatabase->startTransaction());

  // Generate a random path and stage it
  auto path =
    GENERATE(std::filesystem::path{StagedDirectory::RootDirectoryName},
             take(10, randomFilePath(1, 5, randomFilename(1, 50))));
  stagedDatabase->add(path);

  // Load all the raw files
  std::vector<RawFile> rawFiles;
  rawFiles.reserve(ArchiverTest::numberAdditionalTestFiles);
  for (uint64_t i = 0; i < ArchiverTest::numberAdditionalTestFiles; ++i) {
    rawFiles.emplace_back(fmt::format("test_files/{}.adt", i), readBuffer);
  }

  // Stage all the raw files
  for (const auto& rawFile : rawFiles) {
    stagedDatabase->add(rawFile, path / rawFile.path.filename());
  }

  // Get the staged files and directories
  const auto stagedDirectories = stagedDatabase->listAllDirectories();
  const auto stagedFiles = stagedDatabase->listAllFiles();

  // Get the archived database root directory
  const ArchivedDirectory archivedRootDirectory =
    archivedDatabase->getRootDirectory();
  REQUIRE(archivedRootDirectory.parent == archivedRootDirectory.id);
  REQUIRE(archivedRootDirectory.name == ArchivedDirectory::RootDirectoryName);

  // make an archive operation
  auto operation =
    REQUIRE_NOTHROW_RETURN(archivedDatabase->createArchiveOperation());
  REQUIRE(archivedDatabase->createArchiveOperation() > operation);

  auto operationModified = operation;
  operationModified += 100;
  REQUIRE(operationModified > operation);

  // archive the staged directories
  std::vector<ArchivedDirectory> archivedDirectories;
  for (const auto& stagedDirectory : stagedDirectories) {
    archivedDirectories.push_back(
      REQUIRE_NOTHROW_RETURN(archivedDatabase->addDirectory(
        stagedDirectory, archivedDirectories.back(), operation)));
  }

  SECTION("Adding and listing directories") {
    REQUIRE(std::size(archivedDirectories) == std::size(stagedDirectories));
    for (std::size_t i = 0; i < std::size(archivedDirectories); ++i) {
      const auto& archivedDirectory = archivedDirectories.at(i);
      const auto& stagedDirectory = stagedDirectories.at(i);
      REQUIRE(archivedDirectory.name == stagedDirectory.name);
      if (archivedDirectory.name == ArchivedDirectory::RootDirectoryName) {
        REQUIRE(archivedDirectory.containingArchiveOperation == 0);
      } else {
        REQUIRE(archivedDirectory.containingArchiveOperation == operation);
      }
    }
    forEachNotLastIter(
      archivedDirectories, [&, archivedDatabase = archivedDatabase](auto iter) {
        const auto cur = *iter;
        const auto next = *(iter + 1);
        REQUIRE(cur.id == next.parent);
        REQUIRE(cur.id != next.id);

        const auto childDirs = archivedDatabase->listChildDirectories(cur);
        REQUIRE(std::size(childDirs) == 1);
        REQUIRE(childDirs.at(0).id == next.id);
      });

    SECTION("Adding a directory to a non-existent operation") {
      REQUIRE_THROWS_MATCHES(
        archivedDatabase->addDirectory(stagedDirectories.back(),
                                       archivedDirectories.back(),
                                       operationModified),
        ArchivedDatabaseException,
        MessageStartsWith("Could not add directory to archived directories"));
    }
    SECTION("Adding a directory which already exists shouldn't do anything") {
      const auto& parentDirectory =
        std::size(archivedDirectories) > 1
          ? archivedDirectories.at(std::size(archivedDirectories) - 2)
          : archivedDirectories.front();
      auto archivedDirectoryCopy = archivedDatabase->addDirectory(
        stagedDirectories.back(), parentDirectory, operation);
      REQUIRE(archivedDirectoryCopy.id == archivedDirectories.back().id);
      REQUIRE(archivedDirectoryCopy.name == archivedDirectories.back().name);
      REQUIRE(archivedDirectoryCopy.parent == parentDirectory.id);
    }
    SECTION("Adding a directory to a parent it wasn't staged to doesn't "
            "produce an error") {
      if (std::size(stagedDirectories) > 1) {
        auto parentStagedDirectory = stagedDirectories.back();
        parentStagedDirectory.name += "_p";
        auto parentDirectory = archivedDatabase->addDirectory(
          parentStagedDirectory, archivedRootDirectory, operation);
        auto archivedDirectory =
          REQUIRE_NOTHROW_RETURN(archivedDatabase->addDirectory(
            stagedDirectories.back(), parentDirectory, operation));
        REQUIRE(archivedDirectory.id > archivedDirectories.back().id);
        REQUIRE(archivedDirectory.name == stagedDirectories.back().name);
        REQUIRE(archivedDirectory.parent == parentDirectory.id);
      }
    }
  }
  SECTION("Archives for file types") {
    SECTION("Non special archives") {
      auto extensionString =
        "." +
        GENERATE(take(10, randomString(1, 5, filenameExtensionCharacters)));
      auto stagedFile = stagedDatabase->add(
        RawFile{"test_data/TestData1.test", readBuffer},
        path / fmt::format("TestData1.{}", extensionString));

      auto archive =
        REQUIRE_NOTHROW_RETURN(archivedDatabase->getArchiveForFile(stagedFile));
      REQUIRE(archive.extension == extensionString);
      REQUIRE(archivedDatabase->getNextArchivePartNumber(archive) == 1);
      REQUIRE_NOTHROW(
        archivedDatabase->incrementNextArchivePartNumber(archive));
      REQUIRE(archivedDatabase->getNextArchivePartNumber(archive) == 2);

      SECTION("Different extensions have different archives") {
        auto extensionString2 = extensionString + "1";
        auto stagedFile2 = stagedDatabase->add(
          RawFile{"test_data/TestData1.test", readBuffer},
          path / fmt::format("TestData1.{}", extensionString2));
        auto archive2 = REQUIRE_NOTHROW_RETURN(
          archivedDatabase->getArchiveForFile(stagedFile2));
        REQUIRE(archive2.extension == extensionString2);
        REQUIRE(archive2.id != archive.id);
        REQUIRE(archive2.extension != archive.extension);
      }
    }
    SECTION("Blank extension") {
      auto extensionString = GENERATE(as<std::string>{}, "", ".");
      auto stagedFile =
        stagedDatabase->add(RawFile{"test_data/TestData1.test", readBuffer},
                            path / fmt::format("TestData1{}", extensionString));

      auto archive =
        REQUIRE_NOTHROW_RETURN(archivedDatabase->getArchiveForFile(stagedFile));
      REQUIRE(archive.extension == "<BLANK>");
      REQUIRE(archivedDatabase->getNextArchivePartNumber(archive) == 1);
      REQUIRE_NOTHROW(
        archivedDatabase->incrementNextArchivePartNumber(archive));
      REQUIRE(archivedDatabase->getNextArchivePartNumber(archive) == 2);
    }
    SECTION("Full archive") {
      REQUIRE(stagedFiles.at(0).size < config.archive.single_archive_size);
      auto archiveFirst = REQUIRE_NOTHROW_RETURN(
        archivedDatabase->getArchiveForFile(stagedFiles.at(0)));

      // Add a bunch of files
      for (const auto& stagedFile : stagedFiles) {
        auto archive = REQUIRE_NOTHROW_RETURN(
          archivedDatabase->getArchiveForFile(stagedFile));
        REQUIRE_NOTHROW(archivedDatabase->addFile(
          stagedFile, archivedDirectories.back(), archive, operation));
      }

      auto archiveLast = REQUIRE_NOTHROW_RETURN(
        archivedDatabase->getArchiveForFile(stagedFiles.at(0)));

      REQUIRE(archiveFirst.id != archiveLast.id);
    }
  }
  SECTION("Adding and listing files") {
    SECTION("Adding a file to a non-existent archive") {
      auto archiveModified = REQUIRE_NOTHROW_RETURN(
        archivedDatabase->getArchiveForFile(stagedFiles.at(0)));
      archiveModified.id += 10000;
      REQUIRE_THROWS_MATCHES(
        archivedDatabase->addFile(stagedFiles.at(0), archivedDirectories.back(),
                                  archiveModified, operation),
        ArchivedDatabaseException,
        MessageStartsWith("Could not add file to archived files"));
    }
    SECTION("Adding a file to a non-existent operation") {
      auto archive = REQUIRE_NOTHROW_RETURN(
        archivedDatabase->getArchiveForFile(stagedFiles.at(0)));
      REQUIRE_THROWS_MATCHES(
        archivedDatabase->addFile(stagedFiles.at(0), archivedDirectories.back(),
                                  archive, operationModified),
        ArchivedDatabaseException,
        MessageStartsWith("Could not add file to archived files"));
    }
    SECTION("Adding a single file") {
      // Add the file
      auto archive = REQUIRE_NOTHROW_RETURN(
        archivedDatabase->getArchiveForFile(stagedFiles.at(0)));
      const auto& stagedFile = stagedFiles.at(0);
      auto archivedFileResult =
        REQUIRE_NOTHROW_RETURN(archivedDatabase->addFile(
          stagedFile, archivedDirectories.back(), archive, operation));

      REQUIRE(archivedFileResult.first == ArchivedFileAddedType::NewRevision);
      auto archivedFileRevisionId = archivedFileResult.second;

      // Get the child files of the directory we added to
      auto childFiles = REQUIRE_NOTHROW_RETURN(
        archivedDatabase->listChildFiles(archivedDirectories.back()));
      REQUIRE(std::size(childFiles) == 1);

      // Check that the file was created correctly
      const auto& archivedFile = childFiles.back();
      REQUIRE(archivedFile.name == stagedFile.name);
      REQUIRE(archivedFile.parentDirectory.id == archivedDirectories.back().id);
      REQUIRE(std::size(archivedFile.revisions) == 1);

      // Check that the file revision was created correctly
      const auto& revision = archivedFile.revisions.back();
      REQUIRE(revision.size == stagedFile.size);
      REQUIRE(revision.hash == stagedFile.hash);
      REQUIRE(revision.containingArchiveId == archive.id);
      REQUIRE(revision.containingOperation == operation);
      REQUIRE(revision.isDuplicate == false);

      SECTION("Second revision to same file different revision") {
        // Forge a staged file with the same name but different hash
        auto stagedFileForged = stagedFiles.at(0);
        stagedFileForged.hash = stagedFiles.at(1).hash;
        // Add the file
        auto archive2 = REQUIRE_NOTHROW_RETURN(
          archivedDatabase->getArchiveForFile(stagedFileForged));
        auto archivedFileResult2 =
          REQUIRE_NOTHROW_RETURN(archivedDatabase->addFile(
            stagedFileForged, archivedDirectories.back(), archive2, operation));

        REQUIRE(archivedFileResult2.first ==
                ArchivedFileAddedType::NewRevision);
        auto archivedFileRevisionId2 = archivedFileResult2.second;
        REQUIRE(archivedFileRevisionId2 != archivedFileRevisionId);

        // Get the child files of the directory we added to
        auto childFiles2 = REQUIRE_NOTHROW_RETURN(
          archivedDatabase->listChildFiles(archivedDirectories.back()));
        REQUIRE(std::size(childFiles) == 1);

        // Check that the file was created correctly
        const auto& archivedFile2 = childFiles2.back();
        REQUIRE(archivedFile2.id == archivedFile.id);
        REQUIRE(archivedFile2.name == stagedFile.name);
        REQUIRE(archivedFile2.parentDirectory.id ==
                archivedDirectories.back().id);
        REQUIRE(std::size(archivedFile2.revisions) == 2);

        // Check that the file revision was created correctly
        const auto& revision2 = archivedFile2.revisions.back();
        REQUIRE(revision2.size == stagedFileForged.size);
        REQUIRE(revision2.hash == stagedFileForged.hash);
        REQUIRE(revision2.containingArchiveId == archive.id);
        REQUIRE(revision2.containingOperation == operation);
        REQUIRE(revision2.isDuplicate == false);
        REQUIRE(revision2.hash != revision.hash);
        REQUIRE(revision2.id > revision.id);
      }
      SECTION("Second revision to same file duplicate revision") {
        // Add the file
        auto archive2 = REQUIRE_NOTHROW_RETURN(
          archivedDatabase->getArchiveForFile(stagedFiles.at(0)));
        auto archivedFileResult2 =
          REQUIRE_NOTHROW_RETURN(archivedDatabase->addFile(
            stagedFile, archivedDirectories.back(), archive2, operation));

        REQUIRE(archivedFileResult2.first ==
                ArchivedFileAddedType::DuplicateRevision);
        auto archivedFileRevisionId2 = archivedFileResult2.second;
        REQUIRE(archivedFileRevisionId2 == archivedFileRevisionId);

        // Get the child files of the directory we added to
        auto childFiles2 = REQUIRE_NOTHROW_RETURN(
          archivedDatabase->listChildFiles(archivedDirectories.back()));
        REQUIRE(std::size(childFiles) == 1);

        // Check that the file was created correctly
        const auto& archivedFile2 = childFiles2.back();
        REQUIRE(archivedFile2.id == archivedFile.id);
        REQUIRE(archivedFile2.name == stagedFile.name);
        REQUIRE(archivedFile2.parentDirectory.id ==
                archivedDirectories.back().id);
        REQUIRE(std::size(archivedFile2.revisions) == 2);

        // Check that the file revision was created correctly
        const auto& revision2 = archivedFile2.revisions.back();
        REQUIRE(revision2.size == revision.size);
        REQUIRE(revision2.hash == revision.hash);
        REQUIRE(revision2.containingArchiveId == archive.id);
        REQUIRE(revision2.containingOperation == operation);
        REQUIRE(revision2.isDuplicate == true);
        REQUIRE(revision2.id == revision.id);
      }
    }
    SECTION("Adding multiple files") {
      // Add the files
      std::vector<std::pair<ArchivedFileAddedType, ArchivedFileRevisionID>>
        archivedFileResults;
      for (const auto& stagedFile : stagedFiles) {
        auto archive = REQUIRE_NOTHROW_RETURN(
          archivedDatabase->getArchiveForFile(stagedFile));
        archivedFileResults.push_back(
          REQUIRE_NOTHROW_RETURN(archivedDatabase->addFile(
            stagedFile, archivedDirectories.back(), archive, operation)));
      }

      // Check the files
      const auto childFiles =
        archivedDatabase->listChildFiles(archivedDirectories.back());
      REQUIRE(std::size(archivedFileResults) == std::size(stagedFiles));
      REQUIRE(std::size(childFiles) == std::size(stagedFiles));

      for (const auto& archivedFileResult : archivedFileResults) {
        REQUIRE(archivedFileResult.first == ArchivedFileAddedType::NewRevision);
        const auto archivedFileId = archivedFileResult.second;
        const auto& archivedFile = *REQUIRE_NOT_EQUAL_RETURN(
          std::ranges::find(childFiles, archivedFileId, &ArchivedFile::id),
          std::ranges::end(childFiles));
        const auto& stagedFile = *REQUIRE_NOT_EQUAL_RETURN(
          std::ranges::find(stagedFiles, archivedFile.name, &StagedFile::name),
          std::ranges::end(stagedFiles));

        REQUIRE(archivedFile.parentDirectory.id ==
                archivedDirectories.back().id);
        REQUIRE(std::size(archivedFile.revisions) == 1);

        const auto& revision = archivedFile.revisions.back();
        REQUIRE(revision.hash == stagedFile.hash);
        REQUIRE(revision.size == stagedFile.size);
        REQUIRE(revision.containingOperation == operation);
        REQUIRE(revision.isDuplicate == false);
      }

      SECTION("Adding a duplicate revision from a different file") {
        // Forge a staged file with the same name but different hash
        auto stagedFileForged = stagedFiles.at(0);
        stagedFileForged.hash = stagedFiles.at(1).hash;
        // Add the file
        auto archive2 = REQUIRE_NOTHROW_RETURN(
          archivedDatabase->getArchiveForFile(stagedFileForged));
        auto archivedFileResult2 =
          REQUIRE_NOTHROW_RETURN(archivedDatabase->addFile(
            stagedFileForged, archivedDirectories.back(), archive2, operation));

        REQUIRE(archivedFileResult2.first ==
                ArchivedFileAddedType::DuplicateRevision);
        auto archivedFileRevisionId2 = archivedFileResult2.second;

        // Get the child files of the directory we added to
        auto childFiles2 = REQUIRE_NOTHROW_RETURN(
          archivedDatabase->listChildFiles(archivedDirectories.back()));
        const auto& archivedFile = *REQUIRE_NOT_EQUAL_RETURN(
          std::ranges::find_if(childFiles2,
                               [](const auto& childFile) {
                                 return std::size(childFile.revisions) > 1;
                               }),
          std::ranges::end(childFiles2));

        // Check the file
        REQUIRE(archivedFile.name == stagedFileForged.name);
        REQUIRE(archivedFile.name == archivedFile.name);
        REQUIRE(archivedFile.parentDirectory.id ==
                archivedDirectories.back().id);
        REQUIRE(std::size(archivedFile.revisions) == 2);

        // Check that the file revision was created correctly
        const auto& revision2 = archivedFile.revisions.back();
        REQUIRE(revision2.id == archivedFileRevisionId2);
        REQUIRE(revision2.size == stagedFileForged.size);
        REQUIRE(revision2.hash == stagedFileForged.hash);
        REQUIRE(revision2.containingOperation == operation);
        REQUIRE(revision2.isDuplicate == true);
        REQUIRE(revision2.hash != archivedFile.revisions.at(0).hash);
        REQUIRE(revision2.id > archivedFile.revisions.at(0).id);
      }
    }
  }

  REQUIRE_NOTHROW(archivedDatabase->rollback());
  stagedDatabase->rollback();

  REQUIRE(databasesAreEmpty(stagedDatabase, archivedDatabase));
}