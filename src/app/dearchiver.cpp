#include "dearchiver.hpp"
#include "common.h"
#include "compressor.hpp"
#include "raw_file.hpp"
#include "util/string_helpers.hpp"
#include <concepts>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <ranges>
#include <string>
#include <vector>

Dearchiver::Dearchiver(
  std::shared_ptr<ArchivedDatabase>& archivedDatabase,
  const std::filesystem::path& archiveDirectoryLocation,
  const std::filesystem::path& archiveTempDirectoryLocation,
  std::span<char> fileReadBuffer)
  : archivedDatabase(archivedDatabase),
    archiveLocation(archiveDirectoryLocation),
    archiveTempLocation(archiveTempDirectoryLocation),
    readBuffer(fileReadBuffer) {
  if (readBuffer.size() >
      static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
    throw std::logic_error(
      "Could not construct Dearchiver as the given buffer is larger than the "
      "maximum input read stream input size.");
}

void Dearchiver::dearchive(
  const std::filesystem::path& pathToDearchive,
  const std::filesystem::path& dearchiveLocation,
  const std::optional<ArchiveOperationID> archiveOperation) {
  spdlog::info("Dearchiving {} to {}", pathToDearchive, dearchiveLocation);
  if (archiveOperation)
    spdlog::info("Archive operation specified: {}", archiveOperation.value());

  // Find the contianing archived directory.
  ArchivedDirectory parentDirectory = archivedDatabase->getRootDirectory();
  for (const auto& directoryName : pathToDearchive.parent_path()) {
    // Exclude the root directory
    if (directoryName == ArchivedDirectory::RootDirectoryName)
      continue;

    const auto children =
      archivedDatabase->listChildDirectories(parentDirectory);
    auto child =
      std::ranges::find_if(children, [&](const ArchivedDirectory& dir) {
        if (archiveOperation) {
          return dir.name == directoryName.string() &&
                 dir.containingArchiveOperation == archiveOperation.value();
        } else
          return dir.name == directoryName.string();
      });
    if (child == std::end(children)) {
      throw DearchiverException(
        "Attempt to dearchive a path which was never archived.");
    }
    parentDirectory = *child;
  }

  spdlog::info("The parent directory was found to be {} with id {}",
               parentDirectory.name, parentDirectory.id);

  Compressor compressor{archivedDatabase,
                        {archiveLocation, archiveTempLocation}};

  spdlog::info("Compressor created");

  auto getArchivedFileRevisionIterator = [&](const ArchivedFile& file) {
    if (archiveOperation.has_value()) {
      return std::ranges::find_if(
        file.revisions,
        std::bind_front(std::ranges::equal_to(), archiveOperation.value()),
        &ArchivedFileRevision::containingOperation);
    } else
      return --std::ranges::end(file.revisions);
  };
  auto dearchiveFile = [&](const ArchivedFile& file,
                           const std::filesystem::path& containingDirectory) {
    spdlog::info("Processing file {} with id {}", file.name, file.id);
    const auto revisionIterator = getArchivedFileRevisionIterator(file);

    // If no valid revision is found skip the file.
    if (revisionIterator == std::ranges::end(file.revisions)) {
      spdlog::info("File did not have a revision which was part of the "
                   "specified archive operation");
      return;
    }

    spdlog::info("Revision was found");
    const auto revision = *revisionIterator;

    if (revision.containingArchiveId == 1) {
      // The file was archived into a single file archive.
      if (!std::filesystem::exists(archiveTempLocation /
                                   FORMAT_LIB::format("1/{}", revision.id)))
        compressor.decompressSingleArchive(revision.id, archiveTempLocation);
      std::filesystem::copy_file(archiveTempLocation /
                                   FORMAT_LIB::format("1/{}", revision.id),
                                 containingDirectory / file.name);
      return;
    }
    if (!hasArchiveBeenDecompressed(revision.containingArchiveId) &&
        !std::filesystem::exists(
          archiveTempLocation / FORMAT_LIB::format("{}/{}",
                                                   revision.containingArchiveId,
                                                   revision.id))) {
      spdlog::info("Revision has not yet been decompressed, so decompress it");
      mergeArchiveParts(revision.containingArchiveId);
      compressor.decompress(revision.containingArchiveId, archiveTempLocation);
      decompressedArchives.push_back(revision.containingArchiveId);
    }

    spdlog::info("Copying file revision from \"{}/{}\" to \"{}/{}\"",
                 revision.containingArchiveId, revision.id, containingDirectory,
                 file.name);
    std::filesystem::copy_file(
      archiveTempLocation /
        FORMAT_LIB::format("{}/{}", revision.containingArchiveId, revision.id),
      containingDirectory / file.name);
  };

  auto dearchiveDirectory =
    [&](const ArchivedDirectory& directory,
        const std::filesystem::path& containingDirectory,
        auto&& recurse) -> void {
    spdlog::info("Processing directory {} with id {}", directory.name,
                 directory.id);
    auto directoryPath = [&]() {
      if (directory.name == ArchivedDirectory::RootDirectoryName)
        return containingDirectory;
      else
        return containingDirectory / directory.name;
    }();
    std::filesystem::create_directory(directoryPath);

    auto childDirectories = archivedDatabase->listChildDirectories(directory);
    const auto childFiles = archivedDatabase->listChildFiles(directory);

    if (!archiveOperation.has_value()) {
      std::ranges::sort(childDirectories, {}, &ArchivedDirectory::id);
      auto [toEraseStart, toEraseEnd] =
        std::ranges::unique(childDirectories, {}, &ArchivedDirectory::id);
      childDirectories.erase(toEraseStart, toEraseEnd);
    }

    auto rootDirectoryId = archivedDatabase->getRootDirectory().id;

    for (const auto& file : childFiles) {
      spdlog::info("Directory has files");
      dearchiveFile(file, directoryPath);
    }
    for (const auto& dir : childDirectories) {
      spdlog::info("Directory has directories");
      if (dir.id == rootDirectoryId)
        continue;
      if (archiveOperation.has_value())
        spdlog::info("Child directory has archive operation {}",
                     dir.containingArchiveOperation);
      if (!archiveOperation.has_value() ||
          dir.containingArchiveOperation == archiveOperation.value()) {
        recurse(dir, directoryPath, recurse);
      }
    }
  };

  // Now that we have the parent directory we can check if the path being
  // dearchived is a file or directory.
  auto siblingDirectories =
    archivedDatabase->listChildDirectories(parentDirectory);
  auto siblingFiles = archivedDatabase->listChildFiles(parentDirectory);
  if (auto directory = std::ranges::find_if(
        siblingDirectories,
        [&](const auto& dir) {
          if (archiveOperation) {
            return dir.name == pathToDearchive.filename().string() &&
                   dir.containingArchiveOperation == archiveOperation.value();
          } else
            return dir.name == pathToDearchive.filename().string();
        });
      directory != std::end(siblingDirectories)) {
    spdlog::info("Path to dearchive is a directory");
    dearchiveDirectory(*directory, dearchiveLocation, dearchiveDirectory);
  } else if (auto file = std::ranges::find(siblingFiles,
                                           pathToDearchive.filename().string(),
                                           &ArchivedFile::name);
             file != std::end(siblingFiles)) {
    spdlog::info("Path to dearchive is a file");
    dearchiveFile(*file, dearchiveLocation);
  } else if (pathToDearchive.generic_string() ==
             ArchivedDirectory::RootDirectoryName) {
    spdlog::info("Path to dearchive is the root directory");
    dearchiveDirectory(archivedDatabase->getRootDirectory(), dearchiveLocation,
                       dearchiveDirectory);
  } else {
    throw DearchiverException(
      "Attempt to dearchive a path that was never archived.");
  }
}

void Dearchiver::check() {
  spdlog::info("Begining check");

  Compressor compressor{archivedDatabase,
                        {archiveLocation, archiveTempLocation}};

  spdlog::info("Compressor created");

  std::vector<ArchivedFileRevisionID> incorrectRevisions;

  auto checkFile = [&](const ArchivedFile& file) {
    spdlog::info("Checking file {} with id {}", file.name, file.id);

    if (file.revisions.size() == 0) {
      spdlog::error("File with id {} did not have any revisions", file.id);
    }
    for (const auto& revision : file.revisions) {
      spdlog::info("Checking revision with id {}", revision.id);
      if (revision.isDuplicate) {
        spdlog::info("Revision is a duplicate, skipping check");
        continue;
      }

      if (revision.containingArchiveId == 1) {
        // The file was archived into a single file archive.
        if (!std::filesystem::exists(archiveTempLocation /
                                     FORMAT_LIB::format("1/{}", revision.id)))
          compressor.decompressSingleArchive(revision.id, archiveTempLocation);
      }
      if (!std::filesystem::exists(
            archiveTempLocation /
            FORMAT_LIB::format("{}/{}", revision.containingArchiveId,
                               revision.id))) {
        spdlog::info(
          "Revision has not yet been decompressed, so decompress it");
        mergeArchiveParts(revision.containingArchiveId);
        compressor.decompress(revision.containingArchiveId,
                              archiveTempLocation);
      }

      if (!std::filesystem::exists(
            archiveTempLocation /
            FORMAT_LIB::format("{}/{}", revision.containingArchiveId,
                               revision.id))) {
        spdlog::error("Revision with id {} could not be decompressed",
                      revision.id);
        incorrectRevisions.push_back(revision.id);
      } else {
        RawFile rawFile(archiveTempLocation /
                          FORMAT_LIB::format(
                            "{}/{}", revision.containingArchiveId, revision.id),
                        readBuffer);
        if (rawFile.size != revision.size || rawFile.hash != revision.hash) {
          spdlog::warn("Revision with id {} is archived incorrectly",
                       revision.id);
          incorrectRevisions.push_back(revision.id);
        }
      }
    }
  };

  auto rootDirectoryId = archivedDatabase->getRootDirectory().id;

  auto checkDirectory = [&](const ArchivedDirectory& directory,
                            auto&& recurse) -> void {
    spdlog::info("Checking directory {} with id {}", directory.name,
                 directory.id);

    auto childDirectories = archivedDatabase->listChildDirectories(directory);
    const auto childFiles = archivedDatabase->listChildFiles(directory);

    spdlog::info("Sorting child directories");
    std::ranges::sort(childDirectories, {}, &ArchivedDirectory::id);

    spdlog::info("removing duplicate children");
    auto [toEraseStart, toEraseEnd] =
      std::ranges::unique(childDirectories, {}, &ArchivedDirectory::id);
    childDirectories.erase(toEraseStart, toEraseEnd);

    spdlog::info("Listing all child directories");
    for (const auto& dir : childDirectories) {
      spdlog::info("Child directory with id {}", dir.id);
    }

    if (childFiles.size() > 0) {
      spdlog::info("Directory has files");
    }
    for (const auto& file : childFiles) {
      checkFile(file);
    }

    if (childDirectories.size() > 0) {
      spdlog::info("Directory has directories");
    }
    for (const auto& dir : childDirectories) {
      spdlog::info("Child directory with id {}", dir.id);

      if (dir.id != rootDirectoryId) {
        recurse(dir, recurse);
      }
    }
  };

  checkDirectory(archivedDatabase->getRootDirectory(), checkDirectory);

  if (incorrectRevisions.size() > 0) {
    spdlog::warn("The following revisions are not archived correctly.");
    for (const auto rev : incorrectRevisions) {
      spdlog::warn("Revision {}", rev);
    }
  }
}

bool Dearchiver::hasArchiveBeenDecompressed(ArchiveID archiveId) const {
  return std::ranges::find(decompressedArchives, archiveId) !=
         std::ranges::end(decompressedArchives);
}
void Dearchiver::mergeArchiveParts(ArchiveID archiveId) {
  namespace fs = std::filesystem;

  const auto outputPath =
    archiveTempLocation / FORMAT_LIB::format("{}.zpaq", archiveId);
  std::basic_ofstream<char> outputStream(outputPath, std::ios_base::binary |
                                                       std::ios_base::trunc);
  if (outputStream.bad() || !outputStream.is_open()) {
    throw FileException(FORMAT_LIB::format(
      "There was an error opening \"{}\" for writing", outputPath));
  }

  const auto archiveNameStart = FORMAT_LIB::format("{}_", archiveId);

  unsigned long long maxPartNumber = 1;

  for (const auto& directoryEntry : fs::directory_iterator(archiveLocation)) {
    if (!directoryEntry.is_regular_file())
      continue;

    const auto archivePartPath = directoryEntry.path();
    const std::string fileName = directoryEntry.path().filename().string();

    // Skip archive parts which are not
    if (!std::string_view{fileName}.starts_with(archiveNameStart))
      continue;

    const std::string_view partNumberString =
      removeSuffix(removePrefix(fileName, archiveNameStart), ".zpaq");
    try {
      const auto partNumber = std::stoull(std::string{partNumberString});
      maxPartNumber = std::max(maxPartNumber, partNumber);
    } catch (std::invalid_argument& err) {
      // Do nothing and don't count the result.
    }
  }

  for (Size partNumber = 1; partNumber <= maxPartNumber; ++partNumber) {

    const auto archivePartPath =
      archiveLocation /
      FORMAT_LIB::format("{}{}.zpaq", archiveNameStart, partNumber);

    std::basic_ifstream<char> inputStream(archivePartPath,
                                          std::ios_base::binary);

    if (inputStream.bad() || !inputStream.is_open()) {
      throw FileException(FORMAT_LIB::format(
        "There was an error opening \"{}\" for reading", archivePartPath));
    }

    while (!inputStream.eof()) {
      inputStream.read(readBuffer.data(),
                       static_cast<std::streamsize>(readBuffer.size()));

      if (inputStream.bad()) {
        throw FileException(FORMAT_LIB::format(
          "There was an error reading \"{}\"", archivePartPath));
      }

      Size read = static_cast<Size>(inputStream.gcount());

      outputStream.write(readBuffer.data(), read);
    }
  }

  outputStream.flush();
}
