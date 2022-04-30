#include "common.h"
#include "compressor.hpp"
#include "util/string_helpers.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <ranges>
#include <vector>

template <ArchivedDatabase ArchivedDatabase>
Dearchiver<ArchivedDatabase>::Dearchiver(
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

template <ArchivedDatabase ArchivedDatabase>
void Dearchiver<ArchivedDatabase>::dearchive(
  const std::filesystem::path& pathToDearchive,
  const std::filesystem::path& dearchiveLocation) {
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

  Compressor<ArchivedDatabase> compressor{archivedDatabase, archiveLocation};

  auto dearchiveFile = [&](const ArchivedFile& file,
                           const std::filesystem::path& containingDirectory) {
    auto revision = file.revisions.back();
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
    auto directoryPath = containingDirectory / directory.name;
    std::filesystem::create_directory(directoryPath);

    const auto childDirectories =
      archivedDatabase->listChildDirectories(directory);
    const auto childFiles = archivedDatabase->listChildFiles(directory);

    for (const auto& file : childFiles) {
      dearchiveFile(file, directoryPath);
    }
    for (const auto& dir : childDirectories) {
      recurse(dir, directoryPath, recurse);
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
  } else {
    throw DearchiverException(
      "Attempt to dearchive a path that was never archived.");
  }
}

template <ArchivedDatabase ArchivedDatabase>
bool Dearchiver<ArchivedDatabase>::hasArchiveBeenDecompressed(
  ArchiveID archiveId) const {
  return std::ranges::find(decompressedArchives, archiveId) !=
         std::ranges::end(decompressedArchives);
}
template <ArchivedDatabase ArchivedDatabase>
void Dearchiver<ArchivedDatabase>::mergeArchiveParts(ArchiveID archiveId) {
  namespace fs = std::filesystem;

  const auto outputPath =
    archiveLocation / FORMAT_LIB::format("{}.zpaq", archiveId);
  std::basic_ofstream<char> outputStream(outputPath, std::ios_base::binary |
                                                       std::ios_base::trunc);
  if (outputStream.bad() || !outputStream.is_open()) {
    throw FileException(FORMAT_LIB::format(
      "There was an error opening \"{}\" for writing", outputPath));
  }

  for (const auto& directoryEntry : fs::directory_iterator(archiveLocation)) {
    if (!directoryEntry.is_regular_file())
      continue;

    const auto archivePartPath = directoryEntry.path();
    const auto fileName = directoryEntry.path().filename().string();

    // Skip archive parts which are not
    if (!std::string_view{fileName}.starts_with(
          FORMAT_LIB::format("{}_", archiveId)))
      continue;

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