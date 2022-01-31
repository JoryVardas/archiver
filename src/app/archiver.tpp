#include "common.h"
#include "compressor.hpp"
#include "util/string_helpers.hpp"
#include <algorithm>
#include <filesystem>
#include <map>
#include <ranges>
#include <vector>

template <ArchivedFileDatabase ArchivedFileDatabase,
          ArchivedDirectoryDatabase ArchivedDirectoryDatabase,
          ArchiveDatabase ArchiveDatabase>
Archiver<ArchivedFileDatabase, ArchivedDirectoryDatabase, ArchiveDatabase>::
  Archiver(std::shared_ptr<ArchivedFileDatabase>& fileDatabase,
           std::shared_ptr<ArchivedDirectoryDatabase>& directoryDatabase,
           std::shared_ptr<ArchiveDatabase>& archiveDatabase,
           const std::filesystem::path& stageDirectoryLocation,
           const std::filesystem::path& archiveDirectoryLocation,
           Size singleFileArchiveSize)
  : archivedFileDatabase(fileDatabase),
    archivedDirectoryDatabase(directoryDatabase),
    archiveDatabase(archiveDatabase), stageLocation(stageDirectoryLocation),
    archiveLocation(archiveDirectoryLocation),
    singleFileArchiveSize(singleFileArchiveSize) {}

template <ArchivedFileDatabase ArchivedFileDatabase,
          ArchivedDirectoryDatabase ArchivedDirectoryDatabase,
          ArchiveDatabase ArchiveDatabase>
void Archiver<ArchivedFileDatabase, ArchivedDirectoryDatabase,
              ArchiveDatabase>::archive(const std::vector<StagedDirectory>&
                                          stagedDirectories,
                                        const std::vector<StagedFile>&
                                          stagedFiles) {
  const auto startTransactions = [&]() {
    archivedDirectoryDatabase->startTransaction();
    archivedFileDatabase->startTransaction();
    archiveDatabase->startTransaction();
  };
  const auto commitTransactions = [&]() {
    archivedDirectoryDatabase->commit();
    archivedFileDatabase->commit();
    archiveDatabase->commit();
  };
  const auto rollbackTransactions = [&]() {
    archivedDirectoryDatabase->rollback();
    archivedFileDatabase->rollback();
    archiveDatabase->rollback();
  };

  try {
    startTransactions();
    archiveDirectories(stagedDirectories);
    archiveFiles(stagedFiles);
    commitTransactions();

    // Saving the archive parts takes a while and on failure should not undo the
    // entire archive operation.
    startTransactions();
    saveArchiveParts();
    commitTransactions();
  } catch (const std::exception& err) {
    rollbackTransactions();
    throw;
  }
}

template <ArchivedFileDatabase ArchivedFileDatabase,
          ArchivedDirectoryDatabase ArchivedDirectoryDatabase,
          ArchiveDatabase ArchiveDatabase>
void Archiver<ArchivedFileDatabase, ArchivedDirectoryDatabase,
              ArchiveDatabase>::
  archiveDirectories(const std::vector<StagedDirectory>& stagedDirectories) {
  for (const auto& stagedDirectory : stagedDirectories) {
    if (archivedDirectoryMap.contains(stagedDirectory.id()))
      continue;
    if (!stagedDirectory.parent()) {
      throw ArchiverException(fmt::format(
        "Could not archive staged directory with ID {} since it is not "
        "the root directory and doesn't have a parent.",
        stagedDirectory.id()));
    }
    if (const auto parentArchivedDirectory =
          archivedDirectoryMap.find(stagedDirectory.parent().value());
        parentArchivedDirectory == archivedDirectoryMap.end()) {
      throw ArchiverException(
        fmt::format("Could not archive staged directory with ID {} as its "
                    "parent hasn't been archived",
                    stagedDirectory.id()));
    } else {
      const auto addedArchivedDirectory =
        archivedDirectoryDatabase->addDirectory(
          stagedDirectory, parentArchivedDirectory->second);
      archivedDirectoryMap.insert(
        {stagedDirectory.id(), addedArchivedDirectory});
    }
  }
}

template <ArchivedFileDatabase ArchivedFileDatabase,
          ArchivedDirectoryDatabase ArchivedDirectoryDatabase,
          ArchiveDatabase ArchiveDatabase>
void Archiver<ArchivedFileDatabase, ArchivedDirectoryDatabase,
              ArchiveDatabase>::archiveFiles(const std::vector<StagedFile>&
                                               stagedFiles) {
  for (const auto& stagedFile : stagedFiles) {
    if (const auto parentArchivedDirectory =
          archivedDirectoryMap.find(stagedFile.parent);
        parentArchivedDirectory == archivedDirectoryMap.end()) {
      throw ArchiverException(
        fmt::format("Could not archive staged file with ID {} as its "
                    "parent hasn't been archived",
                    stagedFile.id));
    } else {
      const auto archive = [&]() -> Archive {
        if (stagedFile.size >= singleFileArchiveSize)
          return {1, "<SINGLE>"};
        else
          return archiveDatabase->getArchiveForFile(stagedFile);
      }();
      const auto archivedFileType = archivedFileDatabase->addFile(
        stagedFile, parentArchivedDirectory->second, archive);

      if (archivedFileType == ArchivedFileAddedType::NewRevision) {
        const auto archiveDirectory =
          archiveLocation / fmt::format("{}", archive.id);
        if (!std::filesystem::exists(archiveDirectory))
          std::filesystem::create_directories(archiveDirectory);

        std::filesystem::copy_file(stageLocation / stagedFile.hash,
                                   archiveDirectory / stagedFile.hash);
        modifiedArchives.insert(archive);
      }
    }
  }
}

template <ArchivedFileDatabase ArchivedFileDatabase,
          ArchivedDirectoryDatabase ArchivedDirectoryDatabase,
          ArchiveDatabase ArchiveDatabase>
void Archiver<ArchivedFileDatabase, ArchivedDirectoryDatabase,
              ArchiveDatabase>::saveArchiveParts() {
  Compressor<ArchiveDatabase> compressor{archiveDatabase, archiveLocation};
  for (const auto& archive : modifiedArchives) {
    compressor.compress(archive);
  }
}