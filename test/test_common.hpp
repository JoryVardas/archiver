#ifndef ARCHIVER_TEST_COMMON_HPP
#define ARCHIVER_TEST_COMMON_HPP
#include <catch2/catch_all.hpp>
#include <src/database/archived_database.hpp>
#include <src/database/staged_database.hpp>

template <StagedDatabase StagedDatabase, ArchivedDatabase ArchivedDatabase>
bool databasesAreEmpty(std::shared_ptr<StagedDatabase> stagedDatabase,
                       std::shared_ptr<ArchivedDatabase> archivedDatabase) {
  bool ret = true;
  if (!stagedDatabase->listAllDirectories().empty()) {
    UNSCOPED_INFO("Staged database has directories in it");
    ret = false;
  }
  if (!stagedDatabase->listAllFiles().empty()) {
    UNSCOPED_INFO("Staged database has files in it");
    ret = false;
  }
  if (!archivedDatabase
         ->listChildDirectories(archivedDatabase->getRootDirectory())
         .empty()) {
    UNSCOPED_INFO("Archived database has directories in it");
    ret = false;
  }
  if (!archivedDatabase->listChildFiles(archivedDatabase->getRootDirectory())
         .empty()) {
    UNSCOPED_INFO("Archived database has files in it");
    ret = false;
  }

  return ret;
};

#endif
