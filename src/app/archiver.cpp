#include "archiver.hpp"
#include "common.h"
#include "compressor.hpp"
#include <algorithm>
#include <filesystem>
#include <map>
#include <ranges>
#include <vector>

Archiver::Archiver(std::shared_ptr<ArchivedDatabase>& archivedDatabase,
                   const std::filesystem::path& stageDirectoryLocation,
                   const std::filesystem::path& archiveDirectoryLocation,
                   Size singleFileArchiveSize)
  : archivedDatabase(archivedDatabase), stageLocation(stageDirectoryLocation),
    archiveLocation(archiveDirectoryLocation),
    singleFileArchiveSize(singleFileArchiveSize) {}

void Archiver::archive(const std::vector<StagedDirectory>& stagedDirectories,
                       const std::vector<StagedFile>& stagedFiles) {

  try {
    archivedDatabase->startTransaction();
    auto archiveOperationId = archivedDatabase->createArchiveOperation();
    archiveDirectories(stagedDirectories, archiveOperationId);
    archiveFiles(stagedFiles, archiveOperationId);
    archivedDatabase->commit();

    // Saving the archive parts takes a while and on failure should not undo the
    // entire archive operation.
    archivedDatabase->startTransaction();
    saveArchiveParts();
    archivedDatabase->commit();
  } catch (const std::exception& err) {
    archivedDatabase->rollback();
    throw;
  }
}

void Archiver::archiveDirectories(
  const std::vector<StagedDirectory>& stagedDirectories,
  ArchiveOperationID archiveOperation) {
  for (const auto& stagedDirectory : stagedDirectories) {
    if (archivedDirectoryMap.contains(stagedDirectory.id))
      continue;
    if (stagedDirectory.name == StagedDirectory::RootDirectoryName) {
      archivedDirectoryMap.insert(
        {stagedDirectory.id, archivedDatabase->getRootDirectory()});
      continue;
    }
    if (const auto parentArchivedDirectory =
          archivedDirectoryMap.find(stagedDirectory.parent);
        parentArchivedDirectory == archivedDirectoryMap.end()) {
      throw ArchiverException("Could not archive staged directory with ID {} "
                              "as its parent hasn't been archived",
                              stagedDirectory.id);
    } else {
      const auto addedArchivedDirectory = archivedDatabase->addDirectory(
        stagedDirectory, parentArchivedDirectory->second, archiveOperation);
      archivedDirectoryMap.insert({stagedDirectory.id, addedArchivedDirectory});
    }
  }
}

void Archiver::archiveFiles(const std::vector<StagedFile>& stagedFiles,
                            ArchiveOperationID archiveOperation) {
  for (const auto& stagedFile : stagedFiles) {
    if (const auto parentArchivedDirectory =
          archivedDirectoryMap.find(stagedFile.parent);
        parentArchivedDirectory == archivedDirectoryMap.end()) {
      throw ArchiverException("Could not archive staged file with ID {} as its "
                              "parent hasn't been archived",
                              stagedFile.id);
    } else {
      const auto archive = [&]() -> Archive {
        if (stagedFile.size >= singleFileArchiveSize)
          return {1, "<SINGLE>"};
        else
          return archivedDatabase->getArchiveForFile(stagedFile);
      }();
      const auto [archivedFileType, revisionId] = archivedDatabase->addFile(
        stagedFile, parentArchivedDirectory->second, archive, archiveOperation);

      if (archivedFileType == ArchivedFileAddedType::NewRevision) {
        const auto archiveDirectory =
          archiveLocation / FORMAT_LIB::format("{}", archive.id);
        if (!std::filesystem::exists(archiveDirectory))
          std::filesystem::create_directories(archiveDirectory);

        std::filesystem::copy_file(
          stageLocation / FORMAT_LIB::format("{}", stagedFile.id),
          archiveDirectory / FORMAT_LIB::format("{}", revisionId));
        modifiedArchives.insert(archive);
      }
    }
  }
}

void Archiver::saveArchiveParts() {
  Compressor compressor{archivedDatabase, {archiveLocation}};
  for (const auto& archive : modifiedArchives) {
    compressor.compress(archive);
  }
}