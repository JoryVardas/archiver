#ifndef _CONFIG_H
#define _CONFIG_H

#include "../app/common.h"

_make_exception_(ConfigError);

struct Config {
  public:
    struct General {
        Size _fileReadSize;
    } general;
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
            uint64_t port;
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