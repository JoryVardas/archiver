#include "database.h"
#include <iostream>

Database::Database(const DatabaseConnectionParameters& connectionParameters) try
    : _session(mysqlx::SessionOption::HOST, connectionParameters.host,
               mysqlx::SessionOption::PORT, connectionParameters.port,
               mysqlx::SessionOption::USER, connectionParameters.user,
               mysqlx::SessionOption::PWD, connectionParameters.password),
      _schema(_session.getSchema(connectionParameters.schema)) {
} catch (const std::exception& e) {
    throw DatabaseError(
        fmt::format("The following error occured while trying to "
                    "connect to the database:\n\t{}.",
                    e));
} catch (...) {
    throw DatabaseError(fmt::format(
        "An unknown error occured while trying to connect to the database."));
}

mysqlx::abi2::r0::Table Database::getTable(const std::string& table) {
    try {
        return _schema.getTable(table);
    } catch (const std::exception& e) {
        throw DatabaseError(
            fmt::format("The following error occured while trying "
                        "to get the table \"{}\":\n\t{}.",
                        table, e));
    } catch (...) {
        throw DatabaseError(fmt::format(
            "An unknown error occured while trying to get the table \"{}\".",
            table));
    }
};