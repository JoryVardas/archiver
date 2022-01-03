#include "staged_file.hpp"

StagedFile::StagedFile(StagedFileID id, StagedDirectoryID parent,
                       const std::string& stagedName, Size size,
                       const std::string& hash)
  : size(size), hash(hash), name(stagedName), parent(parent), id(id){};