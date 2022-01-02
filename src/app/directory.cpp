#include "directory.h"

ArchivedDirectory::ArchivedDirectory(DirectoryID id,
                                     const std::string_view name,
                                     const std::optional<ID>& parentID)
  : _id(id), _parent(parentID), _name(name){};

DirectoryID ArchivedDirectory::id() const { return _id; };
const std::string& ArchivedDirectory::name() const { return _name; };
std::optional<DirectoryID> ArchivedDirectory::parent() const {
  return isRoot() ? std::nullopt : _parent;
};
bool ArchivedDirectory::isRoot() const { return _id == RootDirectoryID; };
ArchivedDirectory ArchivedDirectory::getRootDirectory() {
  return ArchivedDirectory(RootDirectoryID, RootDirectoryName, std::nullopt);
};