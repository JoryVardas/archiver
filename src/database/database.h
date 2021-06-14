#ifndef _DATABASE_H
#define _DATABASE_H

#include "../app/common.h"
#include <exception>
#include <map>
#include <mysqlx/xdevapi.h>
#include <string>

struct DatabaseConnectionParameters {
    std::string host;
    uint64_t port;
    std::string user;
    std::string password;
    std::string schema;
};

_make_exception_(DatabaseError);

struct Database {
    mysqlx::abi2::r0::Table getTable(const std::string& table);

    Database(const DatabaseConnectionParameters& connectionParameters);

  private:
    mysqlx::Session _session;
    mysqlx::Schema _schema;
};

_make_formatter_for_exception_(mysqlx::Error);

#endif