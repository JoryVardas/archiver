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
    throw ConfigError("Could not open the configuration file \"{}\".", path);

  json configuration;

  try {
    configFile >> configuration;
  } catch (json::parse_error& e) {
    throw ConfigError("The following error occured while reading the "
                      "configuration file:\n\t{}.",
                      e);
  }

  configFile.close();

  const auto getRequired = [&](const std::string& jsonPointer) {
    try {
      return configuration.at(json::json_pointer{jsonPointer});
    } catch (const json::out_of_range& err) {
      auto jsonPointerView = std::string_view{jsonPointer};
      jsonPointerView.remove_prefix(1);
      throw ConfigError("Config file is missing a required entry \"{}\"",
                        jsonPointerView);
    }
  };

  const auto getRequiredValue = [&](const std::string& jsonPointer, auto& var) {
    try {
      getRequired(jsonPointer).get_to(var);
    } catch (const json::type_error& err) {
      auto jsonPointerView = std::string_view{jsonPointer};
      jsonPointerView.remove_prefix(1);
      throw ConfigError("Config file could not be loaded as a required entry "
                        "\"{}\" is of an incorrect type: {}",
                        jsonPointerView, err.what());
    }
  };

  // Call getRequired with on objects even though their values need to be
  // retrieved separately, this is so that an error is generated if the section
  // does not exist.

  getRequired("/general"s);
  getRequiredValue("/general/file_read_sizes"s, this->general.fileReadSizes);

  getRequired("/stager"s);
  getRequiredValue("/stager/stage_directory"s, this->stager.stage_directory);

  getRequired("/archive"s);
  getRequiredValue("/archive/archive_directory"s,
                   this->archive.archive_directory);
  getRequiredValue("/archive/temp_archive_directory"s,
                   this->archive.temp_archive_directory);
  getRequiredValue("/archive/target_size"s, this->archive.target_size);
  getRequiredValue("/archive/single_archive_size"s,
                   this->archive.single_archive_size);

  getRequired("/database"s);
  getRequiredValue("/database/user"s, this->database.user);
  getRequiredValue("/database/password"s, this->database.password);
  getRequiredValue("/database/options"s, this->database.options);

  getRequired("/database/location"s);
  getRequiredValue("/database/location/host"s, this->database.location.host);
  getRequiredValue("/database/location/port"s, this->database.location.port);
  getRequiredValue("/database/location/schema"s,
                   this->database.location.schema);

  getRequired("/aws"s);
  getRequiredValue("/aws/access_key"s, this->aws.access_key);
  getRequiredValue("/aws/secret_key"s, this->aws.secret_key);
}