#ifndef ARCHIVER_ARCHIVER_HPP
#define ARCHIVER_ARCHIVER_HPP

#include "../database/archive_database.hpp"
#include "../database/archived_directory_database.hpp"
#include "../database/archived_file_database.hpp"
#include "common.h"
#include "staged_directory.h"
#include "staged_file.hpp"
#include <set>
#include <span>

template <ArchivedFileDatabase ArchivedFileDatabase,
          ArchivedDirectoryDatabase ArchivedDirectoryDatabase,
          ArchiveDatabase ArchiveDatabase>
class Archiver {
public:
  Archiver(std::shared_ptr<ArchivedFileDatabase>& fileDatabase,
           std::shared_ptr<ArchivedDirectoryDatabase>& directoryDatabase,
           std::shared_ptr<ArchiveDatabase>& archiveDatabase,
           const std::filesystem::path& stageDirectoryLocation,
           const std::filesystem::path& archiveDirectoryLocation,
           Size singleFileArchiveSize);

  void archive(const std::vector<StagedDirectory>& stagedDirectories,
               const std::vector<StagedFile>& stagedFiles);

  Archiver() = delete;
  Archiver(const Archiver&) = delete;
  Archiver(Archiver&&) = default;
  ~Archiver() = default;

  Archiver& operator=(const Archiver&) = delete;
  Archiver& operator=(Archiver&&) = default;

private:
  std::shared_ptr<ArchivedFileDatabase> archivedFileDatabase;
  std::shared_ptr<ArchivedDirectoryDatabase> archivedDirectoryDatabase;
  std::shared_ptr<ArchiveDatabase> archiveDatabase;
  std::filesystem::path stageLocation;
  std::filesystem::path archiveLocation;
  Size singleFileArchiveSize;
  std::set<Archive> modifiedArchives;

  std::map<StagedDirectoryID, ArchivedDirectory> archivedDirectoryMap = {
    {StagedDirectory::RootDirectoryID, ArchivedDirectory::getRootDirectory()}};

  using path = std::filesystem::path;

  void
  archiveDirectories(const std::vector<StagedDirectory>& stagedDirectories);
  void archiveFiles(const std::vector<StagedFile>& stagedFiles);
  void saveArchiveParts();
};

_make_exception_(ArchiverException);

#include "archiver.tpp"

#endif
