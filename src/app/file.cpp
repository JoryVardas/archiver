#include "file.h"
#include <Hash/src/blake2.h>
#include <Hash/src/sha3.h>

FileHash::FileHash(std::basic_istream<char>& inputStream,
                   FileReadBuffer& readBuffer) {
  Chocobo1::SHA3_512 sha3;
  Chocobo1::Blake2 blake2b;

  if (inputStream.bad())
    throw FileHashError(
      "Could not hash the file as the input stream is in a bad state.");

  auto& buffer = readBuffer.getBuffer();

  while (!inputStream.eof()) {
    inputStream.read(buffer.rawData(), buffer.size());
    Size read = inputStream.gcount();

    sha3.addData(buffer.rawData(), read);
    blake2b.addData(buffer.rawData(), read);
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

RawFile::RawFile(const std::filesystem::path& path,
                 const std::filesystem::path& relativePath,
                 FileReadBuffer& buffer)
  : _name(relativePath.filename()), _extension(relativePath.extension()),
    _parent(relativePath.parent_path()),
    _size(std::filesystem::file_size(path)) {
  if (!pathExists(path))
    throw RawFileError(fmt::format(
      "Attempt to open the path \"{}\" which does not exists", path));

  if (!isFile(path))
    throw RawFileError(
      fmt::format("Could not open the path \"{}\" as a file.", path));

  std::ifstream inputStream;
  try {
    inputStream.open(path, inputStream.binary);
  } catch (std::exception& e) {
    throw RawFileError(
      fmt::format("The following error occured while trying to open the raw "
                  "file \"{}\":\n\t{}.",
                  path, e));
  } catch (...) {
    throw RawFileError(fmt::format(
      "An unknown error occured while trying to open the file {}", path));
  }
  if (inputStream.bad())
    throw RawFileError(fmt::format(
      "An unknown error occured while trying to open the file {}", path));
  _fileHash = FileHash(inputStream, buffer);
  inputStream.close();
};

const std::string& RawFile::name() const { return _name; };
const Extension& RawFile::extension() const { return _extension; };
auto RawFile::parent() const -> const std::filesystem::path& {
  return _parent;
};
Size RawFile::size() const { return _size; };
auto RawFile::getHashes() const -> const FileHash& { return _fileHash; }