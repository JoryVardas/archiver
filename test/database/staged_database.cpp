#include "../additional_generators.hpp"
#include "../additional_matchers.hpp"
#include "../helper_functions.hpp"
#include "database_helpers.hpp"
#include <catch2/catch_all.hpp>
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

  SECTION("Adding and listing directories") {
    SECTION("Adding an empty path") {
      REQUIRE_NOTHROW(stagedDatabase->add(""));
      auto addedDirectories = stagedDatabase->listAllDirectories();
      REQUIRE(addedDirectories.size() == 0);
    }
    SECTION("Adding the root directory by itself") {
      REQUIRE_NOTHROW(stagedDatabase->add("/"));
      SECTION(
        "Adding the root directory multiple times has no additional effect") {
        REQUIRE_NOTHROW(stagedDatabase->add("/"));
        REQUIRE_NOTHROW(stagedDatabase->add("/"));
      }

      auto addedDirectories = stagedDatabase->listAllDirectories();
      REQUIRE(addedDirectories.size() == 1);
      REQUIRE(addedDirectories.at(0).parent == addedDirectories.at(0).id);

      REQUIRE(stagedDatabase->getRootDirectory());
      REQUIRE(addedDirectories.at(0).id ==
              stagedDatabase->getRootDirectory().value().id);
    }
    SECTION("Adding a path") {
      auto pathComponents = GENERATE(take(
        10, chunk(GENERATE(take(3, random(2, 15))), randomFilename(1, 50))));

      // Make it an optional so we can check if it was assigned to later.
      std::optional<std::filesystem::path> pathToAdd{std::nullopt};

      SECTION("empty members in the path should be ignored") {
        SECTION("empty member is the second component") {
          pathToAdd = "/" + joinStrings(pathComponents, "/") + "/";
        }
        SECTION("empty member is the last component") {
          pathToAdd = "/" + pathComponents.at(0) + "//" +
                      joinStrings(pathComponents, "/", 1);
        }
      }
      SECTION("Adding a path without empty members") {
        pathToAdd = "/" + joinStrings(pathComponents, "/");

        SECTION("Adding the same path multiple times has no effect") {
          REQUIRE_NOTHROW(stagedDatabase->add(pathToAdd.value()));
        }
      }

      // This section will run one additional time where nothing was added to
      // the database, so guard the checks to make sure that we don't check
      // requirements when nothing was done.
      if (pathToAdd) {
        REQUIRE_NOTHROW(stagedDatabase->add(pathToAdd.value()));

        const auto stagedDirectories = stagedDatabase->listAllDirectories();

        REQUIRE(stagedDirectories.size() == pathComponents.size() + 1);

        // The first directory must be / and is it's own parent
        REQUIRE(stagedDirectories.at(0).name ==
                StagedDirectory::RootDirectoryName);
        REQUIRE(stagedDirectories.at(0).id == stagedDirectories.at(0).parent);

        for (auto elem : std::views::counted(std::begin(stagedDirectories),
                                             stagedDirectories.size() - 1) |
                           enumerateView<StagedDirectory>) {
          const auto& cur = elem.second;
          const auto index = elem.first;
          const auto& next = stagedDirectories.at(index + 1);

          REQUIRE(cur.id == next.parent);
          REQUIRE(cur.id < next.id);
          if (index > 0) {
            REQUIRE(cur.name == pathComponents.at(index - 1));
          }
        }
      }
    }
  }

  SECTION("Removing directories") {
    auto pathComponents = GENERATE(
      take(10, chunk(GENERATE(take(3, random(2, 15))), randomFilename(1, 50))));

    std::filesystem::path pathToAdd = "/" + joinStrings(pathComponents, "/");

    REQUIRE_NOTHROW(stagedDatabase->add(pathToAdd));

    auto stagedDirectories = stagedDatabase->listAllDirectories();
    REQUIRE(stagedDirectories.size() == pathComponents.size() + 1);

    SECTION("Can't remove a staged directory which is the parent of other "
            "staged directories") {
      REQUIRE_THROWS_MATCHES(
        stagedDatabase->remove(stagedDirectories.at(0)),
        StagedDatabaseException,
        MessageStartsWith("Can not remove staged directory which is the "
                          "parent of other staged directories."));
      REQUIRE(stagedDatabase->listAllDirectories().size() ==
              pathComponents.size() + 1);
    }

    REQUIRE_NOTHROW(
      stagedDatabase->remove(stagedDirectories.at(pathComponents.size())));
    REQUIRE(stagedDatabase->listAllDirectories().size() ==
            pathComponents.size());

    SECTION("Removal of a staged directory which has already been removed "
            "does nothing") {
      REQUIRE_NOTHROW(
        stagedDatabase->remove(stagedDirectories.at(pathComponents.size())));
      REQUIRE(stagedDatabase->listAllDirectories().size() ==
              pathComponents.size());
    }

    REQUIRE_NOTHROW(stagedDatabase->removeAllDirectories());
    REQUIRE(stagedDatabase->listAllDirectories().size() == 0);
  }

  SECTION("Adding and listing files") {
    REQUIRE_NOTHROW(stagedDatabase->add("/dirTest"));

    RawFile testFile1{"test_data/TestData1.test", readBuffer};
    RawFile testFile2{"test_data/TestData_Single.test", readBuffer};

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
    REQUIRE_NOTHROW(stagedDatabase->add(testFile2, "/dirTest/TestData2.test"));

    auto stagedFiles = stagedDatabase->listAllFiles();
    auto stagedDirectories = stagedDatabase->listAllDirectories();

    REQUIRE(stagedFiles.size() == 2);
    REQUIRE(stagedFiles.at(0).parent == stagedDirectories.at(0).id);
    REQUIRE(stagedFiles.at(0).size == testFile1.size);
    REQUIRE(stagedFiles.at(0).hash == testFile1.hash);
    REQUIRE(stagedFiles.at(0).name == "TestData1.test");
    REQUIRE(stagedFiles.at(1).parent == stagedDirectories.at(1).id);
    REQUIRE(stagedFiles.at(1).size == testFile2.size);
    REQUIRE(stagedFiles.at(1).hash == testFile2.hash);
    REQUIRE(stagedFiles.at(1).name == "TestData2.test");
    REQUIRE(stagedFiles.at(1).parent != stagedFiles.at(0).parent);
    REQUIRE(stagedFiles.at(1).id > stagedFiles.at(0).id);
  }

  SECTION("Removing files") {
    REQUIRE_NOTHROW(stagedDatabase->add("/"));
    REQUIRE_NOTHROW(stagedDatabase->add("/dirTest/"));

    RawFile testFile1{"test_data/TestData1.test", readBuffer};
    RawFile testFile2{"test_data/TestData_Single.test", readBuffer};

    stagedDatabase->add(testFile1, "/TestData1.test");
    stagedDatabase->add(testFile2, "/dirTest/TestData2.test");
    stagedDatabase->add(testFile2, "/dirTest/TestData3.test");
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

  REQUIRE(databaseIsEmpty(stagedDatabase));
}