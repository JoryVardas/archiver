#ifndef ARCHIVER_STAGED_DATABASE_HPP
#define ARCHIVER_STAGED_DATABASE_HPP

#include "../app/common.h"
#include "../app/raw_file.hpp"
#include "../app/staged_directory.h"
#include "../app/staged_file.hpp"
#include "database.hpp"
#include <concepts>
#include <vector>

interface StagedDatabase : public Database {
  // Listing
  virtual auto listAllDirectories() -> std::vector<StagedDirectory> abstract;
  virtual auto listAllFiles() -> std::vector<StagedFile> abstract;
  virtual auto getRootDirectory() -> std::optional<StagedDirectory> abstract;
  // Adding
  virtual void add(const std::filesystem::path& stagePath) abstract;
  virtual auto add(const RawFile& file, const std::filesystem::path& stagePath)
    -> StagedFile abstract;
  // Removing
  virtual void remove(const StagedDirectory& directory) abstract;
  virtual void remove(const StagedFile& stagedFile) abstract;
  virtual void removeAllDirectories() abstract;
  virtual void removeAllFiles() abstract;

  virtual ~StagedDatabase() = default;
};

_make_exception_(StagedDatabaseException);

#endif
