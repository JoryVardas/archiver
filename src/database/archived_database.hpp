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

template <typename T>
concept ArchivedDatabase =
  Database<T> &&
  requires(T t, const StagedFile& stagedFile, const ArchivedFile& archivedFile,
           const StagedDirectory& stagedDirectory,
           const ArchivedDirectory& archivedDirectory,
           const ArchivedDirectory& archivedParent, const Archive& archive,
           const ArchiveOperationID archiveOperation) {
    /* clang-format off */
  // Listing
  { t.listChildDirectories(archivedDirectory) }
      -> std::same_as<std::vector<ArchivedDirectory>>;
  { t.listChildFiles(archivedDirectory) }
      -> std::same_as<std::vector<ArchivedFile>>;
  { t.getArchiveForFile(stagedFile) } -> std::same_as<Archive>;
  { t.getNextArchivePartNumber(archive) } -> std::same_as<uint64_t>;
  { t.getRootDirectory() } -> std::same_as<ArchivedDirectory>;
  // Adding
  { t.createArchiveOperation()} -> std::same_as<ArchiveOperationID>;
  { t.addDirectory(stagedDirectory, archivedParent,  archiveOperation) }
      -> std::same_as<ArchivedDirectory>;
  { t.addFile(stagedFile, archivedDirectory, archive, archiveOperation) }
      -> std::same_as<std::pair<ArchivedFileAddedType, ArchivedFileRevisionID>>;
  // Updating
  t.incrementNextArchivePartNumber(archive);
    /* clang-format on */
  };

_make_exception_(ArchivedDatabaseException);

#endif
