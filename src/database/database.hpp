#ifndef ARCHIVER_DATABASE_HPP
#define ARCHIVER_DATABASE_HPP

#include "../app/common.h"
#include <concepts>

template <typename T>
concept Database = requires(T t) {
  t.startTransaction();
  t.commit();
  t.rollback();
};

_make_exception_(DatabaseException);

#endif
