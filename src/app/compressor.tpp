#include "compressor.hpp"
#include "util/index_iterator.hpp"
#include <concepts>
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
  const auto newArchiveName =
    archiveLocation /
    fmt::format("{}_{}.zpaq", archive.id,
                archiveDatabase->getNextArchivePartNumber(archive));
  const auto archiveIndex =
    archiveLocation / fmt::format("{}_index", archive.id);
  const std::string command = fmt::format(
    "zpaq a {} ./{} -m5 -index {}", newArchiveName, archive.id, archiveIndex);

  std::flush(std::cout);
  if (const auto exitCode = system(command.c_str()); exitCode != 0) {
    throw CompressorException(
      fmt::format("There was an error while compressing archive with id {}, "
                  "zpaq returned the error {}",
                  archive.id, exitCode));
  }
  std::flush(std::cout);

  archiveDatabase->incrementNextArchivePartNumber(archive);
}