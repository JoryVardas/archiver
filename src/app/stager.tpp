#include "common.h"
#include <algorithm>
#include <filesystem>
#include <ranges>
#include <vector>

template <StagedFileDatabase StagedFileDatabase,
          StagedDirectoryDatabase StagedDirectoryDatabase>
Stager<StagedFileDatabase, StagedDirectoryDatabase>::Stager(
  std::shared_ptr<StagedFileDatabase>& fileDatabase,
  std::shared_ptr<StagedDirectoryDatabase>& directoryDatabase,
  std::span<uint8_t> fileReadBuffer, const path& stageDirectoryLocation)
  : stagedFileDatabase(fileDatabase),
    stagedDirectoryDatabase(directoryDatabase), readBuffer(fileReadBuffer),
    stageLocation(stageDirectoryLocation) {}

template <StagedFileDatabase StagedFileDatabase,
          StagedDirectoryDatabase StagedDirectoryDatabase>
void Stager<StagedFileDatabase, StagedDirectoryDatabase>::stage(
  const std::vector<path>& paths, std::string_view prefixToRemove) {
  // If the prefix has a trailing forward slash then when we remove it from a
  // path that path will be missing a leading forward slash.
  if (prefixToRemove.ends_with('/'))
    prefixToRemove.remove_suffix(1);

  const auto removePrefix = [&](const path& fullPath) -> path {
    auto stringPath = fullPath.generic_string();
    std::string_view stringPathView = stringPath;
    if (stringPathView.starts_with(prefixToRemove)) {
      stringPathView.remove_prefix(prefixToRemove.size());
    }
    // Need to make a copy because the string that stringPathView is viewing is
    // local.
    return {stringPathView};
  };

  const auto stagePath = [&](const path& itemPath) {
    if (isFile(itemPath)) {
      try {
        stageFile(itemPath, removePrefix(itemPath));
      } catch (StagedFileDatabaseException& err) {
        throw StagerException(fmt::format("Could not stage file \"{}\" : {}",
                                          itemPath, err.what()));
      } catch (std::filesystem::filesystem_error& err) {
        throw StagerException(fmt::format("Could not stage file \"{}\" : {}",
                                          itemPath, err.what()));
      }
    } else if (isDirectory(itemPath))
      try {
        stageDirectory(itemPath, removePrefix(itemPath));
      } catch (StagedFileDatabaseException& err) {
        throw StagerException(fmt::format(
          "Could not stage directory \"{}\" : {}", itemPath, err.what()));
      }
    else {
      throw StagerException(fmt::format(
        "The provided path \"{}\" was neither a regular file or a directory",
        itemPath));
    }
  };

  const auto startTransactions = [&]() {
    stagedDirectoryDatabase->startTransaction();
    stagedFileDatabase->startTransaction();
  };
  const auto commitTransactions = [&]() {
    stagedDirectoryDatabase->commit();
    stagedFileDatabase->commit();
  };
  const auto rollbackTransactions = [&]() {
    stagedDirectoryDatabase->rollback();
    stagedFileDatabase->rollback();
  };

  for (const auto& currentPath : paths) {
    startTransactions();
    try {
      // If path isn't a directory then we need to stage it, and if it is a
      // directory then we need to stage the root directory before we iterate
      // through it since recursive_directory_iterator will not visit the root.
      stagePath(currentPath);
      if (isDirectory(currentPath)) {
        std::ranges::for_each(
          std::filesystem::recursive_directory_iterator(currentPath),
          stagePath);
      }
    } catch (StagerException& err) {
      spdlog::error(fmt::format("Unable to stage \"{}\", skipping. {}",
                                currentPath, err.what()));
      rollbackTransactions();
      continue;
    }
    commitTransactions();
  }
}

template <StagedFileDatabase StagedFileDatabase,
          StagedDirectoryDatabase StagedDirectoryDatabase>
void Stager<StagedFileDatabase, StagedDirectoryDatabase>::stageDirectory(
  const std::filesystem::path& path, const std::filesystem::path& stagePath) {
  stagedDirectoryDatabase->add({stagePath});
}
template <StagedFileDatabase StagedFileDatabase,
          StagedDirectoryDatabase StagedDirectoryDatabase>
void Stager<StagedFileDatabase, StagedDirectoryDatabase>::stageFile(
  const std::filesystem::path& path, const std::filesystem::path& stagePath) {
  RawFile rawFile{path, readBuffer};
  std::filesystem::copy(rawFile.path,
                        stageLocation / (rawFile.shaHash + rawFile.blakeHash));
  stagedFileDatabase->add(rawFile, stagePath);
}
