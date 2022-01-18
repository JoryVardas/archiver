#ifndef ARCHIVER_ARCHIVED_FILE_DATABASE_HPP
#define ARCHIVER_ARCHIVED_FILE_DATABASE_HPP

#include "../app/archive.h"
#include "../app/archived_file.hpp"
#include "../app/common.h"
#include "../app/staged_file.hpp"
#include "database.hpp"
#include <concepts>

enum class ArchivedFileAddedType : uint8_t { NewRevision, DuplicateRevision };

template <typename T>
concept ArchivedFileDatabase = Database<T> &&
  requires(T t, const StagedFile& stagedFile, const ArchivedFile& archivedFile,
           const ArchivedDirectory& archivedDirectory, const Archive& archive) {
  {
    t.listChildFiles(archivedDirectory)
    } -> std::same_as<std::vector<ArchivedFile>>;
  {
    t.addFile(stagedFile, archivedDirectory, archive)
    } -> std::same_as<ArchivedFileAddedType>;
};

_make_exception_(ArchivedFileDatabaseException);

#endif
