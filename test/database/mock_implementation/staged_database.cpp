#include "staged_database.hpp"
#include <concepts>
#include <ranges>
#include <src/app/staged_file.hpp>
#include <src/database/staged_directory_database.hpp>
#include <src/database/staged_file_database.hpp>

namespace ranges = std::ranges;

namespace database::mock {
void StagedDatabase::startTransaction() {
  if (hasTransaction)
    throw DatabaseException(
      "Attempt to start a transaction on the staged database while already "
      "having an existing transaction");
  transactionStagedDirectories = stagedDirectories;
  transactionStagedFiles = stagedFiles;
  hasTransaction = true;
}
void StagedDatabase::rollback() {
  if (hasTransaction) {
    transactionStagedDirectories = stagedDirectories;
    transactionStagedFiles = stagedFiles;
    hasTransaction = false;
  }
}
void StagedDatabase::commit() {
  stagedDirectories = std::move(transactionStagedDirectories);
  stagedFiles = std::move(transactionStagedFiles);
  transactionStagedDirectories.clear();
  transactionStagedFiles.clear();
  hasTransaction = false;
}

auto StagedDatabase::listAllFiles() -> std::vector<StagedFile> {
  return getFileVector();
}

auto StagedDatabase::add(const RawFile& file,
                         const std::filesystem::path& stagePath) -> StagedFile {
  auto parentStagedDirectory = getStagedDirectory(stagePath.parent_path());
  if (!parentStagedDirectory) {
    throw StagedFileDatabaseException(FORMAT_LIB::format(
      "Could not add file to staged file database as the parent directory "
      "hasn't been added to the staged directory database"));
  }

  getFileVector().push_back({nextStagedFileId++, parentStagedDirectory->id,
                             stagePath.filename().string(), file.size,
                             file.hash});
  return getFileVector().back();
}

void StagedDatabase::remove(const StagedFile& stagedFile) {
  std::erase_if(getFileVector(), [&](const StagedFile& file) {
    return file.id == stagedFile.id;
  });
}

void StagedDatabase::removeAllFiles() {
  getFileVector().clear();
  nextStagedFileId = 1;
}

auto StagedDatabase::listAllDirectories() -> std::vector<StagedDirectory> {
  return getDirectoryVector();
}

void StagedDatabase::add(const std::filesystem::path& stagePath) {
  const auto pathToStage = [](const std::filesystem::path& stagePath) {
    if (stagePath.filename().empty())
      return stagePath.parent_path();
    else
      return stagePath;
  }(stagePath);
  if (getStagedDirectory(pathToStage))
    return;

  if (pathToStage.string() == StagedDirectory::RootDirectoryName) {

    getDirectoryVector().push_back(
      {nextStagedDirectoryId, std::string{StagedDirectory::RootDirectoryName},
       nextStagedDirectoryId});
    ++nextStagedDirectoryId;

    return;
  }

  auto parentStagedDirectory = getStagedDirectory(pathToStage.parent_path());
  if (!parentStagedDirectory) {
    add(pathToStage.parent_path());
  }
  parentStagedDirectory = getStagedDirectory(pathToStage.parent_path());

  getDirectoryVector().push_back({nextStagedDirectoryId++,
                                  pathToStage.filename().string(),
                                  parentStagedDirectory->id});
}

void StagedDatabase::remove(const StagedDirectory& stagedDirectory) {

  if (ranges::find(getDirectoryVector(), stagedDirectory.id,
                   &StagedDirectory::parent) !=
      ranges::end(getDirectoryVector())) {
    throw StagedDirectoryDatabaseException(
      "Can not remove staged directory which is the parent of other staged "
      "directories.");
  }
  std::erase_if(getDirectoryVector(), [&](const StagedDirectory& dir) {
    return dir.id == stagedDirectory.id;
  });
}

void StagedDatabase::removeAllDirectories() {
  getDirectoryVector().clear();
  nextStagedDirectoryId = 1;
}

auto StagedDatabase::getRootDirectory() -> std::optional<StagedDirectory> {
  return getStagedDirectory(StagedDirectory::RootDirectoryName);
};

auto StagedDatabase::getStagedDirectory(const std::filesystem::path& stagePath)
  -> std::optional<StagedDirectory> {

  StagedDirectory foundStagedDirectory;
  ID currentId;

  const auto& directoryVector = getDirectoryVector();

  for (const auto& name : stagePath) {
    if (name == StagedDirectory::RootDirectoryName) {
      const auto results =
        ranges::find(directoryVector, name.string(), &StagedDirectory::name);
      if (results == ranges::end(directoryVector)) {
        return std::nullopt;
      }

      currentId = results->id;
      foundStagedDirectory = *results;
    } else {
      const auto results =
        ranges::find_if(directoryVector, [&](const auto& dir) {
          return dir.name == name.string() && dir.parent == currentId;
        });
      if (results == ranges::end(directoryVector)) {
        return std::nullopt;
      }
      currentId = results->id;
      foundStagedDirectory = *results;
    }
  }

  return foundStagedDirectory;
}

auto StagedDatabase::getFileVector() -> decltype(stagedFiles)& {
  if (hasTransaction)
    return transactionStagedFiles;
  else
    return stagedFiles;
}
auto StagedDatabase::getDirectoryVector() -> decltype(stagedDirectories)& {
  if (hasTransaction)
    return transactionStagedDirectories;
  else
    return stagedDirectories;
}
}