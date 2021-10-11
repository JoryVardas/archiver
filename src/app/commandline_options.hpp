#ifndef ARCHIVER_COMMANDLINE_OPTIONS_HPP
#define ARCHIVER_COMMANDLINE_OPTIONS_HPP

#include "common.h"
#include <cxxsubs.hpp>

_make_exception_(CommandValidateException);

class StageCommand : public cxxsubs::IOptions {
public:
  StageCommand();

  void validate() override;
  void exec() override;
};
class ArchiveCommand : public cxxsubs::IOptions {
public:
  ArchiveCommand();

  void validate() override;
  void exec() override;
};
class UploadCommand : public cxxsubs::IOptions {
public:
  UploadCommand();

  void validate() override;
  void exec() override;
};
class DearchiveCommand : public cxxsubs::IOptions {
public:
  DearchiveCommand();

  void validate() override;
  void exec() override;
};

#endif
