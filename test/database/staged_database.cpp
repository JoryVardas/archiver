#include "../additional_generators.hpp"
#include "../additional_matchers.hpp"
#include "../helper_functions.hpp"
#include "../helper_macros.hpp"
#include "database_helpers.hpp"
#include <catch2/catch_all.hpp>
#include <fmt/format.h>
#include <ranges>
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
  auto stagedDatabase = databaseConnector.connect(config, readBuffer).first;

  REQUIRE(databaseIsEmpty(stagedDatabase));
  REQUIRE_NOTHROW(stagedDatabase->startTransaction());

  auto path =
    GENERATE(std::filesystem::path{StagedDirectory::RootDirectoryName},
             take(10, randomFilePath(1, 3, randomFilename(1, 50))));

  SECTION("Directory operations") {
    SECTION("Adding and listing directories") {
      SECTION("Adding an empty path") {
        REQUIRE_NOTHROW(stagedDatabase->add(""));
        REQUIRE(stagedDatabase->listAllDirectories().size() == 0);
      }
      SECTION("Getting the root directory") {
        SECTION("Getting the root directory before it has been staged") {
          REQUIRE(!stagedDatabase->getRootDirectory().has_value());
        }
        SECTION("Getting the root directory after it has been staged") {
          REQUIRE_NOTHROW(
            stagedDatabase->add(StagedDirectory::RootDirectoryName));
          auto rootOptional = stagedDatabase->getRootDirectory();
          REQUIRE(rootOptional.has_value());
          auto rootDir = rootOptional.value();
          REQUIRE(rootDir.parent == rootDir.id);
          REQUIRE(rootDir.name == StagedDirectory::RootDirectoryName);
        }
      }
      SECTION("Adding a path") {
        REQUIRE_NOTHROW(stagedDatabase->add(path));

        SECTION("Adding the same path multiple times has no effect") {
          REQUIRE_NOTHROW(stagedDatabase->add(path));
        }
        SECTION(
          "Adding the same path with a trailing separator has no effect") {
          REQUIRE_NOTHROW(stagedDatabase->add(path.generic_string() + "/"));
        }

        const auto stagedDirectories = stagedDatabase->listAllDirectories();

        REQUIRE(stagedDirectories.size() == getNumberPathElements(path));

        // The first directory must be / and is it's own parent
        REQUIRE(stagedDirectories.at(0).name ==
                StagedDirectory::RootDirectoryName);
        REQUIRE(stagedDirectories.at(0).id == stagedDirectories.at(0).parent);

        // Check all parts were staged
        for (const auto& pathComponent : path) {
          REQUIRE(std::ranges::find(stagedDirectories, pathComponent.string(),
                                    &StagedDirectory::name) !=
                  std::ranges::end(stagedDirectories));
        }

        // Check that the parents were assigned correctly
        forEachNotLastIter(stagedDirectories, [&](auto iterator) {
          const auto& cur = *iterator;
          const auto& next = *(iterator + 1);

          REQUIRE(cur.id == next.parent);
        });
      }
    }

    SECTION("Removing directories") {
      stagedDatabase->add(path);

      SECTION("Removing a single directory") {
        auto stagedDirectories = stagedDatabase->listAllDirectories();
        REQUIRE(stagedDirectories.size() == getNumberPathElements(path));

        SECTION("Can't remove a staged directory which is the parent of other "
                "staged directories") {
          REQUIRE_THROWS_MATCHES(
            stagedDatabase->remove(stagedDirectories.at(0)),
            StagedDatabaseException,
            MessageStartsWith("Can not remove staged directory which is the "
                              "parent of other staged directories."));
          REQUIRE(stagedDatabase->listAllDirectories().size() ==
                  getNumberPathElements(path));
        }
        if (path != StagedDirectory::RootDirectoryName) {
          REQUIRE_NOTHROW(
            stagedDatabase->remove(getLastElement(stagedDirectories)));
          REQUIRE(stagedDatabase->listAllDirectories().size() ==
                  getNumberPathElements(path) - 1);

          SECTION(
            "Removal of a staged directory which has already been removed "
            "does nothing") {
            REQUIRE_NOTHROW(
              stagedDatabase->remove(getLastElement(stagedDirectories)));
            REQUIRE(stagedDatabase->listAllDirectories().size() ==
                    getNumberPathElements(path) - 1);
          }
        }
      }
      SECTION("Removing all directories") {
        REQUIRE_NOTHROW(stagedDatabase->removeAllDirectories());
        REQUIRE(stagedDatabase->listAllDirectories().size() == 0);
      }
    }
  }
  SECTION("File operations") {
    std::vector<RawFile> rawFiles;
    rawFiles.reserve(ArchiverTest::numberAdditionalTestFiles);
    for (uint64_t i = 0; i < ArchiverTest::numberAdditionalTestFiles; ++i) {
      rawFiles.emplace_back(fmt::format("test_files/{}.adt", i), readBuffer);
    }

    SECTION("Adding and listing files") {
      stagedDatabase->add(path);

      SECTION("Adding a single file") {
        auto rawFile = GENERATE_REF(
          take(5, map([&](auto index) { return rawFiles.at(index); },
                      random(uint64_t{0}, std::size(rawFiles) - 1))));

        SECTION("Adding a file which is in a directory that has not yet been "
                "staged should throw an error") {
          REQUIRE_THROWS_MATCHES(
            stagedDatabase->add(rawFile, std::filesystem::path{"/t"} /
                                           rawFile.path.filename()),
            StagedDatabaseException,
            MessageStartsWith(
              "Could not add file to staged file database as the parent "
              "directory "
              "hasn't been added to the staged directory database"));
        }

        REQUIRE_NOTHROW(
          stagedDatabase->add(rawFile, path / rawFile.path.filename()));

        auto stagedFiles = stagedDatabase->listAllFiles();
        auto stagedDirectories = stagedDatabase->listAllDirectories();

        REQUIRE(std::size(stagedFiles) == 1);

        auto stagedFile = stagedFiles.at(0);
        REQUIRE(stagedFile.parent == getLastElement(stagedDirectories).id);
        REQUIRE(stagedFile.size == rawFile.size);
        REQUIRE(stagedFile.hash == rawFile.hash);
        REQUIRE(stagedFile.name == rawFile.path.filename().string());
      }
      SECTION("Adding multiple files") {
        for (const auto& rawFile : rawFiles) {
          REQUIRE_NOTHROW(
            stagedDatabase->add(rawFile, path / rawFile.path.filename()));
        }

        auto stagedFiles = stagedDatabase->listAllFiles();
        auto stagedDirectories = stagedDatabase->listAllDirectories();
        auto parentDirectory = getLastElement(stagedDirectories);

        REQUIRE(std::size(stagedFiles) == std::size(rawFiles));

        for (const auto& rawFile : rawFiles) {
          // Get the corresponding staged file
          auto stagedFile = *REQUIRE_NOT_EQUAL_RETURN(
            std::ranges::find(stagedFiles, rawFile.path.filename().string(),
                              &StagedFile::name),
            std::ranges::end(stagedFiles));

          // The name was implicitly checked when finding the staged file.
          REQUIRE(stagedFile.size == rawFile.size);
          REQUIRE(stagedFile.hash == rawFile.hash);
          REQUIRE(stagedFile.parent == parentDirectory.id);
        }

        // Check that the id is unique
        std::ranges::sort(stagedFiles, {}, &StagedFile::id);
        forEachNotLastIter(stagedFiles, [](auto iter) {
          const auto& cur = *iter;
          const auto& next = *(iter + 1);
          REQUIRE(cur.id != next.id);
        });
      }
    }

    SECTION("Removing files") {
      stagedDatabase->add(path);
      for (const auto& rawFile : rawFiles) {
        stagedDatabase->add(rawFile, path / rawFile.path.filename());
      }

      auto stagedFiles = stagedDatabase->listAllFiles();
      REQUIRE(std::size(stagedFiles) == std::size(rawFiles));

      SECTION("Remove a single staged file") {
        REQUIRE_NOTHROW(stagedDatabase->remove(stagedFiles.at(0)));

        auto updatedStagedFiles = stagedDatabase->listAllFiles();
        REQUIRE(std::size(updatedStagedFiles) == std::size(stagedFiles) - 1);

        REQUIRE(std::ranges::find(updatedStagedFiles, stagedFiles.at(0).id,
                                  &StagedFile::id) ==
                std::ranges::end(updatedStagedFiles));

        SECTION("Removing a staged file that is already removed does nothing") {
          REQUIRE_NOTHROW(stagedDatabase->remove(stagedFiles.at(0)));
          REQUIRE(std::size(stagedDatabase->listAllFiles()) ==
                  std::size(updatedStagedFiles));
        }
      }
      SECTION("Removing all staged files") {
        REQUIRE_NOTHROW(stagedDatabase->removeAllFiles());
        REQUIRE(stagedDatabase->listAllFiles().empty());
      }
    }
  }
  REQUIRE_NOTHROW(stagedDatabase->rollback());

  REQUIRE(databaseIsEmpty(stagedDatabase));
}