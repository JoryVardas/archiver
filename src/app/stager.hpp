#ifndef ARCHIVER_STAGER_HPP
#define ARCHIVER_STAGER_HPP

#include "../database/staged_database.hpp"
#include "common.h"
#include <span>

class Stager {
public:
  Stager(std::shared_ptr<StagedDatabase>& stagedDatabase,
         std::span<char> fileReadBuffer,
         const std::filesystem::path& stageDirectoryLocation);

  void stage(const std::vector<std::filesystem::path>& paths,
             std::string_view prefixToRemove);

  auto getDirectoriesSorted() -> std::vector<StagedDirectory>;
  auto getFilesSorted() -> std::vector<StagedFile>;

  Stager() = delete;
  Stager(const Stager&) = delete;
  Stager(Stager&&) = default;
  ~Stager() = default;

  Stager& operator=(const Stager&) = delete;
  Stager& operator=(Stager&&) = default;

private:
  void stageFile(const std::filesystem::path& path,
                 const std::filesystem::path& stagePath);
  void stageDirectory(const std::filesystem::path& path,
                      const std::filesystem::path& stagePath);

  std::shared_ptr<StagedDatabase> stagedDatabase;
  std::span<char> readBuffer;
  std::filesystem::path stageLocation;

  using path = std::filesystem::path;
};

_make_exception_(StagerException);

#endif
