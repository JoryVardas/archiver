#ifndef ARCHIVER_ARCHIVED_DIRECTORY_H
#define ARCHIVER_ARCHIVED_DIRECTORY_H

#include "archive_operation.hpp"
#include "common.h"
#include "directory.h"

using ArchivedDirectoryID = DirectoryID;

struct ArchivedDirectory {
  ArchivedDirectoryID id;
  std::string name;
  ArchivedDirectoryID parent;
  ArchiveOperationID containingArchiveOperation;

  static constexpr std::string_view RootDirectoryName = "/";
};

#endif