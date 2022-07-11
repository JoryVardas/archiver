#include "dearchiver.hpp"
#include "common.h"
#include "compressor.hpp"
#include "raw_file.hpp"
#include "util/string_helpers.hpp"
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
  // Find the contianing archived directory.
  ArchivedDirectory parentDirectory = archivedDatabase->getRootDirectory();
  for (const auto& directoryName : pathToDearchive.parent_path()) {
    // Exclude the root directory
    if (directoryName == ArchivedDirectory::RootDirectoryName)
      continue;

    const auto children =
      archivedDatabase->listChildDirectories(parentDirectory);
    auto child = std::ranges::find(children, directoryName.string(),
                                   &ArchivedDirectory::name);
    if (child == std::end(children)) {
      throw DearchiverException(
        "Attempt to dearchive a path which was never archived.");
    }
    parentDirectory = *child;
  }

  Compressor compressor{archivedDatabase, archiveLocation};

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
    const auto revisionIterator = getArchivedFileRevisionIterator(file);

    // If no valid revision is found skip the file.
    if (revisionIterator == std::ranges::end(file.revisions))
      return;

    const auto revision = *revisionIterator;

    if (revision.containingArchiveId == 1) {
      // The file was archived into a single file archive.
      compressor.decompressSingleArchive(revision.id, archiveTempLocation);
      std::filesystem::copy_file(archiveTempLocation /
                                   FORMAT_LIB::format("1/{}", revision.id),
                                 containingDirectory / file.name);
      return;
    }
    if (!hasArchiveBeenDecompressed(revision.containingArchiveId)) {
      mergeArchiveParts(revision.containingArchiveId);
      compressor.decompress(revision.containingArchiveId, archiveTempLocation);
      decompressedArchives.push_back(revision.containingArchiveId);
    }

    std::filesystem::copy_file(
      archiveTempLocation /
        FORMAT_LIB::format("{}/{}", revision.containingArchiveId, revision.id),
      containingDirectory / file.name);
  };

  auto dearchiveDirectory =
    [&](const ArchivedDirectory& directory,
        const std::filesystem::path& containingDirectory,
        auto&& recurse) -> void {
    auto directoryPath = [&]() {
      if (directory.name == ArchivedDirectory::RootDirectoryName)
        return containingDirectory;
      else
        return containingDirectory / directory.name;
    }();
    std::filesystem::create_directory(directoryPath);

    const auto childDirectories =
      archivedDatabase->listChildDirectories(directory);
    const auto childFiles = archivedDatabase->listChildFiles(directory);

    for (const auto& file : childFiles) {
      dearchiveFile(file, directoryPath);
    }
    for (const auto& dir : childDirectories) {
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
  if (auto directory = std::ranges::find(siblingDirectories,
                                         pathToDearchive.filename().string(),
                                         &ArchivedDirectory::name);
      directory != std::end(siblingDirectories)) {
    dearchiveDirectory(*directory, dearchiveLocation, dearchiveDirectory);
  } else if (auto file = std::ranges::find(siblingFiles,
                                           pathToDearchive.filename().string(),
                                           &ArchivedFile::name);
             file != std::end(siblingFiles)) {
    dearchiveFile(*file, dearchiveLocation);
  } else if (pathToDearchive.generic_string() ==
             ArchivedDirectory::RootDirectoryName) {
    dearchiveDirectory(archivedDatabase->getRootDirectory(), dearchiveLocation,
                       dearchiveDirectory);
  } else {
    throw DearchiverException(
      "Attempt to dearchive a path that was never archived.");
  }
}

bool Dearchiver::hasArchiveBeenDecompressed(ArchiveID archiveId) const {
  return std::ranges::find(decompressedArchives, archiveId) !=
         std::ranges::end(decompressedArchives);
}
void Dearchiver::mergeArchiveParts(ArchiveID archiveId) {
  namespace fs = std::filesystem;

  const auto outputPath =
    archiveLocation / FORMAT_LIB::format("{}.zpaq", archiveId);
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