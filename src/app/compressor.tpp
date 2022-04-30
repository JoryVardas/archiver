#include "compressor.hpp"
#include <concepts>
#include <filesystem>
#include <functional>
#include <limits>
#include <ranges>
#include <subprocess.hpp>

template <ArchivedDatabase ArchivedDatabase>
Compressor<ArchivedDatabase>::Compressor(
  std::shared_ptr<ArchivedDatabase>& archivedDatabase,
  const std::filesystem::path& archiveLocation)
  : archivedDatabase(archivedDatabase), archiveLocation(archiveLocation) {}

template <ArchivedDatabase ArchivedDatabase>
void Compressor<ArchivedDatabase>::compress(const Archive& archive) {
  if (archive.id == 1)
    return compressSingleArchives();

  const auto archiveIndex =
    archiveLocation / FORMAT_LIB::format("{}_index", archive.id);

  auto compressFiles = [&](const std::vector<std::string> files) {
    const auto newArchiveName =
      archiveLocation /
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
                                  .cwd = archiveLocation});

    archivedDatabase->incrementNextArchivePartNumber(archive);
  };

  // Chunk and add the files to the archive.
  std::vector<std::string> filesChunk;
  const decltype(filesChunk)::size_type chunkSize = 10;
  for (const auto& file : std::filesystem::directory_iterator(
         archiveLocation / FORMAT_LIB::format("{}", archive.id))) {
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

template <ArchivedDatabase ArchivedDatabase>
void Compressor<ArchivedDatabase>::decompress(
  ArchiveID archiveId, const std::filesystem::path& destination) {
  const auto archiveName = FORMAT_LIB::format("{}.zpaq", archiveId);

  std::vector<std::string> commandList = {"zpaq", "x",
                                          archiveLocation / archiveName};

  subprocess::run(commandList, {.check = true,
                                .cout = subprocess::PipeOption::cout,
                                .cerr = subprocess::PipeOption::cerr,
                                .cwd = destination});
}
template <ArchivedDatabase ArchivedDatabase>
void Compressor<ArchivedDatabase>::decompressSingleArchive(
  ArchivedFileRevisionID revisionId, const std::filesystem::path& destination) {
  const auto archiveName = FORMAT_LIB::format("{}_{}.zpaq", 1, revisionId);

  std::vector<std::string> commandList = {"zpaq", "x",
                                          archiveLocation / archiveName};

  subprocess::run(commandList, {.check = true,
                                .cout = subprocess::PipeOption::cout,
                                .cerr = subprocess::PipeOption::cerr,
                                .cwd = destination});
}

template <ArchivedDatabase ArchivedDatabase>
void Compressor<ArchivedDatabase>::compressSingleArchives() {
  std::ranges::for_each(
    std::filesystem::directory_iterator(archiveLocation / "1"),
    [&](const auto& file) {
      const auto newArchiveName =
        archiveLocation /
        FORMAT_LIB::format("1_{}.zpaq", file.path().filename());

      std::vector<std::string> commandList = {
        "zpaq", "a", newArchiveName,
        std::filesystem::path("1") / file.path().filename(), "-m5"};

      subprocess::run(commandList, {.check = true,
                                    .cout = subprocess::PipeOption::cout,
                                    .cerr = subprocess::PipeOption::cerr,
                                    .cwd = archiveLocation});
    });
}