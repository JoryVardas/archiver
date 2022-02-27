#include "compressor.hpp"
#include <concepts>
#include <filesystem>
#include <functional>
#include <limits>
#include <ranges>

template <ArchiveDatabase ArchiveDatabase>
Compressor<ArchiveDatabase>::Compressor(
  std::shared_ptr<ArchiveDatabase>& archiveDatabase,
  const std::filesystem::path& archiveLocation)
  : archiveDatabase(archiveDatabase), archiveLocation(archiveLocation) {}

template <ArchiveDatabase ArchiveDatabase>
void Compressor<ArchiveDatabase>::compress(const Archive& archive) {
  if (archive.id == 1)
    return compressSingleArchives();
  const auto newArchiveName =
    archiveLocation /
    FORMAT_LIB::format("{}_{}.zpaq", archive.id,
                       archiveDatabase->getNextArchivePartNumber(archive));
  const auto archiveIndex =
    archiveLocation / FORMAT_LIB::format("{}_index", archive.id);
  const std::string command = FORMAT_LIB::format(
    "zpaq a {} ./{} -m5 -index {}", newArchiveName, archive.id, archiveIndex);

  std::flush(std::cout);
  if (const auto exitCode = system(command.c_str()); exitCode != 0) {
    throw CompressorException(FORMAT_LIB::format(
      "There was an error while compressing archive with id {}, "
      "zpaq returned the error {}",
      archive.id, exitCode));
  }
  std::flush(std::cout);

  archiveDatabase->incrementNextArchivePartNumber(archive);
}

template <ArchiveDatabase ArchiveDatabase>
void Compressor<ArchiveDatabase>::compressSingleArchives() {
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