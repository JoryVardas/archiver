#include "file.h"
#include <Hash/src/blake2.h>
#include <Hash/src/sha3.h>

FileHash::FileHash(std::basic_istream<char>& inputStream,
                   std::span<uint8_t> readBuffer) {
  Chocobo1::SHA3_512 sha3;
  Chocobo1::Blake2 blake2b;

  if (inputStream.bad())
    throw FileHashError(
      "Could not hash the file as the input stream is in a bad state.");

  while (!inputStream.eof()) {
    inputStream.read((char*)readBuffer.data(), readBuffer.size());
    Size read = inputStream.gcount();

    sha3.addData(readBuffer.data(), read);
    blake2b.addData(readBuffer.data(), read);
  }

  _sha3 = sha3.finalize().toString();
  _blake2b = blake2b.finalize().toString();
};

auto FileHash::getSHA3() const -> std::string_view { return _sha3; };
auto FileHash::getBlake2b() const -> std::string_view { return _blake2b; };

FileRevision::FileRevision(FileID parentFileID, const FileInfo& fileInfo,
                           TimeStamp archiveTime,
                           const Archive& containingArchive)
  : _fileId(parentFileID), _fileInfo(fileInfo), _archiveTime(archiveTime),
    _archive(containingArchive){};
const FileHash& FileRevision::hashs() const { return _fileInfo.hash; };
FileID FileRevision::parentFileID() const { return _fileId; };
Size FileRevision::size() const { return _fileInfo.size; };
TimeStamp FileRevision::archiveTime() const { return _archiveTime; };
const Archive& FileRevision::containingArchive() const { return _archive; };
std::string FileRevision::getName() const {
  auto timeAsMilliseconds =
    std::chrono::time_point_cast<std::chrono::milliseconds>(_archiveTime);
  auto timeAsSeconds =
    std::chrono::time_point_cast<std::chrono::seconds>(_archiveTime);
  auto miliseconds = timeAsMilliseconds - timeAsSeconds;
  return fmt::format("{}_{:%Y-%m-%e-%H:%M:%S}.{}", _fileId, _archiveTime,
                     miliseconds.count());
};

ArchivedFile::ArchivedFile(FileID id, const ArchivedDirectory& parent,
                           const std::string& name)
  : _id(id), _parent(parent), _name(name){};
FileID ArchivedFile::id() const { return _id; };
const ArchivedDirectory& ArchivedFile::parent() const { return _parent; };
const std::string& ArchivedFile::name() const { return _name; };