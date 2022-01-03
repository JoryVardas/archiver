#include "raw_file.hpp"
#include "../config/config.h"
#include <Hash/src/blake2.h>
#include <Hash/src/sha3.h>
#include <fstream>

RawFile::RawFile(const std::filesystem::path& path, std::span<uint8_t> buffer) {
  if (!std::filesystem::exists(path)) {
    throw FileDoesNotExist(
      fmt::format("The file \"{}\" could not be found", path));
  }
  if (!isFile(path)) {
    throw NotAFile(fmt::format("The path \"{}\" is not a file", path));
  }

  std::basic_ifstream<uint8_t> inputStream(path);

  if (inputStream.bad() || !inputStream.is_open()) {
    throw FileException(
      fmt::format("There was an error opening \"{}\" for reading", path));
  }

  Chocobo1::SHA3_512 sha3;
  Chocobo1::Blake2 blake2B;

  while (!inputStream.eof()) {
    inputStream.read(buffer.data(), buffer.size());

    if (inputStream.bad()) {
      throw FileException(
        fmt::format("There was an error reading \"{}\"", path));
    }

    Size read = inputStream.gcount();

    sha3.addData(buffer.data(), read);
    blake2B.addData(buffer.data(), read);
  }

  this->hash = sha3.finalize().toString() + blake2B.finalize().toString();
  this->size = std::filesystem::file_size(path);
  this->path = path;
}