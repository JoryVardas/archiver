#ifndef _DIRECTORY_MANAGER_H
#define _DIRECTORY_MANAGER_H

#include "../config/config.h"
#include "../database/directory_database.hpp"
#include "archive.h"
#include "common.h"
#include "file.h"
#include <string>

class DirectoryManager {
  public:
    DirectoryManager(std::shared_ptr<DirectoryDatabase>& directoryDatabase,
                     std::shared_ptr<Config>& config);

    virtual std::optional<ArchivedDirectory>
    getDirectory(const RawDirectory& directory);
    virtual ArchivedDirectory addDirectoryIfNew(const RawDirectory& directory);

  protected:
    std::shared_ptr<DirectoryDatabase> _directoryDatabase;
    std::shared_ptr<Config> _config;
};

#endif