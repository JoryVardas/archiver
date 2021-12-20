#ifndef ARCHIVER_COMMANDLINE_OPTIONS_HPP
#define ARCHIVER_COMMANDLINE_OPTIONS_HPP

#include "common.h"
#include <cxxsubs.hpp>

_make_exception_(CommandValidateException);

class StageCommand : public cxxsubs::IOptions {
public:
  StageCommand();

  int validate() override;
  int exec() override;
};
class ArchiveCommand : public cxxsubs::IOptions {
public:
  ArchiveCommand();

  int validate() override;
  int exec() override;
};
class UploadCommand : public cxxsubs::IOptions {
public:
  UploadCommand();

  int validate() override;
  int exec() override;
};
class DearchiveCommand : public cxxsubs::IOptions {
public:
  DearchiveCommand();

  int validate() override;
  int exec() override;
};

#endif
