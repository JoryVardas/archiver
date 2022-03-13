#include "archived_database.hpp"
#include <concepts>
#include <numeric>
#include <ranges>

using namespace std::string_literals;
namespace ranges = std::ranges;
namespace views = ranges::views;

namespace database::mock {
const std::string ArchivedDatabase::noExtensionArchiveContents = "<BLANK>"s;

ArchivedDatabase::ArchivedDatabase(Size archiveTargetSize)
  : targetSize(archiveTargetSize) {}

void ArchivedDatabase::startTransaction() {
  if (hasTransaction)
    throw DatabaseException(
      "Attempt to start a transaction on the archived database while already "
      "having an existing transaction");

  transactionArchivedDirectories = archivedDirectories;
  transactionArchivedFiles = archivedFiles;
  transactionArchives = archives;
  transactionArchiveNextPartNumbers = archiveNextPartNumbers;
  hasTransaction = true;
}
void ArchivedDatabase::rollback() {
  if (hasTransaction) {
    transactionArchivedDirectories.clear();
    transactionArchivedFiles.clear();
    transactionArchives.clear();
    transactionArchiveNextPartNumbers.clear();
    hasTransaction = false;
  }
}
void ArchivedDatabase::commit() {
  if (hasTransaction) {
    archivedDirectories = transactionArchivedDirectories;
    archivedFiles = transactionArchivedFiles;
    archives = transactionArchives;
    archiveNextPartNumbers = transactionArchiveNextPartNumbers;
    hasTransaction = false;
  }
}

auto ArchivedDatabase::getArchiveForFile(const StagedFile& file) -> Archive {
  const std::string_view fileName = file.name;
  const auto fileExtension =
    std::string{fileName.substr(fileName.find_last_of('.'))};
  auto archiveForExtension = getArchiveForExtension(fileExtension);

  if (getArchiveSize(archiveForExtension) < targetSize) {
    return archiveForExtension;
  }

  return addArchiveForExtension(fileExtension);
}
auto ArchivedDatabase::getNextArchivePartNumber(const Archive& archive)
  -> uint64_t {
  const auto found =
    ranges::find(getArchivePartNumberVector(), archive.id,
                 &decltype(archiveNextPartNumbers)::value_type::first);

  if (found == ranges::end(getArchivePartNumberVector()))
    throw ArchiveDatabaseException(
      FORMAT_LIB::format("Could not find archive with id {}"s, archive.id));

  return found->second;
}
void ArchivedDatabase::incrementNextArchivePartNumber(const Archive& archive) {
  auto found =
    ranges::find(getArchivePartNumberVector(), archive.id,
                 &decltype(archiveNextPartNumbers)::value_type::first);
  if (found == ranges::end(getArchivePartNumberVector()))
    throw ArchiveDatabaseException(FORMAT_LIB::format(
      "Could not update next archive part number of archive with id {}"s,
      archive.id));
  ++(found->second);
}

auto ArchivedDatabase::getArchiveForExtension(const std::string& extension)
  -> Archive {
  std::string extensionName = extension;
  if (extension.empty()) {
    extensionName = noExtensionArchiveContents;
  }
  const auto found =
    ranges::find(getArchiveVector(), extensionName, &Archive::extension);

  if (found == ranges::end(getArchiveVector()))
    return addArchiveForExtension(extensionName);
  else
    return *found;
}

auto ArchivedDatabase::addArchiveForExtension(const std::string& extension)
  -> Archive {
  std::string extensionName = extension;
  if (extension.empty()) {
    extensionName = noExtensionArchiveContents;
  }

  getArchiveVector().push_back({nextArchiveId, extensionName});
  getArchivePartNumberVector().push_back({nextArchiveId, 1});
  ++nextArchiveId;
  return getArchiveVector().back();
}

auto ArchivedDatabase::getArchiveSize(const Archive& archive) -> Size {
  auto allRevisions =
    getFileVector() |
    views::transform([](auto& file) { return file.revisions; }) | views::join;
  auto revisionsInArchive = allRevisions | views::filter([&](const auto& rev) {
                              return rev.containingArchiveId == archive.id;
                            });
  auto revisionSizes = revisionsInArchive |
                       views::transform([&](auto& rev) { return rev.size; }) |
                       views::common;

  return std::accumulate(ranges::begin(revisionSizes),
                         ranges::end(revisionSizes), Size{0});
}

auto ArchivedDatabase::listChildDirectories(const ArchivedDirectory& directory)
  -> std::vector<ArchivedDirectory> {
  std::vector<ArchivedDirectory> ret;
  ranges::copy_if(getDirectoryVector(), std::back_inserter(ret),
                  [&](const auto& dir) {
                    return dir.parent == directory.id &&
                           dir.name != ArchivedDirectory::RootDirectoryName;
                  });
  return ret;
}
auto ArchivedDatabase::addDirectory(const StagedDirectory& directory,
                                    const ArchivedDirectory& parent)
  -> ArchivedDirectory {
  if (directory.name == ArchivedDirectory::RootDirectoryName)
    return getRootDirectory();

  const auto found =
    ranges::find_if(getDirectoryVector(), [&](const auto& dir) {
      return dir.parent == parent.id && dir.name == directory.name;
    });
  if (found != ranges::end(getDirectoryVector())) {
    return *found;
  }

  getDirectoryVector().push_back(
    {nextArchivedDirectoryId++, directory.name, parent.id});
  return getDirectoryVector().back();
}
auto ArchivedDatabase::getRootDirectory() -> ArchivedDirectory {
  return getDirectoryVector().front();
}

auto ArchivedDatabase::listChildFiles(const ArchivedDirectory& directory)
  -> std::vector<ArchivedFile> {
  std::vector<ArchivedFile> ret;
  ranges::copy_if(
    getFileVector(), std::back_inserter(ret),
    [&](const auto& file) { return file.parentDirectory.id == directory.id; });
  return ret;
}

auto ArchivedDatabase::addFile(const StagedFile& stagedFile,
                               const ArchivedDirectory& directory,
                               const Archive& archive)
  -> ArchivedFileAddedType {
  auto existingFile = ranges::find_if(getFileVector(), [&](const auto& file) {
    return file.parentDirectory.id == directory.id &&
           file.name == stagedFile.name;
  });

  auto& addedFile = [&]() -> ArchivedFile& {
    if (existingFile != ranges::end(getFileVector())) {
      return *existingFile;
    } else {
      getFileVector().push_back(
        {nextArchivedFileId++, stagedFile.name, directory, {}});
      return getFileVector().back();
    }
  }();

  auto allRevisions =
    getFileVector() |
    views::transform([](auto& file) { return file.revisions; }) | views::join;

  const auto& duplicateRevision =
    ranges::find_if(allRevisions, [&](const auto& rev) {
      return rev.hash == stagedFile.hash && rev.size == stagedFile.size;
    });

  if (duplicateRevision == ranges::end(allRevisions)) {
    addedFile.revisions.push_back(
      {nextFileRevisionId++, stagedFile.hash, stagedFile.size,
       std::chrono::system_clock::now(), archive.id});
    return ArchivedFileAddedType::NewRevision;
  } else {
    addedFile.revisions.push_back(*duplicateRevision);
    return ArchivedFileAddedType::DuplicateRevision;
  }
}

auto ArchivedDatabase::getFileVector() -> decltype(archivedFiles)& {
  if (hasTransaction)
    return transactionArchivedFiles;
  else
    return archivedFiles;
}
auto ArchivedDatabase::getDirectoryVector() -> decltype(archivedDirectories)& {
  if (hasTransaction)
    return transactionArchivedDirectories;
  else
    return archivedDirectories;
}
auto ArchivedDatabase::getArchiveVector() -> decltype(archives)& {
  if (hasTransaction)
    return transactionArchives;
  else
    return archives;
}
auto ArchivedDatabase::getArchivePartNumberVector()
  -> decltype(archiveNextPartNumbers)& {
  if (hasTransaction)
    return transactionArchiveNextPartNumbers;
  else
    return archiveNextPartNumbers;
}
}