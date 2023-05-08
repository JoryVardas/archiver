#ifndef ARCHIVER_ARCHIVED_FILE_REVISION_HPP
#define ARCHIVER_ARCHIVED_FILE_REVISION_HPP

#include "archive.h"
#include "archive_operation.hpp"
#include "common.h"

using ArchivedFileRevisionID = ID;

struct ArchivedFileRevision {
public:
  ArchivedFileRevisionID id;
  std::string hash;
  Size size;
  ArchiveID containingArchiveId;
  ArchiveOperationID containingOperation;
  bool isDuplicate;
};
#endif
