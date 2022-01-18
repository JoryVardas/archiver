#ifndef ARCHIVER_ARCHIVE_DATABASE_HPP
#define ARCHIVER_ARCHIVE_DATABASE_HPP

#include "../app/archive.h"
#include "../app/common.h"
#include "../app/staged_file.hpp"
#include "database.hpp"
#include <concepts>

template <typename T>
concept ArchiveDatabase = Database<T> &&
  requires(T t, const StagedFile& stagedFile) {
  { t.getArchiveForFile(stagedFile) } -> std::same_as<Archive>;
};

_make_exception_(ArchiveDatabaseException);

#endif
