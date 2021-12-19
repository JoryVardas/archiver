#ifndef ARCHIVER_STAGED_FILE_HPP
#define ARCHIVER_STAGED_FILE_HPP

#include "common.h"
#include "file.h"
#include "staged_directory.h"

using StagedFileID = FileID;

struct StagedFile {
public:
  StagedFile(StagedFileID id, StagedDirectoryID parent,
             const std::string& stagedName, Size size,
             const std::string& shaHash, const std::string& blakeHash);

  StagedFile() = delete;
  StagedFile(const StagedFile&) = default;
  StagedFile(StagedFile&&) = default;
  StagedFile& operator=(const StagedFile&) = default;
  StagedFile& operator=(StagedFile&&) = default;

  Size size;
  std::string shaHash;
  std::string blakeHash;
  std::string name;
  StagedDirectoryID parent;
  StagedFileID id;
};
#endif