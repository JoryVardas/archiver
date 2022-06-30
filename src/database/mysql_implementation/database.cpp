#include "database.hpp"

namespace database::mysql {
void Database::startTransaction() {
  if (hasTransaction)
    throw DatabaseException(
      "Attempt to start a transaction on the staged database while already "
      "having an existing transaction");

  try {
    databaseConnection.start_transaction();
    hasTransaction = true;
  } catch (sqlpp::exception& err) {
    throw DatabaseException(FORMAT_LIB::format(
      "Could not start a transaction on the staged database: {}", err));
  }
}
void Database::commit() {
  if (hasTransaction) {
    try {
      databaseConnection.commit_transaction();
      hasTransaction = false;
    } catch (sqlpp::exception& err) {
      throw DatabaseException(FORMAT_LIB::format(
        "Could not commit the transaction on the staged database: {}", err));
    }
  }
}
void Database::rollback() {
  if (hasTransaction) {
    try {
      databaseConnection.rollback_transaction(false);
      hasTransaction = false;
    } catch (sqlpp::exception& err) {
      throw DatabaseException(FORMAT_LIB::format(
        "Could not rollback the transaction on the staged database: {}", err));
    }
  }
}

Database::Database(std::shared_ptr<ConnectionConfig>& config)
  : databaseConnection(config) {}
Database::~Database() {
  try {
    rollback();
  } catch (const std::exception& err) {
    spdlog::error(FORMAT_LIB::format(
      "There was an error while trying to rollback changes to "
      "the staged database while destructing the staged database "
      "due to an exception: {}",
      err));
  }
}
}