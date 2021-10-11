#ifndef _FILE_MANAGER_H
#define _FILE_MANAGER_H

#include "../config/config.h"
#include "../database/file_database.hpp"
#include "archive.h"
#include "common.h"
#include "file.h"
#include <string>

class FileManager {
public:
  FileManager(std::shared_ptr<FileDatabase>& fileDatabase,
              std::shared_ptr<Config>& config);

  virtual std::optional<FileRevision>
  getMostRecentRevision(const ArchivedFile& file) const;
  virtual void copyFileToArchive(const RawFile& file,
                                 const FileRevision& revision) const;
  virtual void copyFileFromArchive(const ArchivedFile& file,
                                   const FileRevision& revision,
                                   const RawDirectory& outputDirection) const;
  virtual std::optional<FileRevision>
  getFileRevisionMatchingInfo(const FileInfo& fileInfo) const;
  virtual ArchivedFile addFileIfNew(const RawFile& file,
                                    const ArchivedDirectory& parentDirectory);
  virtual void addDuplicateFileRevision(const ArchivedFile& file,
                                        const FileRevision& duplicateRevision);
  virtual FileRevision addNewFileRevision(const ArchivedFile& file,
                                          const FileInfo& fileInfo,
                                          const Archive& archive);

protected:
  std::shared_ptr<FileDatabase> _fileDatabase;
  std::shared_ptr<Config> _config;
};

#endif