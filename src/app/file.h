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
    using Sha3 = std::array<uint8_t, 64>;
    using Blake2b = std::array<uint8_t, 64>;

    FileHash(std::basic_istream<char>& inputStream, FileReadBuffer& readBuffer);
    FileHash(const Sha3& sha3, const Blake2b& blake2b);

    const Sha3& getSHA3() const;
    const Blake2b& getBlake2b() const;

  private:
    Sha3 _sha3;
    Blake2b _blake2b;
};

struct FileInfo {
    FileHash hash;
    Size size;
};

struct FileRevision {
    FileRevision(ID parentFileID, const FileInfo& fileInfo,
                 TimeStamp archiveTime, const Archive& containingArchive);
    const FileHash& hashs() const;
    ID parentFileID() const;
    Size size() const;
    TimeStamp archiveTime() const;
    const Archive& containingArchive() const;
    std::string getName() const;

  private:
    ID _fileId;
    FileInfo _fileInfo;
    TimeStamp _archiveTime;
    Archive _archive;
};

struct ArchivedFile {
    ArchivedFile(ID id, const ArchivedDirectory& parent,
                 const std::string& name);

    ID id() const;
    const ArchivedDirectory& parent() const;
    const std::string& name() const;

  private:
    ID _id;
    ArchivedDirectory _parent;
    std::string _name;
};

_make_exception_(RawFileError);
struct RawFile {
    RawFile(const std::filesystem::path& path);

    const std::string& name() const;
    const Extension& extension() const;
    const RawDirectory& parent() const;
    Size size() const;
    std::basic_istream<char>& getStream();
    std::filesystem::path getFullPath() const;

  private:
    std::string _name;
    Extension _extension;
    RawDirectory _parent;
    Size _size;
    std::ifstream _inputStream;
};

#endif