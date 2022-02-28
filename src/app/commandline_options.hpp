#ifndef ARCHIVER_COMMANDLINE_OPTIONS_HPP
#define ARCHIVER_COMMANDLINE_OPTIONS_HPP

#include "common.h"
#include <cxxsubs.hpp>

_make_exception_(CommandValidateException);

class StageCommand : public cxxsubs::IOptions {
public:
  StageCommand();
  StageCommand(const StageCommand&) = delete;
  StageCommand(StageCommand&&) = delete;
  virtual ~StageCommand() = default;

  StageCommand& operator=(const StageCommand&) = delete;
  StageCommand& operator=(StageCommand&&) = delete;

  int validate() override;
  int exec() override;
};
class ArchiveCommand : public cxxsubs::IOptions {
public:
  ArchiveCommand();
  ArchiveCommand(const ArchiveCommand&) = delete;
  ArchiveCommand(ArchiveCommand&&) = delete;
  virtual ~ArchiveCommand() = default;

  ArchiveCommand& operator=(const ArchiveCommand&) = delete;
  ArchiveCommand& operator=(ArchiveCommand&&) = delete;

  int validate() override;
  int exec() override;
};
class UploadCommand : public cxxsubs::IOptions {
public:
  UploadCommand();
  UploadCommand(const UploadCommand&) = delete;
  UploadCommand(UploadCommand&&) = delete;
  virtual ~UploadCommand() = default;

  UploadCommand& operator=(const UploadCommand&) = delete;
  UploadCommand& operator=(UploadCommand&&) = delete;

  int validate() override;
  int exec() override;
};
class DearchiveCommand : public cxxsubs::IOptions {
public:
  DearchiveCommand();
  DearchiveCommand(const DearchiveCommand&) = delete;
  DearchiveCommand(DearchiveCommand&&) = delete;
  virtual ~DearchiveCommand() = default;

  DearchiveCommand& operator=(const DearchiveCommand&) = delete;
  DearchiveCommand& operator=(DearchiveCommand&&) = delete;

  int validate() override;
  int exec() override;
};

#endif
