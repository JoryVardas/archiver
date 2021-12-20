#ifndef _STAGED_DIRECTORY_H
#define _STAGED_DIRECTORY_H

#include "common.h"
#include "directory.h"

using StagedDirectoryID = DirectoryID;

struct StagedDirectory {
  StagedDirectory(const StagedDirectoryID id, const std::string& name,
                  const std::optional<ID>& parentID);

  StagedDirectoryID id() const;
  std::string_view name() const;
  std::optional<DirectoryID> parent() const;

  // TODO
  // These should probably be abstracted into a base class since they are also
  // part of the archived directory class, and are the same in both bool
  bool isRoot() const;
  static StagedDirectory getRootDirectory();

private:
  StagedDirectoryID _id;
  std::optional<StagedDirectoryID> _parent;
  std::string _name;

public:
  static constexpr StagedDirectoryID RootDirectoryID = 1;
  static constexpr std::string_view RootDirectoryName = "/";
};

#endif