#ifndef ARCHIVER_GET_FILE_READ_BUFFER_HPP
#define ARCHIVER_GET_FILE_READ_BUFFER_HPP

#include "../../config/config.h"
#include "../common.h"
#include <memory>
#include <vector>

namespace {
std::pair<std::unique_ptr<char[]>, Size>
getFileReadBuffer(const std::vector<Size> potentialSizes) {
  for (auto size : potentialSizes) {
    try {
      auto data = std::make_unique<char[]>(size);
      return std::make_pair(std::move(data), size);
    } catch (const std::bad_alloc&) {
      // If there was a bad alloc, try the next available size
      continue;
    }
  }
  // If none of the sizes could be allocated then throw an error.
  throw std::runtime_error(FORMAT_LIB::format(
    "Unable to allocate a file read buffer of any of the given sizes: {:,}",
    potentialSizes));
}
}

#endif
