#ifndef _STAGED_DIRECTORY_H
#define _STAGED_DIRECTORY_H

#include "common.h"
#include "directory.h"

using StagedDirectoryID = DirectoryID;

struct StagedDirectory {
  StagedDirectoryID id;
  std::string name;
  StagedDirectoryID parent;

  static constexpr std::string_view RootDirectoryName = "/";
};

#endif