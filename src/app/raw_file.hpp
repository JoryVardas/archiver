#ifndef ARCHIVER_RAW_FILE_HPP
#define ARCHIVER_RAW_FILE_HPP

#include "common.h"
#include <span>

_make_exception_(FileDoesNotExist);
_make_exception_(NotAFile);
_make_exception_(FileException);

struct RawFile {
public:
  std::uint64_t size;
  std::string shaHash;
  std::string blakeHash;
  std::filesystem::path path;

  RawFile(const std::filesystem::path& path, std::span<uint8_t> buffer);
};
#endif
