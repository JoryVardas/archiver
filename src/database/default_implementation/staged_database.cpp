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
      stagedFiles.emplace_back(stagedFile.id, stagedFile.parentId,
                               stagedFile.name, stagedFile.size,
                               stagedFile.sha3, stagedFile.blake2B);
    }
  } catch (const sqlpp::exception& err) {
    throw StagedFileDatabaseException(
      fmt::format("Could not list staged files: {}"s, err));
  }
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
    databaseConnection(
      insert_into(stagedFilesTable)
        .set(stagedFilesTable.parentId = parentStagedDirectory->id(),
             stagedFilesTable.name = stagePath.filename(),
             stagedFilesTable.sha3 = file.shaHash,
             stagedFilesTable.blake2B = file.blakeHash,
             stagedFilesTable.size = file.size));
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
      stagedDirectories.emplace_back(
        stagedDirectory.directoryId, stagedDirectory.name,
        stagedDirectory.parentId == StagedDirectory::RootDirectoryID
          ? std::optional<ID>{std::nullopt}
          : std::optional<ID>{stagedDirectory.parentId});
    }
  } catch (const sqlpp::exception& err) {
    throw StagedDirectoryDatabaseException(
      fmt::format("Could not list staged directories: {}"s, err));
  }
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

    databaseConnection(
      insert_into(stagedDirectoriesTable)
        .set(stagedDirectoriesTable.parentId = parentStagedDirectory->id(),
             stagedDirectoriesTable.name = pathToStage.filename()));
  } catch (const sqlpp::exception& err) {
    throw StagedDirectoryDatabaseException(fmt::format(
      "Could not add directory to staged directory database: {}"s, err));
  }
}

void StagedDatabase::remove(const StagedDirectory& stagedDirectory) {
  try {
    databaseConnection(
      remove_from(stagedDirectoriesTable)
        .where(stagedDirectoriesTable.directoryId == stagedDirectory.id()));
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
  try {
    ID currentId = StagedDirectory::RootDirectoryID;

    for (const auto& name : stagePath) {
      if (name == StagedDirectory::RootDirectoryName)
        continue;

      const auto& row = databaseConnection(
        select(all_of(stagedDirectoriesTable))
          .from(stagedDirectoriesTable)
          .where(stagedDirectoriesTable.parentId == currentId and
                 stagedDirectoriesTable.name == name));
      if (row.empty()) {
        return std::nullopt;
      }

      currentId = row.front().directoryId;
    }
  } catch (const sqlpp::exception& err) {
    throw StagedDirectoryDatabaseException(fmt::format(
      "Could not get directory from staged directory database: {}"s, err));
  }
}