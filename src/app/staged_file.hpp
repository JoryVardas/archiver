#ifndef ARCHIVER_STAGED_FILE_HPP
#define ARCHIVER_STAGED_FILE_HPP

#include "common.h"
#include "staged_directory.h"

using StagedFileID = ID;

struct StagedFile {
  StagedFileID id;
  StagedDirectoryID parent;
  std::string name;
  Size size;
  std::string hash;
};
#endif
