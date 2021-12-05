#ifndef ARCHIVER_DATABASE_HPP
#define ARCHIVER_DATABASE_HPP

#include "../app/common.h"

class Database {
public:
  virtual void startTransaction() abstract;
  virtual void commit() abstract;
  virtual void rollback() abstract;

  virtual ~Database() = default;
};

_make_exception_(DatabaseException);

#endif
