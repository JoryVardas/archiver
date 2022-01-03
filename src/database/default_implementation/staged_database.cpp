#include "staged_database.hpp"
#include "../../app/staged_file.hpp"
#include "../staged_directory_database.hpp"
#include "../staged_file_database.hpp"

using namespace sqlpp;

StagedDatabase::StagedDatabase(
  std::shared_ptr<StagedDatabase::ConnectionConfig>& config)
  : databaseConnection(config) {}

StagedDatabase::~StagedDatabase() {
  try {
    rollback();
  } catch (const std::exception& err) {
    if (std::uncaught_exceptions()) {
      // If there was another exception we should log that an error happened,
      // but eat it.
      spdlog::error(
        fmt::format("There was an error while trying to rollback changes to "
                    "the staged database while destructing the staged database "
                    "due to an exception: {}",
                    err));
    } else {
      throw DatabaseException(fmt::format(
        "There was an error while trying to rollback changes to "
        "the staged database while destructing the staged database: {}",
        err));
    }
  } catch (...) {
    if (std::uncaught_exceptions()) {
      // If there was another exception we should log that an error happened,
      // but eat it.
      spdlog::error(
        "There was an unknown error while trying to rollback changes to "
        "the staged database while destructing the staged database "
        "due to an exception");
    } else {
      throw DatabaseException(
        "There was an unknown error while trying to rollback changes to "
        "the staged database while destructing the staged database");
    }
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
    throw DatabaseException(fmt::format(
      "Could not start a transaction on the staged database: {}"s, err));
  }
}
void StagedDatabase::rollback() {
  if (hasTransaction) {
    try {
      databaseConnection.rollback_transaction(false);
      hasTransaction = false;
    } catch (sqlpp::exception& err) {
      throw DatabaseException(fmt::format(
        "Could not rollback the transaction on the staged database: {}"s, err));
    }
  }
}
void StagedDatabase::commit() {
  if (hasTransaction) {
    try {
      databaseConnection.commit_transaction();
      hasTransaction = false;
    } catch (sqlpp::exception& err) {
      throw DatabaseException(fmt::format(
        "Could not commit the transaction on the staged database: {}"s, err));
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
      stagedFiles.emplace_back(stagedFile.id, stagedFileParent.directoryId,
                               stagedFile.name, stagedFile.size,
                               stagedFile.hash);
    }
  } catch (const sqlpp::exception& err) {
    throw StagedFileDatabaseException(
      fmt::format("Could not list staged files: {}"s, err));
  }
  return stagedFiles;
}

void StagedDatabase::add(const RawFile& file,
                         const std::filesystem::path& stagePath) {
  try {
    auto parentStagedDirectory = getStagedDirectory(stagePath.parent_path());
    if (!parentStagedDirectory) {
      throw StagedFileDatabaseException(fmt::format(
        "Could not add file to staged file database as the parent directory "
        "hasn't been added to the staged directory database"));
    }
    const auto stagedFileId = databaseConnection(
      insert_into(stagedFilesTable)
        .set(stagedFilesTable.name = stagePath.filename().string(),
             stagedFilesTable.hash = file.hash,
             stagedFilesTable.size = file.size));
    databaseConnection(
      insert_into(stagedFileParentTable)
        .set(stagedFileParentTable.fileId = stagedFileId,
             stagedFileParentTable.directoryId = parentStagedDirectory->id()));
  } catch (const sqlpp::exception& err) {
    throw StagedFileDatabaseException(
      fmt::format("Could not add file to staged file database: {}"s, err));
  }
}

void StagedDatabase::remove(const StagedFile& stagedFile) {
  try {
    databaseConnection(remove_from(stagedFilesTable)
                         .where(stagedFilesTable.id == stagedFile.id));
  } catch (const sqlpp::exception& err) {
    throw StagedFileDatabaseException(
      fmt::format("Could not remove file from staged file database: {}"s, err));
  }
}

void StagedDatabase::removeAllFiles() {
  try {
    databaseConnection(remove_from(stagedFilesTable).unconditionally());
  } catch (const sqlpp::exception& err) {
    throw StagedFileDatabaseException(fmt::format(
      "Could not remove files from staged file database: {}"s, err));
  }
}

auto StagedDatabase::listAllDirectories() -> std::vector<StagedDirectory> {
  std::vector<StagedDirectory> stagedDirectories;
  try {
    auto results = databaseConnection(select(all_of(stagedDirectoriesTable))
                                        .from(stagedDirectoriesTable)
                                        .unconditionally());

    for (const auto& stagedDirectory : results) {
      const auto& stagedDirectoryParent =
        databaseConnection(
          select(stagedDirectoryParentTable.parentId)
            .from(stagedDirectoryParentTable)
            .where(stagedDirectoryParentTable.childId == stagedDirectory.id))
          .front();
      stagedDirectories.emplace_back(
        stagedDirectory.id, stagedDirectory.name,
        stagedDirectoryParent.parentId == StagedDirectory::RootDirectoryID
          ? std::optional<ID>{std::nullopt}
          : std::optional<ID>{stagedDirectoryParent.parentId});
    }
  } catch (const sqlpp::exception& err) {
    throw StagedDirectoryDatabaseException(
      fmt::format("Could not list staged directories: {}"s, err));
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

    auto parentStagedDirectory = getStagedDirectory(pathToStage.parent_path());
    if (!parentStagedDirectory) {
      // The database is guaranteed to have at lest "/" as an entry, so there is
      // an endpoint to the recursive call.
      add(pathToStage.parent_path());
    }
    parentStagedDirectory = getStagedDirectory(pathToStage.parent_path());

    const auto stagedDirectoryId = databaseConnection(
      insert_into(stagedDirectoriesTable)
        .set(stagedDirectoriesTable.name = pathToStage.filename().string()));
    databaseConnection(
      insert_into(stagedDirectoryParentTable)
        .set(stagedDirectoryParentTable.parentId = parentStagedDirectory->id(),
             stagedDirectoryParentTable.childId = stagedDirectoryId));
  } catch (const sqlpp::exception& err) {
    throw StagedDirectoryDatabaseException(fmt::format(
      "Could not add directory to staged directory database: {}"s, err));
  }
}

void StagedDatabase::remove(const StagedDirectory& stagedDirectory) {
  try {
    databaseConnection(
      remove_from(stagedDirectoriesTable)
        .where(stagedDirectoriesTable.id == stagedDirectory.id()));
  } catch (const sqlpp::exception& err) {
    throw StagedDirectoryDatabaseException(fmt::format(
      "Could not remove directory from staged directory database: {}"s, err));
  }
}

void StagedDatabase::removeAllDirectories() {
  try {
    databaseConnection(remove_from(stagedDirectoriesTable).unconditionally());
  } catch (const sqlpp::exception& err) {
    throw StagedDirectoryDatabaseException(fmt::format(
      "Could not remove directories from staged directory database: {}"s, err));
  }
}

auto StagedDatabase::getStagedDirectory(const std::filesystem::path& stagePath)
  -> std::optional<StagedDirectory> {
  StagedDirectory foundStagedDirectory = StagedDirectory::getRootDirectory();

  try {
    ID currentId = StagedDirectory::RootDirectoryID;

    for (const auto& name : stagePath) {
      if (name == StagedDirectory::RootDirectoryName)
        continue;

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
      foundStagedDirectory =
        StagedDirectory(row.id, row.name,
                        row.parentId == StagedDirectory::RootDirectoryID
                          ? std::optional<ID>{std::nullopt}
                          : std::optional<ID>{row.parentId});
    }
  } catch (const sqlpp::exception& err) {
    throw StagedDirectoryDatabaseException(fmt::format(
      "Could not get directory from staged directory database: {}"s, err));
  }

  return foundStagedDirectory;
}