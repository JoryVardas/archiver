#ifndef ARCHIVER_DIRECTORY_DATABASE_HPP
#define ARCHIVER_DIRECTORY_DATABASE_HPP

#include "../app/directory.h"
#include <string_view>

class DirectoryDatabase {
  public:
    virtual auto getDirectory(DirectoryID id) const
        -> std::optional<ArchivedDirectory> abstract;
    virtual auto getDirectory(DirectoryID parent,
                              const std::string_view name) const
        -> std::optional<ArchivedDirectory> abstract;
    virtual auto addNewDirectory(DirectoryID parent,
                                 const std::string_view name)
        -> ArchivedDirectory abstract;
};

#endif
