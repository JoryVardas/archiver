#ifndef ARCHIVER_MOCK_STAGED_DATABASE_HPP
#define ARCHIVER_MOCK_STAGED_DATABASE_HPP

#include <optional>
#include <src/app/common.h>
#include <src/app/raw_file.hpp>
#include <src/app/staged_directory.h>
#include <src/app/staged_file.hpp>
#include <src/database/staged_database.hpp>
#include <vector>

namespace database::mock {
class StagedDatabase {
public:
  void startTransaction();
  void commit();
  void rollback();

  auto listAllFiles() -> std::vector<StagedFile>;
  auto add(const RawFile& file, const std::filesystem::path& stagePath)
    -> StagedFile;
  void remove(const StagedFile& stagedFile);
  void removeAllFiles();

  auto listAllDirectories() -> std::vector<StagedDirectory>;
  void add(const std::filesystem::path& stagePath);
  void remove(const StagedDirectory& stagedDirectory);
  void removeAllDirectories();
  auto getRootDirectory() -> std::optional<StagedDirectory>;

private:
  std::vector<StagedDirectory> stagedDirectories;
  std::vector<StagedFile> stagedFiles;
  std::vector<StagedDirectory> transactionStagedDirectories;
  std::vector<StagedFile> transactionStagedFiles;
  bool hasTransaction = false;
  StagedFileID nextStagedFileId = 1;
  StagedDirectoryID nextStagedDirectoryId = 1;

  auto getStagedDirectory(const std::filesystem::path& stagePath)
    -> std::optional<StagedDirectory>;

  auto getFileVector() -> decltype(stagedFiles)&;
  auto getDirectoryVector() -> decltype(stagedDirectories)&;
};
}
#endif
