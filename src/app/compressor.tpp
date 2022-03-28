#include "compressor.hpp"
#include <concepts>
#include <filesystem>
#include <functional>
#include <limits>
#include <ranges>

template <ArchivedDatabase ArchivedDatabase>
Compressor<ArchivedDatabase>::Compressor(
  std::shared_ptr<ArchivedDatabase>& archivedDatabase,
  const std::filesystem::path& archiveLocation)
  : archivedDatabase(archivedDatabase), archiveLocation(archiveLocation) {}

template <ArchivedDatabase ArchivedDatabase>
void Compressor<ArchivedDatabase>::compress(const Archive& archive) {
  if (archive.id == 1)
    return compressSingleArchives();
  const auto newArchiveName =
    archiveLocation /
    FORMAT_LIB::format("{}_{}.zpaq", archive.id,
                       archivedDatabase->getNextArchivePartNumber(archive));
  const auto archiveIndex =
    archiveLocation / FORMAT_LIB::format("{}_index", archive.id);
  const std::string command =
    FORMAT_LIB::format("zpaq a {} {}/{} -m5 -index {}", newArchiveName,
                       archiveLocation, archive.id, archiveIndex);

  std::flush(std::cout);
  if (const auto exitCode = system(command.c_str()); exitCode != 0) {
    throw CompressorException(FORMAT_LIB::format(
      "There was an error while compressing archive with id {}, "
      "zpaq returned the error {}",
      archive.id, exitCode));
  }
  std::flush(std::cout);

  archivedDatabase->incrementNextArchivePartNumber(archive);
}

template <ArchivedDatabase ArchivedDatabase>
void Compressor<ArchivedDatabase>::compressSingleArchives() {
  std::ranges::for_each(
    std::filesystem::directory_iterator(archiveLocation / "1"),
    [&](const auto& file) {
      const auto newArchiveName =
        archiveLocation /
        FORMAT_LIB::format("1_{}.zpaq", file.path().filename());
      const std::string command =
        FORMAT_LIB::format("zpaq a {} {} -m5", newArchiveName, file.path());

      std::flush(std::cout);
      if (const auto exitCode = system(command.c_str()); exitCode != 0) {
        throw CompressorException(FORMAT_LIB::format(
          "There was an error while compressing the single file "
          "archive with name {}, "
          "zpaq returned the error {}",
          file.path().filename(), exitCode));
      }
      std::flush(std::cout);
    });
}