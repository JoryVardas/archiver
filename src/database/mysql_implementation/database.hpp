#ifndef ARCHIVER_DEFAULT_IMPLEMENTATION_DATABASE_HPP
#define ARCHIVER_DEFAULT_IMPLEMENTATION_DATABASE_HPP

#include "../../app/common.h"
#include "../database.hpp"
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>

namespace database::mysql {
class Database : public ::Database {
public:
  using ConnectionConfig = sqlpp::mysql::connection_config;

  void startTransaction();
  void commit();
  void rollback();

protected:
  sqlpp::mysql::connection databaseConnection;
  bool hasTransaction = false;

public:
  Database() = delete;
  Database(const Database&) = delete;
  Database(Database&&) = default;
  explicit Database(std::shared_ptr<ConnectionConfig>& config);
  virtual ~Database();

  Database& operator=(const Database&) = delete;
  Database& operator=(Database&&) = default;
};
}

_make_formatter_for_exception_(sqlpp::exception);

#endif
