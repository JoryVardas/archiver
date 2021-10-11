#include "archiver.h"

Archiver::Archiver(std::shared_ptr<FileManager>& fileManager,
                   std::shared_ptr<ArchiveManager>& archiveManager,
                   std::shared_ptr<DirectoryManager>& directoryManager,
                   std::pair<std::shared_ptr<FileReadBuffer>,
                             std::shared_ptr<FileReadBuffer>>& readBuffers)
  : _fileManager(fileManager), _archiveManager(archiveManager),
    _directoryManager(directoryManager), _readBuffers(readBuffers){};

void Archiver::archive(const std::filesystem::path& path,
                       bool continueOnError) {
  if (!pathExists(path))
    throw PathDoesntExistError(fmt::format(
      "Attempt to archive the path \"{}\" which doesn't exist.", path));
  if (isDirectory(path))
    for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
      if (entry.is_directory()) {
        archiveDirectory(RawDirectory(path));
      } else if (entry.is_regular_file()) {
        RawFile rawFile{path, *_readBuffers.first};
        archiveFile(rawFile);
      } else {
        if (continueOnError)
          _log_ << fmt::format("Could not archive the path \"{}\" as "
                               "it is not a file or directory.",
                               entry.path())
                << std::endl;
        else
          throw PathDoesntExistError(
            fmt::format("Could not archive the path \"{}\" as "
                        "it is not a file or directory.",
                        entry.path()));
      }
    }
  else if (isFile(path)) {
    RawFile rawFile{path, *_readBuffers.first};
    archiveFile(rawFile);
  } else
    throw PathDoesntExistError(
      fmt::format("Attempt to archive the path \"{}\" but it "
                  "isn't a file or directory.",
                  path));
};

void Archiver::archive(const std::vector<std::filesystem::path>& paths,
                       bool continueOnError) {
  for (const auto& path : paths) {
    try {
      archive(path, continueOnError);
    } catch (const PathDoesntExistError& err) {
      if (continueOnError)
        _log_ << fmt::format("One of the supplied paths could not be "
                             "archived: {}",
                             err.what())
              << std::endl;
      else
        throw;
    }
  }
};

void Archiver::archiveDirectory(const RawDirectory& directory) {
  ArchivedDirectory archivedDirectory =
    _directoryManager->addDirectoryIfNew(directory);
}

void Archiver::archiveFile(RawFile& file) {
  FileInfo fileInfo{file.getHashes(), file.size()};
  std::optional<FileRevision> revision =
    _fileManager->getFileRevisionMatchingInfo(fileInfo);

  ArchivedDirectory archivedDirectory =
    _directoryManager->addDirectoryIfNew(file.parent());
  ArchivedFile archivedFile =
    _fileManager->addFileIfNew(file, archivedDirectory);

  if (revision) {
    _fileManager->addDuplicateFileRevision(archivedFile, *revision);
  } else {
    Archive archive = _archiveManager->getArchiveForFile(file);
    FileRevision newRevision =
      _fileManager->addNewFileRevision(archivedFile, fileInfo, archive);
    _fileManager->copyFileToArchive(file, newRevision);
  }
};