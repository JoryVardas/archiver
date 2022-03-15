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
  std::string hash;
  std::filesystem::path path;

  RawFile(const std::filesystem::path& path, std::span<char> buffer);
};
#endif
