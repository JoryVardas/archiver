#ifndef ARCHIVER_STAGED_DIRECTORY_DATABASE_HPP
#define ARCHIVER_STAGED_DIRECTORY_DATABASE_HPP

#include "../app/common.h"
#include "../app/staged_directory.h"
#include "database.hpp"
#include <concepts>
#include <vector>

template <typename T>
concept StagedDirectoryDatabase = Database<T> &&
  requires(T t, const std::filesystem::path& stagePath,
           const StagedDirectory& directory) {
  { t.listAllDirectories() } -> std::same_as<std::vector<StagedDirectory>>;
  t.add(stagePath);
  t.remove(directory);
  t.removeAllDirectories();
};

_make_exception_(StagedDirectoryDatabaseException);

#endif
