#ifndef _ARCHIVER_H
#define _ARCHIVER_H

#include "archive_manager.h"
#include "directory_manager.h"
#include "file_manager.h"
#include <filesystem>
#include <memory>
#include <vector>

_make_exception_(PathDoesntExistError);

class Archiver {
public:
  Archiver(std::shared_ptr<FileManager>& fileManager,
           std::shared_ptr<ArchiveManager>& archiveManager,
           std::shared_ptr<DirectoryManager>& directoryManager,
           std::pair<std::shared_ptr<FileReadBuffer>,
                     std::shared_ptr<FileReadBuffer>>& readBuffers);

  void archive(const std::filesystem::path& path, bool continueOnError = true);
  void archive(const std::vector<std::filesystem::path>& paths,
               bool continueOnError = true);

private:
  std::shared_ptr<FileManager>& _fileManager;
  std::shared_ptr<ArchiveManager>& _archiveManager;
  std::shared_ptr<DirectoryManager>& _directoryManager;
  std::pair<std::shared_ptr<FileReadBuffer>, std::shared_ptr<FileReadBuffer>>
    _readBuffers;

  void archiveDirectory(const RawDirectory& directory);
  void archiveFile(RawFile& file);
};

#endif