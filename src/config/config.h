#ifndef _CONFIG_H
#define _CONFIG_H

#include "../app/common.h"

_make_exception_(ConfigError);

struct Config {
public:
  struct General {
    std::vector<Size> fileReadSizes;
  } general;
  struct Stager {
    std::filesystem::path stage_directory;
  } stager;
  struct Archive {
    std::filesystem::path archive_directory;
    std::filesystem::path temp_archive_directory;
    Size target_size;
    Size single_archive_size;
  } archive;
  struct Database {
    std::string user;
    std::string password;
    struct Location {
      std::string host;
      unsigned int port;
      std::string schema;
    } location;
    std::vector<std::string> options;
  } database;
  struct AWS {
    std::string access_key;
    std::string secret_key;
  } aws;

  Config(const std::filesystem::path& path);
};
#endif