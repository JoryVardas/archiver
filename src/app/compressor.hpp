#ifndef ARCHIVER_COMPRESSOR_HPP
#define ARCHIVER_COMPRESSOR_HPP

#include "../database/archived_database.hpp"
#include "archive.h"
#include "common.h"

template <ArchivedDatabase ArchivedDatabase> class Compressor {
public:
  Compressor() = delete;
  Compressor(const Compressor&) = delete;
  Compressor(Compressor&&) = default;
  ~Compressor() = default;

  Compressor(std::shared_ptr<ArchivedDatabase>& archivedDatabase,
             const std::filesystem::path& archiveLocation);

  void compress(const Archive& archive);

  Compressor& operator=(const Compressor&) = delete;
  Compressor& operator=(Compressor&&) = default;

private:
  std::shared_ptr<ArchivedDatabase> archivedDatabase;
  std::filesystem::path archiveLocation;

  void compressSingleArchives();
};

_make_exception_(CompressorException);

#include "compressor.tpp"

#endif
