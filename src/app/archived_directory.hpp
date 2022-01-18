#ifndef ARCHIVER_ARCHIVED_DIRECTORY_H
#define ARCHIVER_ARCHIVED_DIRECTORY_H

#include "common.h"
#include "directory.h"

using ArchivedDirectoryID = DirectoryID;

struct ArchivedDirectory {
  ArchivedDirectory(ArchivedDirectoryID id, const std::string_view name,
                    const std::optional<ID>& parentID);

  ArchivedDirectoryID id() const;
  const std::string& name() const;
  std::optional<ArchivedDirectoryID> parent() const;
  bool isRoot() const;

  static ArchivedDirectory getRootDirectory();

private:
  ArchivedDirectoryID _id;
  std::optional<ArchivedDirectoryID> _parent;
  std::string _name;

public:
  static constexpr ArchivedDirectoryID RootDirectoryID = 1;
  static constexpr std::string_view RootDirectoryName = "/";
};

#endif