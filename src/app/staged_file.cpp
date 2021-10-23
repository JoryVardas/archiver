#include "staged_file.hpp"

StagedFile::StagedFile(StagedFileID id, StagedDirectoryID parent,
                       const std::string& stagedName, Size size,
                       const std::string& shaHash, const std::string& blakeHash)
  : size(size), shaHash(shaHash), blakeHash(blakeHash), name(stagedName),
    parent(parent), id(id){};