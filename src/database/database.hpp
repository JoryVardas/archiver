#ifndef ARCHIVER_DATABASE_HPP
#define ARCHIVER_DATABASE_HPP

#include "../app/common.h"
#include <concepts>

interface Database {
  virtual void startTransaction() abstract;
  virtual void commit() abstract;
  virtual void rollback() abstract;
  
  virtual ~Database() = default;
};

_make_exception_(DatabaseException);

#endif
