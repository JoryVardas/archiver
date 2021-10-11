#ifndef _ARCHIVE_MANAGER_H
#define _ARCHIVE_MANAGER_H

#include "../config/config.h"
#include "archive.h"
#include "file.h"
#include <map>

class ArchiveDatabase {
public:
  virtual std::optional<std::pair<Archive, Size>>
  getArchiveForExtension(const Extension& extension) const abstract;
  virtual Archive
  addNewArchiveForExtension(const Extension& extension) abstract;
  // virtual Archive getSingleFileArchive() const abstract;
};

class ArchiveManager {
public:
  ArchiveManager(std::shared_ptr<ArchiveDatabase>& archiveDatabase,
                 std::shared_ptr<Config>& config);
  virtual const Archive& getArchiveForFile(const RawFile& file);
  // virtual void loadArchive(const Archive& archive);
  // virtual void saveArchives();

protected:
  struct ArchiveInfo {
    Archive archive;
    Size size;
    bool canAdd;
  };
  std::map<Extension, std::vector<ArchiveInfo>> _archiveMap;
  std::shared_ptr<ArchiveDatabase> _archiveDatabase;
  std::shared_ptr<Config> _config;

private:
  bool doesFileFitInArchive(const RawFile& file,
                            const ArchiveInfo& archiveInfo) const;
  // void loadArchiveForExtension(const Extension& extension);
  Archive getSingleFileArchive() const;
};

#endif