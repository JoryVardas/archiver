#ifndef ARCHIVER_ARCHIVED_DIRECTORY_H
#define ARCHIVER_ARCHIVED_DIRECTORY_H

#include "common.h"
#include "directory.h"

using ArchivedDirectoryID = DirectoryID;

struct ArchivedDirectory {
  ArchivedDirectoryID id;
  std::string name;
  ArchivedDirectoryID parent;

  static constexpr ArchivedDirectoryID RootDirectoryID = 1;
  static constexpr std::string_view RootDirectoryName = "/";
};

#endif