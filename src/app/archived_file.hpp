#ifndef ARCHIVER_ARCHIVED_FILE_HPP
#define ARCHIVER_ARCHIVED_FILE_HPP

#include "archive.h"
#include "archive_operation.hpp"
#include "archived_directory.hpp"
#include "common.h"
#include <vector>

using ArchivedFileID = ID;
using ArchivedFileRevisionID = ID;

struct ArchivedFileRevision {
public:
  ArchivedFileRevisionID id;
  std::string hash;
  Size size;
  ArchiveID containingArchiveId;
  ArchiveOperationID containingOperation;
};

struct ArchivedFile {
public:
  ArchivedFileID id;
  std::string name;
  ArchivedDirectory parentDirectory;
  std::vector<ArchivedFileRevision> revisions;
};

#endif
