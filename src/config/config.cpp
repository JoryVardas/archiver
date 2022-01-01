#include "config.h"
#include <fstream>
#include <json.hpp>

#include <iostream>

_make_formatter_for_exception_(nlohmann::json::parse_error);

Config::Config(const std::filesystem::path& path) {
  using namespace nlohmann;
  using namespace std::string_literals;

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

  const auto getRequired = [&](const std::string& jsonPointer) {
    try {
      return configuration.at(json::json_pointer{jsonPointer});
    } catch (const json::out_of_range& err) {
      auto jsonPointerView = std::string_view{jsonPointer};
      jsonPointerView.remove_prefix(1);
      throw ConfigError(fmt::format(
        "Config file is missing a required entry \"{}\"", jsonPointerView));
    }
  };

  // Call getRequired with on objects even though their values need to be
  // retrieved separately, this is so that an error is generated if the section
  // does not exist.

  getRequired("/general"s);
  getRequired("/general/file_read_sizes"s).get_to(this->general.fileReadSizes);

  getRequired("/stager"s);
  getRequired("/stager/stage_directory"s).get_to(this->stager.stage_directory);

  getRequired("/archive"s);
  getRequired("/archive/archive_directory"s)
    .get_to(this->archive.archive_directory);
  getRequired("/archive/temp_archive_directory"s)
    .get_to(this->archive.temp_archive_directory);
  getRequired("/archive/target_size"s).get_to(this->archive.target_size);
  getRequired("/archive/single_archive_size"s)
    .get_to(this->archive.single_archive_size);

  getRequired("/database"s);
  getRequired("/database/user"s).get_to(this->database.user);
  getRequired("/database/password"s).get_to(this->database.password);
  getRequired("/database/options"s).get_to(this->database.options);

  getRequired("/database/location"s);
  getRequired("/database/location/host"s).get_to(this->database.location.host);
  getRequired("/database/location/port"s).get_to(this->database.location.port);
  getRequired("/database/location/schema"s)
    .get_to(this->database.location.schema);

  getRequired("/aws"s);
  getRequired("/aws/access_key"s).get_to(this->aws.access_key);
  getRequired("/aws/secret_key"s).get_to(this->aws.secret_key);
}