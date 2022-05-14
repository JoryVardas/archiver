#ifndef ARCHIVER_ARCHIVE_OPERATION_HPP
#define ARCHIVER_ARCHIVE_OPERATION_HPP

#include "common.h"

typedef uint64_t ArchiveOperationID;

struct ArchiveOperation {
  ArchiveOperationID id;
  TimeStamp archiveTime;
};

#endif
