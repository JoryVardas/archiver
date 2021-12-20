#ifndef ARCHIVER_STAGER_HPP
#define ARCHIVER_STAGER_HPP

#include "../database/staged_directory_database.hpp"
#include "../database/staged_file_database.hpp"
#include "common.h"
#include <span>

template <StagedFileDatabase StagedFileDatabase,
          StagedDirectoryDatabase StagedDirectoryDatabase>
class Stager {
public:
  Stager(std::shared_ptr<StagedFileDatabase>& fileDatabase,
         std::shared_ptr<StagedDirectoryDatabase>& directoryDatabase,
         std::span<uint8_t> fileReadBuffer);

  void stage(const std::vector<std::filesystem::path>& paths,
             std::string_view prefixToRemove);

  Stager() = delete;
  Stager(const Stager&) = delete;
  Stager(Stager&&) = default;
  ~Stager() = default;

private:
  void stageFile(const std::filesystem::path& path,
                 const std::filesystem::path& stagePath);
  void stageDirectory(const std::filesystem::path& path,
                      const std::filesystem::path& stagePath);

  std::shared_ptr<StagedFileDatabase> stagedFileDatabase;
  std::shared_ptr<StagedDirectoryDatabase> stagedDirectoryDatabase;
  std::span<uint8_t> readBuffer;

  using path = std::filesystem::path;
};

_make_exception_(StagerException);

#include "stager.tpp"

#endif
