#ifndef ARCHIVER_MYSQL_ARCHIVED_DATABASE_HPP
#define ARCHIVER_MYSQL_ARCHIVED_DATABASE_HPP

#include "../../app/archived_directory.hpp"
#include "../../app/archived_file.hpp"
#include "../../app/common.h"
#include "../../app/staged_directory.h"
#include "../../app/staged_file.hpp"
#include "../archive_database.hpp"
#include "../archived_directory_database.hpp"
#include "../archived_file_database.hpp"
#include "archiver_database.h"
#include "database.hpp"
#include <optional>
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>
#include <string>
#include <vector>

namespace database::mysql {
class ArchivedDatabase {
public:
  using ConnectionConfig = sqlpp::mysql::connection_config;

  ArchivedDatabase() = delete;
  ArchivedDatabase(const ArchivedDatabase&) = delete;
  ArchivedDatabase(ArchivedDatabase&&) = default;
  explicit ArchivedDatabase(std::shared_ptr<ConnectionConfig>& config,
                            Size archiveTargetSize);
  ~ArchivedDatabase();

  ArchivedDatabase& operator=(const ArchivedDatabase&) = delete;
  ArchivedDatabase& operator=(ArchivedDatabase&&) = default;

  void startTransaction();
  void commit();
  void rollback();

  auto getArchiveForFile(const StagedFile& file) -> Archive;
  auto getNextArchivePartNumber(const Archive& archive) -> uint64_t;
  void incrementNextArchivePartNumber(const Archive& archive);

  auto listChildDirectories(const ArchivedDirectory& directory)
    -> std::vector<ArchivedDirectory>;
  auto addDirectory(const StagedDirectory& directory,
                    const ArchivedDirectory& parent) -> ArchivedDirectory;
  auto getRootDirectory() -> ArchivedDirectory;

  auto listChildFiles(const ArchivedDirectory& directory)
    -> std::vector<ArchivedFile>;
  auto addFile(const StagedFile& file, const ArchivedDirectory& directory,
               const Archive& archive) -> ArchivedFileAddedType;

private:
  sqlpp::mysql::connection databaseConnection;
  archiver_database::Archive archivesTable;
  archiver_database::File filesTable;
  archiver_database::FileParent fileParentTable;
  archiver_database::Directory directoriesTable;
  archiver_database::DirectoryParent directoryParentTable;
  archiver_database::FileRevision fileRevisionTable;
  archiver_database::FileRevisionArchive fileRevisionArchiveTable;
  archiver_database::FileRevisionParent fileRevisionParentTable;
  archiver_database::FileRevisionDuplicate fileRevisionDuplicateTable;
  bool hasTransaction = false;
  Size targetSize;

  static const std::string noExtensionArchiveContents;

  auto getArchiveForExtension(const std::string& extension) -> Archive;
  auto addArchiveForExtension(const std::string& extension) -> Archive;
  auto getArchiveSize(const Archive& archive) -> Size;
  auto getFileRevisionsForFile(ArchivedFileID fileId)
    -> std::vector<ArchivedFileRevision>;
  auto getFileId(const std::string& name, const ArchivedDirectory& directory)
    -> std::optional<ArchivedFileID>;
  //  auto addNewFile(std::string_view name, const ArchivedDirectory& directory)
  //    -> ArchivedFile;
  auto findDuplicateRevisionId(const StagedFile& file)
    -> std::optional<ArchivedFileRevisionID>;
};
}
#endif
