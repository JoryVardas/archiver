#ifndef ARCHIVER_STAGED_FILE_DATABASE_HPP
#define ARCHIVER_STAGED_FILE_DATABASE_HPP

#include "../app/common.h"
#include "../app/raw_file.hpp"
#include "../app/staged_file.hpp"
#include "database.hpp"
#include <vector>

template <typename T>
concept StagedFileDatabase = Database<T> &&
  requires(T t, const RawFile& file, const std::filesystem::path& stagePath,
           const StagedFile& stagedFile) {
  { t.listAll() } -> std::same_as<std::vector<StagedFile>>;
  t.add(file, stagePath);
  t.remove(stagedFile);
  t.removeAll();
};
_make_exception_(StagedFileDatabaseException);

#endif
