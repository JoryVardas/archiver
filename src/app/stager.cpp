#include "stager.hpp"
#include "common.h"
#include "util/string_helpers.hpp"
#include <algorithm>
#include <filesystem>
#include <ranges>
#include <vector>

Stager::Stager(std::shared_ptr<StagedDatabase>& stagedDatabase,
               std::span<char> fileReadBuffer,
               const path& stageDirectoryLocation)
  : stagedDatabase(stagedDatabase), readBuffer(fileReadBuffer),
    stageLocation(stageDirectoryLocation) {}

void Stager::stage(const std::vector<path>& paths,
                   std::string_view prefixToRemove) {
  // If the prefix has a trailing forward slash then when we remove it from a
  // path that path will be missing a leading forward slash.
  prefixToRemove = removeSuffix(prefixToRemove, "/");

  const auto removePrefix = [&](const path& fullPath) -> path {
    auto stringPath = fullPath.generic_string();
    std::string_view stringPathView = stringPath;
    // Need to make a copy because the string that stringPathView is viewing is
    // local.
    return {::removePrefix(stringPathView, prefixToRemove)};
  };

  const auto stagePath = [&](const path& itemPath) {
    if (std::filesystem::is_regular_file(itemPath)) {
      try {
        stageFile(itemPath, removePrefix(itemPath));
      } catch (StagedDatabaseException& err) {
        throw StagerException(FORMAT_LIB::format(
          "Could not stage file \"{}\" : {}", itemPath, err.what()));
      } catch (std::filesystem::filesystem_error& err) {
        throw StagerException(FORMAT_LIB::format(
          "Could not stage file \"{}\" : {}", itemPath, err.what()));
      } catch (...) {
        throw StagerException(FORMAT_LIB::format(
          "An unknown error occurred while trying to stage file \"{}\"",
          itemPath));
      }
    } else if (std::filesystem::is_directory(itemPath))
      try {
        stageDirectory(itemPath, removePrefix(itemPath));
      } catch (StagedDatabaseException& err) {
        throw StagerException(FORMAT_LIB::format(
          "Could not stage directory \"{}\" : {}", itemPath, err.what()));
      } catch (...) {
        throw StagerException(FORMAT_LIB::format(
          "An unknown error occurred while trying to stage directory \"{}\"",
          itemPath));
      }
    else {
      throw StagerException(FORMAT_LIB::format(
        "The provided path \"{}\" was neither a regular file or a directory",
        itemPath));
    }
  };

  for (const auto& currentPath : paths) {
    stagedDatabase->startTransaction();
    try {
      // If path isn't a directory then we need to stage it, and if it is a
      // directory then we need to stage the root directory before we iterate
      // through it since recursive_directory_iterator will not visit the root.
      stagePath(currentPath);
      if (std::filesystem::is_directory(currentPath)) {
        std::ranges::for_each(
          std::filesystem::recursive_directory_iterator(currentPath),
          stagePath);
      }
    } catch (StagerException& err) {
      spdlog::error(FORMAT_LIB::format("Unable to stage \"{}\", skipping. {}",
                                       currentPath, err.what()));
      stagedDatabase->rollback();
      continue;
    }
    stagedDatabase->commit();
  }
}

auto Stager::getDirectoriesSorted() -> std::vector<StagedDirectory> {
  auto directories = stagedDatabase->listAllDirectories();
  std::ranges::sort(directories, {}, &StagedDirectory::id);
  return directories;
}
auto Stager::getFilesSorted() -> std::vector<StagedFile> {
  auto files = stagedDatabase->listAllFiles();
  std::ranges::sort(files, {}, &StagedFile::parent);
  return files;
}

void Stager::stageDirectory(const std::filesystem::path& path,
                            const std::filesystem::path& stagePath) {
  stagedDatabase->add({stagePath});
}
void Stager::stageFile(const std::filesystem::path& path,
                       const std::filesystem::path& stagePath) {
  RawFile rawFile{path, readBuffer};
  auto stagedFile = stagedDatabase->add(rawFile, stagePath);
  std::filesystem::copy(rawFile.path.native(),
                        stageLocation /
                          FORMAT_LIB::format("{}", stagedFile.id));
}
