#ifndef ARCHIVER_DIRECTORY_DATABASE_HPP
#define ARCHIVER_DIRECTORY_DATABASE_HPP

#include "../app/directory.h"
#include <string_view>

class DirectoryDatabase {
  public:
    virtual std::optional<ArchivedDirectory>
    getDirectory(DirectoryID id) const abstract;
    virtual std::optional<ArchivedDirectory>
    getDirectory(DirectoryID parent,
                 const std::string_view name) const abstract;
    virtual ArchivedDirectory
    addNewDirectory(DirectoryID parent, const std::string_view name) abstract;
};

#endif
