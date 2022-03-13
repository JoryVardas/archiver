#ifndef ARCHIVER_MYSQL_STAGED_DATABASE_HPP
#define ARCHIVER_MYSQL_STAGED_DATABASE_HPP

#include "../../app/common.h"
#include "../../app/raw_file.hpp"
#include "../../app/staged_directory.h"
#include "../../app/staged_file.hpp"
#include "../staged_directory_database.hpp"
#include "../staged_file_database.hpp"
#include "archiver_database.h"
#include "database.hpp"
#include <optional>
#include <vector>

namespace database::mysql {
class StagedDatabase {
public:
  using ConnectionConfig = sqlpp::mysql::connection_config;

  StagedDatabase() = delete;
  StagedDatabase(const StagedDatabase&) = delete;
  StagedDatabase(StagedDatabase&&) = default;
  explicit StagedDatabase(std::shared_ptr<ConnectionConfig>& config);
  ~StagedDatabase();

  StagedDatabase& operator=(const StagedDatabase&) = delete;
  StagedDatabase& operator=(StagedDatabase&&) = default;

  void startTransaction();
  void commit();
  void rollback();

  auto listAllFiles() -> std::vector<StagedFile>;
  auto add(const RawFile& file, const std::filesystem::path& stagePath)
    -> StagedFile;
  void remove(const StagedFile& stagedFile);
  void removeAllFiles();

  auto listAllDirectories() -> std::vector<StagedDirectory>;
  void add(const std::filesystem::path& stagePath);
  void remove(const StagedDirectory& stagedDirectory);
  void removeAllDirectories();
  auto getRootDirectory() -> std::optional<StagedDirectory>;

private:
  sqlpp::mysql::connection databaseConnection;
  archiver_database::StagedFile stagedFilesTable;
  archiver_database::StagedFileParent stagedFileParentTable;
  archiver_database::StagedDirectory stagedDirectoriesTable;
  archiver_database::StagedDirectoryParent stagedDirectoryParentTable;
  bool hasTransaction = false;

  auto getStagedDirectory(const std::filesystem::path& stagePath)
    -> std::optional<StagedDirectory>;
};
}

#endif
