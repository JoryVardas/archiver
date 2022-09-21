#include "compressor.hpp"
#include <concepts>
#include <filesystem>
#include <functional>
#include <limits>
#include <ranges>
#include <subprocess.hpp>

Compressor::Compressor(
  std::shared_ptr<ArchivedDatabase>& archivedDatabase,
  const std::vector<std::filesystem::path>& archiveLocations)
  : archivedDatabase(archivedDatabase), archiveLocations(archiveLocations) {}

void Compressor::compress(const Archive& archive) {
  if (archive.id == 1)
    return compressSingleArchives();

  const auto archiveIndex =
    archiveLocations.at(0) / FORMAT_LIB::format("{}_index", archive.id);

  auto compressFiles = [&](const std::vector<std::string> files) {
    const auto newArchiveName =
      archiveLocations.at(0) /
      FORMAT_LIB::format("{}_{}.zpaq", archive.id,
                         archivedDatabase->getNextArchivePartNumber(archive));

    std::vector<std::string> commandList = {"zpaq", "a", newArchiveName};
    std::ranges::copy(files, std::back_inserter(commandList));
    commandList.push_back("-m5");
    commandList.push_back("-index");
    commandList.push_back(archiveIndex.native());

    subprocess::run(commandList, {.check = true,
                                  .cout = subprocess::PipeOption::cout,
                                  .cerr = subprocess::PipeOption::cerr,
                                  .cwd = archiveLocations.at(0)});

    archivedDatabase->incrementNextArchivePartNumber(archive);
  };

  // Chunk and add the files to the archive.
  std::vector<std::string> filesChunk;
  const decltype(filesChunk)::size_type chunkSize = 100;
  for (const auto& file : std::filesystem::directory_iterator(
         archiveLocations.at(0) / FORMAT_LIB::format("{}", archive.id))) {
    filesChunk.push_back(
      std::filesystem::path(FORMAT_LIB::format("{}", archive.id)) /
      file.path().filename());

    if (filesChunk.size() == chunkSize) {
      compressFiles(filesChunk);
      filesChunk.clear();
    }
  }
  if (!filesChunk.empty()) {
    compressFiles(filesChunk);
  }
}

void Compressor::decompress(ArchiveID archiveId,
                            const std::filesystem::path& destination) {
  const auto archiveName = FORMAT_LIB::format("{}.zpaq", archiveId);

  const auto archiveLocation = [&]() {
    for (const auto& location : archiveLocations) {
      if (std::filesystem::exists(location / archiveName))
        return location;
    }
    throw std::runtime_error{FORMAT_LIB::format(
      "Archive {} could not be found to be decompressed!", archiveName)};
  }();

  std::vector<std::string> commandList = {"zpaq", "x",
                                          archiveLocation / archiveName};

  subprocess::run(commandList, {.check = true,
                                .cout = subprocess::PipeOption::cout,
                                .cerr = subprocess::PipeOption::cerr,
                                .cwd = destination});
}
void Compressor::decompressSingleArchive(
  ArchivedFileRevisionID revisionId, const std::filesystem::path& destination) {
  const auto archiveName = FORMAT_LIB::format("{}_{}.zpaq", 1, revisionId);

  const auto archiveLocation = [&]() {
    for (const auto& location : archiveLocations) {
      if (std::filesystem::exists(location / archiveName))
        return location;
    }
    throw std::runtime_error{FORMAT_LIB::format(
      "Archive {} could not be found to be decompressed!", archiveName)};
  }();

  std::vector<std::string> commandList = {"zpaq", "x",
                                          archiveLocation / archiveName};

  subprocess::run(commandList, {.check = true,
                                .cout = subprocess::PipeOption::cout,
                                .cerr = subprocess::PipeOption::cerr,
                                .cwd = destination});
}

void Compressor::compressSingleArchives() {
  std::ranges::for_each(
    std::filesystem::directory_iterator(archiveLocations.at(0) / "1"),
    [&](const auto& file) {
      const auto newArchiveName =
        archiveLocations.at(0) /
        FORMAT_LIB::format("1_{}.zpaq", file.path().filename());

      std::vector<std::string> commandList = {
        "zpaq", "a", newArchiveName,
        std::filesystem::path("1") / file.path().filename(), "-m5"};

      subprocess::run(commandList, {.check = true,
                                    .cout = subprocess::PipeOption::cout,
                                    .cerr = subprocess::PipeOption::cerr,
                                    .cwd = archiveLocations.at(0)});
    });
}