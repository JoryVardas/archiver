#ifndef ARCHIVER_COMPRESSOR_HPP
#define ARCHIVER_COMPRESSOR_HPP

#include "../database/archived_database.hpp"
#include "archive.h"
#include "common.h"

class Compressor {
public:
  Compressor() = delete;
  Compressor(const Compressor&) = delete;
  Compressor(Compressor&&) = default;
  ~Compressor() = default;

  Compressor(std::shared_ptr<ArchivedDatabase>& archivedDatabase,
             const std::vector<std::filesystem::path>& archiveLocations);

  void compress(const Archive& archive);
  void decompress(ArchiveID archiveId,
                  const std::filesystem::path& destination);
  void decompressSingleArchive(ArchivedFileRevisionID revisionId,
                               const std::filesystem::path& destination);

  Compressor& operator=(const Compressor&) = delete;
  Compressor& operator=(Compressor&&) = default;

private:
  std::shared_ptr<ArchivedDatabase> archivedDatabase;
  std::vector<std::filesystem::path> archiveLocations;

  void compressSingleArchives();
};

_make_exception_(CompressorException);

#endif
