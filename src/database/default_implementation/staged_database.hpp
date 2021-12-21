#ifndef ARCHIVER_STAGED_DATABASE_HPP
#define ARCHIVER_STAGED_DATABASE_HPP

#include "../../app/common.h"
#include "../../app/raw_file.hpp"
#include "../../app/staged_directory.h"
#include "../../app/staged_file.hpp"
#include "archiver_database.h"
#include <optional>
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>
#include <vector>

_make_formatter_for_exception_(sqlpp::exception);

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
  void add(const RawFile& file, const std::filesystem::path& stagePath);
  void remove(const StagedFile& stagedFile);
  void removeAllFiles();

  auto listAllDirectories() -> std::vector<StagedDirectory>;
  void add(const std::filesystem::path& stagePath);
  void remove(const StagedDirectory& stagedDirectory);
  void removeAllDirectories();

private:
  sqlpp::mysql::connection databaseConnection;
  archiver_database::StagedFile stagedFilesTable;
  archiver_database::StagedDirectory stagedDirectoriesTable;
  bool hasTransaction = false;

  auto getStagedDirectory(const std::filesystem::path& stagePath)
    -> std::optional<StagedDirectory>;
};

#endif
