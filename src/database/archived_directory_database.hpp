#ifndef ARCHIVER_ARCHIVED_DIRECTORY_DATABASE_HPP
#define ARCHIVER_ARCHIVED_DIRECTORY_DATABASE_HPP

#include "../app/archived_directory.hpp"
#include "../app/common.h"
#include "../app/staged_directory.h"
#include "database.hpp"
#include <concepts>

template <typename T>
concept ArchivedDirectoryDatabase = Database<T> &&
  requires(T t, const StagedDirectory& stagedDirectory,
           const ArchivedDirectory& archivedDirectory,
           const ArchivedDirectory& archivedParent) {
  {
    t.listChildDirectories(archivedDirectory)
    } -> std::same_as<std::vector<ArchivedDirectory>>;
  {
    t.addDirectory(stagedDirectory, archivedParent)
    } -> std::same_as<ArchivedDirectory>;
  { t.getRootDirectory() } -> std::same_as<ArchivedDirectory>;
};

_make_exception_(ArchivedDirectoryDatabaseException);

#endif
