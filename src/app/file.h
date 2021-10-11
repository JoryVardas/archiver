#ifndef _FILE_H
#define _FILE_H

#include "archive.h"
#include "common.h"
#include "directory.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

class FileReadBuffer {
public:
  virtual CArray<char, Size>& getBuffer() abstract;
};

_make_exception_(FileHashError);

struct FileHash {
  FileHash(std::basic_istream<char>& inputStream, FileReadBuffer& readBuffer);
  FileHash(std::string_view sha3, std::string_view blake2b);

  auto getSHA3() const -> std::string_view;
  auto getBlake2b() const -> std::string_view;

private:
  std::string _sha3;
  std::string _blake2b;
};

struct FileInfo {
  FileHash hash;
  Size size;
};

using FileID = ID;

struct FileRevision {
  FileRevision(FileID parentFileID, const FileInfo& fileInfo,
               TimeStamp archiveTime, const Archive& containingArchive);
  const FileHash& hashs() const;
  FileID parentFileID() const;
  Size size() const;
  TimeStamp archiveTime() const;
  const Archive& containingArchive() const;
  std::string getName() const;

private:
  FileID _fileId;
  FileInfo _fileInfo;
  TimeStamp _archiveTime;
  Archive _archive;
};

struct ArchivedFile {
  ArchivedFile(FileID id, const ArchivedDirectory& parent,
               const std::string& name);

  FileID id() const;
  const ArchivedDirectory& parent() const;
  const std::string& name() const;

private:
  FileID _id;
  ArchivedDirectory _parent;
  std::string _name;
};

_make_exception_(RawFileError);
struct RawFile {
  RawFile(const std::filesystem::path& path, FileReadBuffer& buffer);

  const std::string& name() const;
  const Extension& extension() const;
  const RawDirectory& parent() const;
  Size size() const;
  auto getHashes() const -> const FileHash&;
  std::filesystem::path getFullPath() const;

private:
  std::string _name;
  Extension _extension;
  RawDirectory _parent;
  Size _size;
  FileHash _fileHash;
};

#endif