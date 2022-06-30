#ifndef ARCHIVER_ARCHIVED_DATABASE_HPP
#define ARCHIVER_ARCHIVED_DATABASE_HPP

#include "../app/archive.h"
#include "../app/archive_operation.hpp"
#include "../app/archived_directory.hpp"
#include "../app/archived_file.hpp"
#include "../app/common.h"
#include "../app/staged_directory.h"
#include "../app/staged_file.hpp"
#include "database.hpp"
#include <concepts>

enum class ArchivedFileAddedType : uint8_t { NewRevision, DuplicateRevision };

interface ArchivedDatabase : public Database {
  // Listing
  virtual auto listChildDirectories(const ArchivedDirectory& archivedDirectory)
    -> std::vector<ArchivedDirectory> abstract;
  virtual auto listChildFiles(const ArchivedDirectory& archivedDirectory)
    -> std::vector<ArchivedFile> abstract;
  virtual auto getArchiveForFile(const StagedFile& stagedFile)
    -> Archive abstract;
  virtual auto getNextArchivePartNumber(const Archive& archive)
    -> uint64_t abstract;
  virtual auto getRootDirectory() -> ArchivedDirectory abstract;
  // Adding
  virtual auto createArchiveOperation() -> ArchiveOperationID abstract;
  virtual auto addDirectory(const StagedDirectory& stagedDirectory,
                            const ArchivedDirectory& archivedParent,
                            const ArchiveOperationID archiveOperation)
    -> ArchivedDirectory abstract;
  virtual auto addFile(const StagedFile& stagedFile,
                       const ArchivedDirectory& archivedDirectory,
                       const Archive& archive,
                       const ArchiveOperationID archiveOperation)
    -> std::pair<ArchivedFileAddedType, ArchivedFileRevisionID> abstract;
  // Updating
  virtual void incrementNextArchivePartNumber(const Archive& archive) abstract;

  virtual ~ArchivedDatabase() = default;
};

_make_exception_(ArchivedDatabaseException);

#endif
