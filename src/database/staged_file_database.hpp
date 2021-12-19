#ifndef ARCHIVER_STAGED_FILE_DATABASE_HPP
#define ARCHIVER_STAGED_FILE_DATABASE_HPP

#include "../app/common.h"
#include "../app/raw_file.hpp"
#include "../app/staged_file.hpp"
#include "database.hpp"
#include <vector>

class StagedFileDatabase : public Database {
public:
  virtual auto listAll() -> std::vector<StagedFile> abstract;
  virtual void add(const RawFile& file,
                   const std::filesystem::path& stagePath) abstract;
  virtual void remove(const StagedFile& file) abstract;
  virtual void removeAll() abstract;
};

_make_exception_(StagedFileDatabaseException);

#endif
