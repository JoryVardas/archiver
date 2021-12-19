#ifndef ARCHIVER_STAGED_DIRECTORY_DATABASE_HPP
#define ARCHIVER_STAGED_DIRECTORY_DATABASE_HPP

#include "../app/common.h"
#include "../app/staged_directory.h"
#include "database.hpp"
#include <vector>

class StagedDirectoryDatabase : public Database {
public:
  virtual auto listAll() -> std::vector<StagedDirectory> abstract;
  virtual void add(const std::filesystem::path& stagePath) abstract;
  virtual void remove(const StagedDirectory& directory) abstract;
  virtual void removeAll() abstract;
};

_make_exception_(StagedDirectoryDatabaseException);

#endif
