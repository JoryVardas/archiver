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
class StagedDatabase : public ::StagedDatabase {
public:
  void startTransaction() final;
  void commit() final;
  void rollback() final;

  auto listAllFiles() -> std::vector<StagedFile> final;
  auto add(const RawFile& file, const std::filesystem::path& stagePath)
    -> StagedFile final;
  void remove(const StagedFile& stagedFile) final;
  void removeAllFiles() final;

  auto listAllDirectories() -> std::vector<StagedDirectory> final;
  void add(const std::filesystem::path& stagePath) final;
  void remove(const StagedDirectory& stagedDirectory) final;
  void removeAllDirectories() final;
  auto getRootDirectory() -> std::optional<StagedDirectory> final;

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
