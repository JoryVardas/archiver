#include "config.h"
#include <exception>
#include <fstream>
#include <json.hpp>

#include <iostream>

_make_formatter_for_exception_(nlohmann::json::parse_error);

Config::Config(const std::filesystem::path& path) {
    using namespace nlohmann;

    std::ifstream configFile(path);
    if (!configFile.is_open())
        throw ConfigError(
            fmt::format("Could not open the configuration file \"{}\".", path));

    json configuration;

    try {
        configFile >> configuration;
    } catch (json::parse_error& e) {
        throw ConfigError(
            fmt::format("The following error occured while reading the "
                        "configuration file:\n\t{}.",
                        e));
    }

    configFile.close();

    auto general = configuration["general"];
    auto archive = configuration["archive"];
    auto database = configuration["database"];
    auto aws = configuration["aws"];

    if (general.is_null()) {
        throw ConfigError(
            "Config file is missing a required section \"general\".");
    }
    if (archive.is_null()) {
        throw ConfigError(
            "Config file is missing a required section \"archive\".");
    }
    if (database.is_null()) {
        throw ConfigError(
            "Config file is missing a required section \"database\".");
    }
    if (aws.is_null()) {
        throw ConfigError("Config file is missing a required section \"aws\".");
    }

    auto general_file_read_size = general["file_read_size"];
    auto archive_archive_directory = archive["archive_directory"];
    auto archive_temp_archive_directory = archive["temp_archive_directory"];
    auto archive_target_size = archive["target_size"];
    auto archive_single_archive_size = archive["single_archive_size"];
    auto database_user = database["user"];
    auto database_password = database["password"];
    auto database_location = database["location"];
    auto database_options = database["options"];
    auto aws_access_key = aws["access_key"];
    auto aws_secret_key = aws["secret_key"];

    if (general_file_read_size.is_null()) {
        throw ConfigError("Config file is missing a required entry "
                          "\"file_read_size\" in section \"general\".");
    }
    if (archive_archive_directory.is_null()) {
        throw ConfigError("Config file is missing a required entry "
                          "\"archive_directory\" in section \"archive\".");
    }
    if (archive_temp_archive_directory.is_null()) {
        throw ConfigError("Config file is missing a required entry "
                          "\"temp_archive_directory\" in section \"archive\".");
    }
    if (archive_target_size.is_null()) {
        throw ConfigError("Config file is missing a required entry "
                          "\"target_size\" in section \"archive\".");
    }
    if (archive_single_archive_size.is_null()) {
        throw ConfigError("Config file is missing a required entry "
                          "\"single_archive_size\" in section \"archive\".");
    }
    if (database_user.is_null()) {
        throw ConfigError("Config file is missing a required entry \"user\" in "
                          "section \"database\".");
    }
    if (database_password.is_null()) {
        throw ConfigError("Config file is missing a required entry "
                          "\"password\" in section \"database\".");
    }
    if (database_location.is_null()) {
        throw ConfigError("Config file is missing a required entry "
                          "\"location\" in section \"database\".");
    }
    if (database_options.is_null()) {
        throw ConfigError("Config file is missing a required entry \"options\" "
                          "in section \"database\".");
    }
    if (aws_access_key.is_null()) {
        throw ConfigError("Config file is missing a required entry "
                          "\"access_key\" in section \"aws\".");
    }
    if (aws_secret_key.is_null()) {
        throw ConfigError("Config file is missing a required entry "
                          "\"secret_key\" in section \"aws\".");
    }

    auto database_location_host = database_location["host"];
    auto database_location_port = database_location["port"];
    auto database_location_schema = database_location["schema"];

    if (database_location_host.is_null()) {
        throw ConfigError("Config file is missing a required entry \"host\" in "
                          "section \"database.location\".");
    }
    if (database_location_port.is_null()) {
        throw ConfigError("Config file is missing a required entry \"port\" in "
                          "section \"database.location\".");
    }
    if (database_location_schema.is_null()) {
        throw ConfigError("Config file is missing a required entry \"schema\" "
                          "in section \"database.location\".");
    }

    this->general._fileReadSize = general_file_read_size.get<uint64_t>();
    this->archive.archive_directory =
        archive_archive_directory.get<std::string>();
    this->archive.temp_archive_directory =
        archive_temp_archive_directory.get<std::string>();
    this->archive.target_size = archive_target_size.get<uint64_t>();
    this->archive.single_archive_size =
        archive_single_archive_size.get<uint64_t>();
    this->database.user = database_user.get<std::string>();
    this->database.password = database_password.get<std::string>();
    this->database.location.host = database_location_host.get<std::string>();
    this->database.location.port = database_location_port.get<uint64_t>();
    this->database.location.schema =
        database_location_schema.get<std::string>();
    this->database.options = database_options.get<std::vector<std::string>>();
    this->aws.access_key = aws_access_key.get<std::string>();
    this->aws.secret_key = aws_secret_key.get<std::string>();
};