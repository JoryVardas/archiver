#include "staged_database.hpp"
#include "../../app/staged_file.hpp"
#include "../staged_database.hpp"

using namespace sqlpp;

namespace database::mysql {
StagedDatabase::StagedDatabase(
  std::shared_ptr<StagedDatabase::ConnectionConfig>& config)
  : databaseConnection(config) {}

StagedDatabase::~StagedDatabase() {
  try {
    rollback();
  } catch (const std::exception& err) {
    spdlog::error(FORMAT_LIB::format(
      "There was an error while trying to rollback changes to "
      "the staged database while destructing the staged database "
      "due to an exception: {}",
      err));
  }
}

void StagedDatabase::startTransaction() {
  if (hasTransaction)
    throw DatabaseException(
      "Attempt to start a transaction on the staged database while already "
      "having an existing transaction");

  try {
    databaseConnection.start_transaction();
    hasTransaction = true;
  } catch (sqlpp::exception& err) {
    throw DatabaseException(
      "Could not start a transaction on the staged database: {}", err);
  }
}
void StagedDatabase::rollback() {
  if (hasTransaction) {
    try {
      databaseConnection.rollback_transaction(false);
      hasTransaction = false;
    } catch (sqlpp::exception& err) {
      throw DatabaseException(
        "Could not rollback the transaction on the staged database: {}", err);
    }
  }
}
void StagedDatabase::commit() {
  if (hasTransaction) {
    try {
      databaseConnection.commit_transaction();
      hasTransaction = false;
    } catch (sqlpp::exception& err) {
      throw DatabaseException(
        "Could not commit the transaction on the staged database: {}", err);
    }
  }
}

auto StagedDatabase::listAllFiles() -> std::vector<StagedFile> {
  std::vector<StagedFile> stagedFiles;
  try {
    auto results = databaseConnection(select(all_of(stagedFilesTable))
                                        .from(stagedFilesTable)
                                        .unconditionally());

    for (const auto& stagedFile : results) {
      const auto& stagedFileParent =
        databaseConnection(
          select(stagedFileParentTable.directoryId)
            .from(stagedFileParentTable)
            .where(stagedFileParentTable.fileId == stagedFile.id))
          .front();
      stagedFiles.push_back({stagedFile.id, stagedFileParent.directoryId,
                             stagedFile.name, stagedFile.size,
                             stagedFile.hash});
    }
  } catch (const sqlpp::exception& err) {
    throw StagedDatabaseException("Could not list staged files: {}", err);
  }
  return stagedFiles;
}

auto StagedDatabase::add(const RawFile& file,
                         const std::filesystem::path& stagePath) -> StagedFile {
  try {
    auto parentStagedDirectory = getStagedDirectory(stagePath.parent_path());
    if (!parentStagedDirectory) {
      throw StagedDatabaseException(
        "Could not add file to staged file database as the parent directory "
        "hasn't been added to the staged directory database");
    }
    const auto stagedFileId = databaseConnection(
      insert_into(stagedFilesTable)
        .set(stagedFilesTable.name = stagePath.filename().string(),
             stagedFilesTable.hash = file.hash,
             stagedFilesTable.size = file.size));
    databaseConnection(
      insert_into(stagedFileParentTable)
        .set(stagedFileParentTable.fileId = stagedFileId,
             stagedFileParentTable.directoryId = parentStagedDirectory->id));
    return {stagedFileId, parentStagedDirectory->id,
            stagePath.filename().string(), file.size, file.hash};
  } catch (const sqlpp::exception& err) {
    throw StagedDatabaseException(
      "Could not add file to staged file database: {}", err);
  }
}

void StagedDatabase::remove(const StagedFile& stagedFile) {
  try {
    databaseConnection(remove_from(stagedFileParentTable)
                         .where(stagedFileParentTable.fileId == stagedFile.id));
    databaseConnection(remove_from(stagedFilesTable)
                         .where(stagedFilesTable.id == stagedFile.id));
  } catch (const sqlpp::exception& err) {
    throw StagedDatabaseException(
      "Could not remove file from staged file database: {}", err);
  }
}

void StagedDatabase::removeAllFiles() {
  try {
    databaseConnection(remove_from(stagedFileParentTable).unconditionally());
    databaseConnection(remove_from(stagedFilesTable).unconditionally());
  } catch (const sqlpp::exception& err) {
    throw StagedDatabaseException(
      "Could not remove files from staged file database: {}", err);
  }
}

auto StagedDatabase::listAllDirectories() -> std::vector<StagedDirectory> {
  std::vector<StagedDirectory> stagedDirectories;
  try {
    auto results = databaseConnection(select(all_of(stagedDirectoriesTable))
                                        .from(stagedDirectoriesTable)
                                        .unconditionally());

    for (const auto& stagedDirectory : results) {
      const auto& stagedDirectoryParentResult = databaseConnection(
        select(stagedDirectoryParentTable.parentId)
          .from(stagedDirectoryParentTable)
          .where(stagedDirectoryParentTable.childId == stagedDirectory.id));
      if (stagedDirectoryParentResult.empty()) {
        stagedDirectories.push_back(
          {stagedDirectory.id, stagedDirectory.name, stagedDirectory.id});
      } else {
        stagedDirectories.push_back(
          {stagedDirectory.id, stagedDirectory.name,
           stagedDirectoryParentResult.front().parentId});
      }
    }
  } catch (const sqlpp::exception& err) {
    throw StagedDatabaseException("Could not list staged directories: {}", err);
  }
  return stagedDirectories;
}

void StagedDatabase::add(const std::filesystem::path& stagePath) {
  const auto pathToStage = [](const std::filesystem::path& stagePath) {
    if (stagePath.filename().empty())
      return stagePath.parent_path();
    else
      return stagePath;
  }(stagePath);
  try {
    if (getStagedDirectory(pathToStage))
      return;

    if (pathToStage.string() == StagedDirectory::RootDirectoryName) {
      databaseConnection(insert_into(stagedDirectoriesTable)
                           .set(stagedDirectoriesTable.name = std::string{
                                  StagedDirectory::RootDirectoryName}));
      return;
    }

    auto parentStagedDirectory = getStagedDirectory(pathToStage.parent_path());
    if (!parentStagedDirectory) {
      add(pathToStage.parent_path());
    }
    parentStagedDirectory = getStagedDirectory(pathToStage.parent_path());

    const auto stagedDirectoryId = databaseConnection(
      insert_into(stagedDirectoriesTable)
        .set(stagedDirectoriesTable.name = pathToStage.filename().string()));
    databaseConnection(
      insert_into(stagedDirectoryParentTable)
        .set(stagedDirectoryParentTable.parentId = parentStagedDirectory->id,
             stagedDirectoryParentTable.childId = stagedDirectoryId));
  } catch (const sqlpp::exception& err) {
    throw StagedDatabaseException(
      "Could not add directory to staged directory database: {}", err);
  }
}

void StagedDatabase::remove(const StagedDirectory& stagedDirectory) {
  try {
    const Size numberOfChildren =
      databaseConnection(
        select(count(1))
          .from(stagedDirectoryParentTable)
          .where(stagedDirectoryParentTable.parentId == stagedDirectory.id))
        .front()
        .count;
    if (numberOfChildren != 0) {
      throw StagedDatabaseException(
        "Can not remove staged directory which is the parent of other staged "
        "directories.");
    }
    databaseConnection(
      remove_from(stagedDirectoryParentTable)
        .where(stagedDirectoryParentTable.childId == stagedDirectory.id));
    databaseConnection(
      remove_from(stagedDirectoriesTable)
        .where(stagedDirectoriesTable.id == stagedDirectory.id));
  } catch (const sqlpp::exception& err) {
    throw StagedDatabaseException(
      "Could not remove directory from staged directory database: {}", err);
  }
}

void StagedDatabase::removeAllDirectories() {
  try {
    databaseConnection(
      remove_from(stagedDirectoryParentTable).unconditionally());
    databaseConnection(remove_from(stagedDirectoriesTable).unconditionally());
    databaseConnection.execute(
      FORMAT_LIB::format("ALTER TABLE {} AUTO_INCREMENT = 1",
                         decltype(stagedDirectoriesTable)::_alias_t::_literal));
  } catch (const sqlpp::exception& err) {
    throw StagedDatabaseException(
      "Could not remove directories from staged directory database: {}", err);
  }
}

auto StagedDatabase::getRootDirectory() -> std::optional<StagedDirectory> {
  return getStagedDirectory(StagedDirectory::RootDirectoryName);
};

auto StagedDatabase::getStagedDirectory(const std::filesystem::path& stagePath)
  -> std::optional<StagedDirectory> {

  StagedDirectory foundStagedDirectory;
  ID currentId;

  try {
    for (const auto& name : stagePath) {
      if (name == StagedDirectory::RootDirectoryName) {
        const auto& results = databaseConnection(
          select(all_of(stagedDirectoriesTable))
            .from(stagedDirectoriesTable)
            .where(stagedDirectoriesTable.name == name.string()));
        if (results.empty()) {
          return std::nullopt;
        }

        const auto& row = results.front();
        currentId = row.id;
        foundStagedDirectory = StagedDirectory{row.id, row.name, row.id};
      } else {
        const auto& results = databaseConnection(
          select(all_of(stagedDirectoriesTable),
                 stagedDirectoryParentTable.parentId)
            .from(stagedDirectoriesTable.join(stagedDirectoryParentTable)
                    .on(stagedDirectoriesTable.id ==
                        stagedDirectoryParentTable.childId))
            .where(stagedDirectoryParentTable.parentId == currentId and
                   stagedDirectoriesTable.name == name.string()));
        if (results.empty()) {
          return std::nullopt;
        }

        const auto& row = results.front();
        currentId = row.id;
        foundStagedDirectory = StagedDirectory{row.id, row.name, row.parentId};
      }
    }
  } catch (const sqlpp::exception& err) {
    throw StagedDatabaseException(
      "Could not get directory from staged directory database: {}", err);
  }

  return foundStagedDirectory;
}
}