#ifndef ARCHIVER_GET_FILE_READ_BUFFER_HPP
#define ARCHIVER_GET_FILE_READ_BUFFER_HPP

#include "../../config/config.h"
#include "../common.h"
#include <memory>

namespace {
std::pair<std::unique_ptr<uint8_t[]>, Size>
getFileReadBuffer(const Config& config) {
  for (auto size : config.general.fileReadSizes) {
    try {
      auto data = std::make_unique<uint8_t[]>(size);
      return std::make_pair(std::move(data), size);
    } catch (const std::bad_alloc&) {
      // If there was a bad alloc, try the next available size
      continue;
    }
  }
  // If none of the sizes could be allocated then throw an error.
  throw std::runtime_error(FORMAT_LIB::format(
    "Unable to allocate a file read buffer of any of the given sizes: {:,}",
    config.general.fileReadSizes));
}
}

#endif
