#include "archived_database.hpp"

using namespace sqlpp;
using namespace std::string_literals;

const std::string ArchivedDatabase::noExtensionArchiveContents = "<BLANK>"s;

ArchivedDatabase::ArchivedDatabase(
  std::shared_ptr<ArchivedDatabase::ConnectionConfig>& config,
  Size archiveTargetSize)
  : databaseConnection(config), targetSize(archiveTargetSize) {}

ArchivedDatabase::~ArchivedDatabase() {
  try {
    rollback();
  } catch (const std::exception& err) {
    if (std::uncaught_exceptions()) {
      // If there was another exception we should log that an error happened,
      // but eat it.
      spdlog::error(FORMAT_LIB::format(
        "There was an error while trying to rollback changes to "
        "the archived database while destructing the staged database "
        "due to an exception: {}",
        err));
    } else {
      throw DatabaseException(FORMAT_LIB::format(
        "There was an error while trying to rollback changes to "
        "the archived database while destructing the archived database: {}",
        err));
    }
  } catch (...) {
    if (std::uncaught_exceptions()) {
      // If there was another exception we should log that an error happened,
      // but eat it.
      spdlog::error(
        "There was an unknown error while trying to rollback changes to "
        "the archived database while destructing the staged database "
        "due to an exception");
    } else {
      throw DatabaseException(
        "There was an unknown error while trying to rollback changes to "
        "the archived database while destructing the staged database");
    }
  }
}

void ArchivedDatabase::startTransaction() {
  if (hasTransaction)
    throw DatabaseException(
      "Attempt to start a transaction on the archived database while already "
      "having an existing transaction");

  try {
    databaseConnection.start_transaction();
    hasTransaction = true;
  } catch (sqlpp::exception& err) {
    throw DatabaseException(FORMAT_LIB::format(
      "Could not start a transaction on the archived database: {}"s, err));
  }
}
void ArchivedDatabase::rollback() {
  if (hasTransaction) {
    try {
      databaseConnection.rollback_transaction(false);
      hasTransaction = false;
    } catch (sqlpp::exception& err) {
      throw DatabaseException(FORMAT_LIB::format(
        "Could not rollback the transaction on the archived database: {}"s,
        err));
    }
  }
}
void ArchivedDatabase::commit() {
  if (hasTransaction) {
    try {
      databaseConnection.commit_transaction();
      hasTransaction = false;
    } catch (sqlpp::exception& err) {
      throw DatabaseException(FORMAT_LIB::format(
        "Could not commit the transaction on the archived database: {}"s, err));
    }
  }
}

auto ArchivedDatabase::getArchiveForFile(const StagedFile& file) -> Archive {
  const std::string_view fileName = file.name;
  const auto fileExtension =
    std::string{fileName.substr(fileName.find_last_of('.'))};
  auto archiveForExtension = getArchiveForExtension(fileExtension);

  if (getArchiveSize(archiveForExtension) < targetSize) {
    return archiveForExtension;
  }

  return addArchiveForExtension(fileExtension);
}
auto ArchivedDatabase::getNextArchivePartNumber(const Archive& archive)
  -> uint64_t {
  try {
    auto archiveResults =
      databaseConnection(select(archivesTable.nextPartNumber)
                           .from(archivesTable)
                           .where(archivesTable.id == archive.id)
                           .limit(1u));

    if (archiveResults.empty())
      throw ArchiveDatabaseException(
        FORMAT_LIB::format("Could not find archive with id {}"s, archive.id));
    else
      return archiveResults.front().nextPartNumber;
  } catch (const sqlpp::exception& err) {
    throw ArchiveDatabaseException(FORMAT_LIB::format(
      "Could not get next archive part number for archive with id {}: {}"s,
      archive.id, err));
  }
}
void ArchivedDatabase::incrementNextArchivePartNumber(const Archive& archive) {
  try {
    auto rowsUpdated = databaseConnection(
      update(archivesTable)
        .set(archivesTable.nextPartNumber = archivesTable.nextPartNumber + 1)
        .where(archivesTable.id == archive.id));

    if (rowsUpdated != 1)
      throw ArchiveDatabaseException(FORMAT_LIB::format(
        "Could not update next archive part number of archive with id {}"s,
        archive.id));
  } catch (const sqlpp::exception& err) {
    throw ArchiveDatabaseException(FORMAT_LIB::format(
      "Could not update next archive part number of archive with id {}"s,
      archive.id, err));
  }
}

auto ArchivedDatabase::getArchiveForExtension(const std::string& extension)
  -> Archive {
  std::string extensionName = extension;
  try {
    if (extension.empty()) {
      extensionName = noExtensionArchiveContents;
    }
    auto archiveResults =
      databaseConnection(select(all_of(archivesTable))
                           .from(archivesTable)
                           .where(archivesTable.contents == extensionName)
                           .limit(1u));

    if (archiveResults.empty())
      return addArchiveForExtension(extensionName);
    else
      return {archiveResults.front().id, archiveResults.front().contents};
  } catch (const sqlpp::exception& err) {
    throw ArchiveDatabaseException(FORMAT_LIB::format(
      "Could not get archive for extension \"{}\": {}"s, extensionName, err));
  }
}

auto ArchivedDatabase::addArchiveForExtension(const std::string& extension)
  -> Archive {
  std::string extensionName = extension;
  try {
    if (extension.empty()) {
      extensionName = noExtensionArchiveContents;
    }

    auto archiveId = databaseConnection(
      insert_into(archivesTable).set(archivesTable.contents = extensionName));

    return {archiveId, extensionName};
  } catch (const sqlpp::exception& err) {
    throw ArchiveDatabaseException(FORMAT_LIB::format(
      "Could not add archive for extension \"{}\": {}"s, extensionName, err));
  }
}

auto ArchivedDatabase::getArchiveSize(const Archive& archive) -> Size {
  try {
    return databaseConnection(
             select(sum(fileRevisionTable.size))
               .from(fileRevisionTable.join(fileRevisionArchiveTable)
                       .on(fileRevisionTable.id ==
                           fileRevisionArchiveTable.revisionId))
               .where(fileRevisionArchiveTable.archiveId == archive.id))
      .front()
      .sum;
  } catch (const sqlpp::exception& err) {
    throw ArchiveDatabaseException(FORMAT_LIB::format(
      "Could not get archive size for archive with id {}: {}"s, archive.id,
      err));
  }
}

auto ArchivedDatabase::listChildDirectories(const ArchivedDirectory& directory)
  -> std::vector<ArchivedDirectory> {
  try {
    auto results = databaseConnection(
      select(all_of(directoriesTable))
        .from(directoriesTable.join(directoryParentTable)
                .on(directoriesTable.id == directoryParentTable.childId))
        .where(directoryParentTable.parentId == directory.id));

    std::vector<ArchivedDirectory> childDirectories;

    for (const auto& row : results) {
      childDirectories.push_back({row.id, row.name.value(), directory.id});
    }

    return childDirectories;

  } catch (const sqlpp::exception& err) {
    throw ArchivedDirectoryDatabaseException(FORMAT_LIB::format(
      "Could not list child directories for archived directory with id {}: {}"s,
      directory.id, err));
  }
}
auto ArchivedDatabase::addDirectory(const StagedDirectory& directory,
                                    const ArchivedDirectory& parent)
  -> ArchivedDirectory {
  if (directory.name == ArchivedDirectory::RootDirectoryName)
    return getRootDirectory();
  try {
    auto matchingDirectory = databaseConnection(
      select(directoriesTable.id)
        .from(directoryParentTable.join(directoriesTable)
                .on(directoryParentTable.childId == directoriesTable.id))
        .where(directoryParentTable.parentId == parent.id and
               directoriesTable.name == directory.name));
    if (!matchingDirectory.empty()) {
      return {matchingDirectory.front().id, directory.name, parent.id};
    }

    auto directoryId =
      databaseConnection(insert_into(directoriesTable)
                           .set(directoriesTable.name = directory.name));
    databaseConnection(insert_into(directoryParentTable)
                         .set(directoryParentTable.parentId = parent.id,
                              directoryParentTable.childId = directoryId));

    return {directoryId, directory.name, parent.id};
  } catch (const sqlpp::exception& err) {
    throw ArchivedDirectoryDatabaseException(FORMAT_LIB::format(
      "Could not add directory to archived directories: {}"s, err));
  }
}
auto ArchivedDatabase::getRootDirectory() -> ArchivedDirectory {
  try {
    auto result = databaseConnection(
      select(all_of(directoriesTable))
        .from(directoriesTable)
        .where(directoriesTable.name ==
               std::string{ArchivedDirectory::RootDirectoryName}));
    const auto& rootDirectory = result.front();
    return {rootDirectory.id, rootDirectory.name, rootDirectory.id};
  } catch (const sqlpp::exception& err) {
    throw ArchivedDirectoryDatabaseException(FORMAT_LIB::format(
      "Could not get the root archived directory: {}"s, err));
  }
}

auto ArchivedDatabase::listChildFiles(const ArchivedDirectory& directory)
  -> std::vector<ArchivedFile> {
  try {
    auto results =
      databaseConnection(select(all_of(filesTable))
                           .from(filesTable.join(fileParentTable)
                                   .on(filesTable.id == fileParentTable.fileId))
                           .where(fileParentTable.directoryId == directory.id));

    std::vector<ArchivedFile> childFiles;

    for (const auto& row : results) {
      childFiles.push_back(
        {row.id, row.name, directory, getFileRevisionsForFile(row.id)});
    }

    return childFiles;

  } catch (const sqlpp::exception& err) {
    throw ArchivedFileDatabaseException(FORMAT_LIB::format(
      "Could not list child files for archived directory with id {}: {}"s,
      directory.id, err));
  }
}

namespace get_file_revisions_for_file_table_alias {
SQLPP_ALIAS_PROVIDER(DuplicateRevisionTable);
SQLPP_ALIAS_PROVIDER(RelevantRevisionTable);
SQLPP_ALIAS_PROVIDER(RelevantRevisionWithDuplicateTable);
SQLPP_ALIAS_PROVIDER(revisionId);
SQLPP_ALIAS_PROVIDER(revisionSize);
SQLPP_ALIAS_PROVIDER(revisionHash);
SQLPP_ALIAS_PROVIDER(revisionArchiveId);
SQLPP_ALIAS_PROVIDER(revisionArchiveTime);
SQLPP_ALIAS_PROVIDER(isDuplicate);
}
auto ArchivedDatabase::getFileRevisionsForFile(ArchivedFileID fileId)
  -> std::vector<ArchivedFileRevision> {
  using namespace get_file_revisions_for_file_table_alias;
  try {
    auto duplicateRevisionTable = fileRevisionTable.as(DuplicateRevisionTable);
    auto relevantFileRevisions =
      select(
        all_of(fileRevisionTable),
        fileRevisionDuplicateTable.originalRevisionId,
        fileRevisionDuplicateTable.revisionId.is_not_null().as(isDuplicate))
        .from(
          fileRevisionTable.join(fileRevisionParentTable)
            .on(fileRevisionTable.id == fileRevisionParentTable.revisionId)
            .left_outer_join(fileRevisionDuplicateTable)
            .on(fileRevisionTable.id == fileRevisionDuplicateTable.revisionId))
        .where(fileRevisionParentTable.fileId == fileId)
        .as(RelevantRevisionTable);
    auto relevantFileRevisionsWithDuplicateInfo =
      select(case_when(relevantFileRevisions.isDuplicate == false)
               .then(relevantFileRevisions.id)
               .else_(duplicateRevisionTable.id)
               .as(revisionId),
             case_when(relevantFileRevisions.isDuplicate == false)
               .then(relevantFileRevisions.hash)
               .else_(duplicateRevisionTable.hash)
               .as(revisionHash),
             case_when(relevantFileRevisions.isDuplicate == false)
               .then(relevantFileRevisions.size)
               .else_(duplicateRevisionTable.size)
               .as(revisionSize),
             relevantFileRevisions.archiveTime.as(revisionArchiveTime))
        .from(relevantFileRevisions.left_outer_join(duplicateRevisionTable)
                .on(relevantFileRevisions.originalRevisionId ==
                    duplicateRevisionTable.id))
        .unconditionally()
        .as(RelevantRevisionWithDuplicateTable);
    auto relevantFileRevisionsWithDuplicateAndArchiveInfo =
      select(all_of(relevantFileRevisionsWithDuplicateInfo),
             fileRevisionArchiveTable.archiveId.as(revisionArchiveId))
        .from(relevantFileRevisionsWithDuplicateInfo
                .left_outer_join(fileRevisionArchiveTable)
                .on(relevantFileRevisionsWithDuplicateInfo.revisionId ==
                    fileRevisionArchiveTable.revisionId))
        .unconditionally();
    auto fileRevisionResults =
      databaseConnection(relevantFileRevisionsWithDuplicateAndArchiveInfo);

    std::vector<ArchivedFileRevision> revisions;

    for (const auto& row : fileRevisionResults) {
      ArchivedFileRevision a = {
        row.revisionId, row.revisionHash, row.revisionSize,
        row.revisionArchiveTime.value(), row.revisionArchiveId};
      revisions.push_back(a);
    }

    return revisions;

  } catch (const sqlpp::exception& err) {
    throw ArchivedFileDatabaseException(FORMAT_LIB::format(
      "Could not list revisions for archived file with id {}: {}"s, fileId,
      err));
  }
}

auto ArchivedDatabase::addFile(const StagedFile& file,
                               const ArchivedDirectory& directory,
                               const Archive& archive)
  -> ArchivedFileAddedType {
  try {
    const auto fileId = [&]() {
      auto existingFile = getFileId(file.name, directory);
      if (!existingFile) {
        auto newFileId = databaseConnection(
          insert_into(filesTable).set(filesTable.name = file.name));
        databaseConnection(insert_into(fileParentTable)
                             .set(fileParentTable.fileId = newFileId,
                                  fileParentTable.directoryId = directory.id));
        return newFileId;
      }
      return existingFile.value();
    }();

    const auto [revisionId, revisionIsDuplicate] =
      [&]() -> std::pair<ArchivedFileRevisionID, bool> {
      const auto duplicateRevision = findDuplicateRevisionId(file);
      if (duplicateRevision) {
        auto newRevisionId =
          databaseConnection(insert_into(fileRevisionTable)
                               .set(fileRevisionTable.archiveTime =
                                      verbatim_t<sqlpp::time_point>("NOW()")));
        databaseConnection(
          insert_into(fileRevisionDuplicateTable)
            .set(fileRevisionDuplicateTable.revisionId = newRevisionId,
                 fileRevisionDuplicateTable.originalRevisionId =
                   duplicateRevision.value()));
        return {newRevisionId, true};
      } else {
        auto newRevisionId =
          databaseConnection(insert_into(fileRevisionTable)
                               .set(fileRevisionTable.archiveTime =
                                      verbatim_t<sqlpp::time_point>("NOW()"),
                                    fileRevisionTable.hash = file.hash,
                                    fileRevisionTable.size = file.size));
        databaseConnection(
          insert_into(fileRevisionArchiveTable)
            .set(fileRevisionArchiveTable.revisionId = newRevisionId,
                 fileRevisionArchiveTable.archiveId = archive.id));
        return {newRevisionId, false};
      }
    }();

    databaseConnection(insert_into(fileRevisionParentTable)
                         .set(fileRevisionParentTable.revisionId = revisionId,
                              fileRevisionParentTable.fileId = fileId));

    return revisionIsDuplicate ? ArchivedFileAddedType::DuplicateRevision
                               : ArchivedFileAddedType::NewRevision;
  } catch (const sqlpp::exception& err) {
    throw ArchivedFileDatabaseException(
      FORMAT_LIB::format("Could not add file to archived files: {}"s, err));
  }
}

auto ArchivedDatabase::getFileId(const std::string& name,
                                 const ArchivedDirectory& directory)
  -> std::optional<ArchivedFileID> {
  try {
    auto results =
      databaseConnection(select(fileParentTable.fileId)
                           .from(filesTable.join(fileParentTable)
                                   .on(filesTable.id == fileParentTable.fileId))
                           .where(filesTable.name == name and
                                  fileParentTable.directoryId == directory.id));
    if (results.empty())
      return std::nullopt;
    return results.front().fileId;
  } catch (const sqlpp::exception& err) {
    throw ArchivedFileDatabaseException(
      FORMAT_LIB::format("Could not find file with name \"{}\" and with parent "
                         "directory id {}: {}",
                         name, directory.id, err));
  }
}
auto ArchivedDatabase::findDuplicateRevisionId(const StagedFile& file)
  -> std::optional<ArchivedFileRevisionID> {
  try {
    auto results =
      databaseConnection(select(fileRevisionTable.id)
                           .from(fileRevisionTable)
                           .where(fileRevisionTable.size == file.size and
                                  fileRevisionTable.hash == file.hash));
    if (results.empty())
      return std::nullopt;
    return results.front().id;
  } catch (const sqlpp::exception& err) {
    throw ArchivedFileDatabaseException(FORMAT_LIB::format(
      "Could not find duplicate revision for staged file with id "
      "{}, name \"{}\", and hash \"{}\": {}",
      file.id, file.name, file.hash, err));
  }
}