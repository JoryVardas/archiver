#ifndef _DIRECTORY_H
#define _DIRECTORY_H

#include "common.h"

using DirectoryID = ID;

struct ArchivedDirectory {
  ArchivedDirectory(DirectoryID id, const std::string_view name,
                    const std::optional<ID>& parentID);

  DirectoryID id() const;
  const std::string& name() const;
  std::optional<DirectoryID> parent() const;
  bool isRoot() const;

  static ArchivedDirectory getRootDirectory();

private:
  DirectoryID _id;
  std::optional<DirectoryID> _parent;
  std::string _name;
  static constexpr DirectoryID RootDirectoryID = 1;
  static constexpr std::string_view RootDirectoryName = "/";
};

#endif