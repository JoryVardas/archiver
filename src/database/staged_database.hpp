#ifndef ARCHIVER_STAGED_DATABASE_HPP
#define ARCHIVER_STAGED_DATABASE_HPP

#include "../app/common.h"
#include "../app/raw_file.hpp"
#include "../app/staged_directory.h"
#include "../app/staged_file.hpp"
#include "database.hpp"
#include <concepts>
#include <vector>

template <typename T>
concept StagedDatabase = Database<T> &&
  requires(T t, const RawFile& file, const std::filesystem::path& stagePath,
           const StagedFile& stagedFile, const StagedDirectory& directory) {
  // Listing
  { t.listAllDirectories() } -> std::same_as<std::vector<StagedDirectory>>;
  { t.listAllFiles() } -> std::same_as<std::vector<StagedFile>>;
  { t.getRootDirectory() } -> std::same_as<std::optional<StagedDirectory>>;
  // Adding
  t.add(stagePath);
  { t.add(file, stagePath) } -> std::same_as<StagedFile>;
  // Removing
  t.remove(directory);
  t.remove(stagedFile);
  t.removeAllDirectories();
  t.removeAllFiles();
};
_make_exception_(StagedDatabaseException);

#endif
