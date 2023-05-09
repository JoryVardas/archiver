#include "raw_file.hpp"
#include <Hash/src/blake2.h>
#include <Hash/src/sha3.h>
#include <fstream>

RawFile::RawFile(const std::filesystem::path& path, std::span<char> buffer) {
  if (buffer.size() >
      static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
    throw std::logic_error(
      "Could not construct RawFile as the given buffer is larger than the "
      "maximum input read stream input size.");
  if (!std::filesystem::exists(path)) {
    throw FileDoesNotExist("The file \"{}\" could not be found", path);
  }
  if (!std::filesystem::is_regular_file(path)) {
    throw NotAFile("The path \"{}\" is not a file", path);
  }

  std::basic_ifstream<char> inputStream(path, std::ios_base::binary);

  if (inputStream.bad() || !inputStream.is_open()) {
    throw FileException("There was an error opening \"{}\" for reading", path);
  }

  Chocobo1::SHA3_512 sha3;
  Chocobo1::Blake2 blake2B;

  while (!inputStream.eof()) {
    inputStream.read(buffer.data(),
                     static_cast<std::streamsize>(buffer.size()));

    if (inputStream.bad()) {
      throw FileException("There was an error reading \"{}\"", path);
    }

    Size read = static_cast<Size>(inputStream.gcount());

    sha3.addData(buffer.data(), read);
    blake2B.addData(buffer.data(), read);
  }

  this->hash = sha3.finalize().toString() + blake2B.finalize().toString();
  this->size = std::filesystem::file_size(path);
  this->path = path;
}