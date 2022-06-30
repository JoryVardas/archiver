#ifndef ARCHIVER_MOCK_ARCHIVED_DATABASE_HPP
#define ARCHIVER_MOCK_ARCHIVED_DATABASE_HPP

#include <optional>
#include <src/app/archive_operation.hpp>
#include <src/app/archived_directory.hpp>
#include <src/app/archived_file.hpp>
#include <src/app/common.h>
#include <src/app/staged_directory.h>
#include <src/app/staged_file.hpp>
#include <src/database/archived_database.hpp>
#include <string>
#include <vector>

namespace database::mock {
class ArchivedDatabase : public ::ArchivedDatabase {
public:
  ArchivedDatabase() = delete;
  ArchivedDatabase(const ArchivedDatabase&) = delete;
  ArchivedDatabase(ArchivedDatabase&&) = default;
  explicit ArchivedDatabase(Size archiveTargetSize);
  ~ArchivedDatabase() = default;

  ArchivedDatabase& operator=(const ArchivedDatabase&) = delete;
  ArchivedDatabase& operator=(ArchivedDatabase&&) = default;

  void startTransaction() final;
  void commit() final;
  void rollback() final;

  auto getArchiveForFile(const StagedFile& file) -> Archive final;
  auto getNextArchivePartNumber(const Archive& archive) -> uint64_t final;
  void incrementNextArchivePartNumber(const Archive& archive) final;

  auto listChildDirectories(const ArchivedDirectory& directory)
    -> std::vector<ArchivedDirectory> final;
  auto addDirectory(const StagedDirectory& directory,
                    const ArchivedDirectory& parent,
                    const ArchiveOperationID archiveOperation)
    -> ArchivedDirectory final;
  auto getRootDirectory() -> ArchivedDirectory final;

  auto listChildFiles(const ArchivedDirectory& directory)
    -> std::vector<ArchivedFile> final;
  auto addFile(const StagedFile& stagedFile, const ArchivedDirectory& directory,
               const Archive& archive,
               const ArchiveOperationID archiveOperation)
    -> std::pair<ArchivedFileAddedType, ArchivedFileRevisionID> final;

  auto createArchiveOperation() -> ArchiveOperationID final;

private:
  std::vector<ArchivedDirectory> archivedDirectories = {
    {1, std::string{ArchivedDirectory::RootDirectoryName}, 1, 0}};
  std::vector<ArchivedFile> archivedFiles;
  std::vector<Archive> archives = {{1, "<SINGLE>"}};
  std::vector<std::pair<ArchiveID, uint64_t>> archiveNextPartNumbers;
  std::vector<ArchiveOperation> archiveOperations;
  std::vector<ArchivedDirectory> transactionArchivedDirectories;
  std::vector<ArchivedFile> transactionArchivedFiles;
  std::vector<Archive> transactionArchives;
  std::vector<std::pair<ArchiveID, uint64_t>> transactionArchiveNextPartNumbers;
  std::vector<ArchiveOperation> transactionArchiveOperations;
  bool hasTransaction = false;
  ArchivedFileID nextArchivedFileId = 1;
  ArchivedDirectoryID nextArchivedDirectoryId = 2;
  ArchiveID nextArchiveId = 2;
  ArchivedFileRevisionID nextFileRevisionId = 1;
  ArchiveOperationID nextArchiveOperationId = 1;
  Size targetSize;

  static const std::string noExtensionArchiveContents;

  auto getArchiveForExtension(const std::string& extension) -> Archive;
  auto addArchiveForExtension(const std::string& extension) -> Archive;
  auto getArchiveSize(const Archive& archive) -> Size;

  auto getFileVector() -> decltype(archivedFiles)&;
  auto getDirectoryVector() -> decltype(archivedDirectories)&;
  auto getArchiveVector() -> decltype(archives)&;
  auto getArchivePartNumberVector() -> decltype(archiveNextPartNumbers)&;
  auto getArchiveOperationVector() -> decltype(archiveOperations)&;
};
}
#endif
