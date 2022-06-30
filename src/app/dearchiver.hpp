#ifndef ARCHIVER_DEARCHIVER_HPP
#define ARCHIVER_DEARCHIVER_HPP

#include "../database/archived_database.hpp"
#include "common.h"
#include <span>

class Dearchiver {
public:
  Dearchiver(std::shared_ptr<ArchivedDatabase>& archivedDatabase,
             const std::filesystem::path& archiveDirectoryLocation,
             const std::filesystem::path& archiveTempDirectoryLocation,
             std::span<char> fileReadBuffer);

  void dearchive(const std::filesystem::path& pathToDearchive,
                 const std::filesystem::path& dearchiveLocation);

  Dearchiver() = delete;
  Dearchiver(const Dearchiver&) = delete;
  Dearchiver(Dearchiver&&) = default;
  ~Dearchiver() = default;

  Dearchiver& operator=(const Dearchiver&) = delete;
  Dearchiver& operator=(Dearchiver&&) = default;

private:
  std::shared_ptr<ArchivedDatabase> archivedDatabase;
  std::filesystem::path archiveLocation;
  std::filesystem::path archiveTempLocation;
  std::vector<ArchiveID> decompressedArchives;
  std::span<char> readBuffer;

  bool hasArchiveBeenDecompressed(ArchiveID archiveId) const;
  void mergeArchiveParts(ArchiveID archiveId);
};

_make_exception_(DearchiverException);

#endif
