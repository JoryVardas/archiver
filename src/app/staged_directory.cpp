#include "staged_directory.h"

StagedDirectory::StagedDirectory(
  StagedDirectoryID id, const std::string& name,
  const std::optional<StagedDirectoryID>& parentID)
  : _id(id), _parent(parentID), _name(name){};

StagedDirectoryID StagedDirectory::id() const { return _id; };
std::string_view StagedDirectory::name() const { return _name; };
std::optional<StagedDirectoryID> StagedDirectory::parent() const {
  return isRoot() ? std::nullopt : _parent;
};
bool StagedDirectory::isRoot() const { return _id == RootDirectoryID; };
StagedDirectory StagedDirectory::getRootDirectory() {
  return StagedDirectory(RootDirectoryID, std::string(RootDirectoryName),
                         std::nullopt);
};