#ifndef _FILE_H
#define _FILE_H

#include "archive.h"
#include "common.h"
#include "directory.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <span>
#include <string>

_make_exception_(FileHashError);

struct FileHash {
  FileHash(std::basic_istream<char>& inputStream,
           std::span<uint8_t> readBuffer);
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

#endif