#ifndef ARCHIVER_TEST_DATABASE_CONNECTOR_HPP
#define ARCHIVER_TEST_DATABASE_CONNECTOR_HPP

#include "database/mock_implementation/archived_database.hpp"
#include "database/mock_implementation/staged_database.hpp"
#include <src/config/config.h>
#include <src/database/mysql_implementation/archived_database.hpp>
#include <src/database/mysql_implementation/staged_database.hpp>
#include <utility>

using MysqlDatabase =
  std::pair<database::mysql::StagedDatabase, database::mysql::ArchivedDatabase>;
using MockDatabase =
  std::pair<database::mock::StagedDatabase, database::mock::ArchivedDatabase>;

template <typename Database> struct DatabaseConnectorBase {
  using StagedDatabase = decltype(Database::first);
  using ArchivedDatabase = decltype(Database::second);
};
template <typename Database> struct DatabaseConnector {};
template <>
struct DatabaseConnector<MysqlDatabase>
  : public DatabaseConnectorBase<MysqlDatabase> {
  auto connect(const Config& config, std::span<char> readBuffer) {
    auto databaseConnectionConfig =
      std::make_shared<StagedDatabase::ConnectionConfig>();
    databaseConnectionConfig->database = config.database.location.schema;
    databaseConnectionConfig->host = config.database.location.host;
    databaseConnectionConfig->port = config.database.location.port;
    databaseConnectionConfig->user = config.database.user;
    databaseConnectionConfig->password = config.database.password;

    auto stagedDatabase =
      std::make_shared<StagedDatabase>(databaseConnectionConfig);
    auto archivedDatabase = std::make_shared<ArchivedDatabase>(
      databaseConnectionConfig, config.archive.target_size);
    const ArchivedDirectory archivedRootDirectory =
      archivedDatabase->getRootDirectory();

    return std::pair{
      std::static_pointer_cast<::StagedDatabase>(stagedDatabase),
      std::static_pointer_cast<::ArchivedDatabase>(archivedDatabase)};
  };
};
template <>
struct DatabaseConnector<MockDatabase>
  : public DatabaseConnectorBase<MockDatabase> {
  auto connect(const Config& config, std::span<char> readBuffer) {
    auto stagedDatabase = std::make_shared<StagedDatabase>();
    auto archivedDatabase =
      std::make_shared<ArchivedDatabase>(config.archive.target_size);
    const ArchivedDirectory archivedRootDirectory =
      archivedDatabase->getRootDirectory();

    return std::pair{
      std::static_pointer_cast<::StagedDatabase>(stagedDatabase),
      std::static_pointer_cast<::ArchivedDatabase>(archivedDatabase)};
  };
};

#endif
