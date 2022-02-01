#include <catch2/catch_all.hpp>
#include <src/config/config.h>

TEST_CASE("Loading a complete config file", "[config]") {
  REQUIRE_NOTHROW(Config("./config/test_config.json"));
  Config config("./config/test_config.json");

  REQUIRE(config.archive.single_archive_size == 104857600);

  REQUIRE(config.stager.stage_directory ==
          "${ARCHIVER_TEST_CONFIG_STAGE_DIRECTORY_VALUE}");

  REQUIRE(config.archive.archive_directory ==
          "${ARCHIVER_TEST_CONFIG_ARCHIVE_DIRECTORY_VALUE}");
  REQUIRE(config.archive.temp_archive_directory ==
          "${ARCHIVER_TEST_CONFIG_ARCHIVE_TEMP_DIRECTORY_VALUE}");
  REQUIRE(config.archive.target_size == 10240);
  REQUIRE(config.archive.single_archive_size == 5120);

  REQUIRE(config.database.user ==
          "${ARCHIVER_TEST_CONFIG_DATABASE_USERNAME_VALUE}");
  REQUIRE(config.database.password ==
          "${ARCHIVER_TEST_CONFIG_DATABASE_PASSWORD_VALUE}");
  REQUIRE(config.database.location.host == "127.0.0.1");
  REQUIRE(config.database.location.port == 33060);
  REQUIRE(config.database.location.schema == "archiver");
  REQUIRE(config.database.options.size() == 1);
  REQUIRE(config.database.options.at(0) == "parseTime=true");

  REQUIRE(config.aws.access_key == "access_key");
  REQUIRE(config.aws.secret_key == "secret_key");
}

TEST_CASE("Loading a config file missing a single value", "[config]") {
  REQUIRE_THROWS_AS(Config("./config/test_config_incomplete1.json"),
                    ConfigError);
  REQUIRE_THROWS_WITH(
    Config("./config/test_config_incomplete1.json"),
    "Config file is missing a required entry \"stager/stage_directory\"");
}

TEST_CASE("Loading a config file missing an entire section", "[config]") {
  REQUIRE_THROWS_AS(Config("./config/test_config_incomplete2.json"),
                    ConfigError);
  REQUIRE_THROWS_WITH(Config("./config/test_config_incomplete2.json"),
                      "Config file is missing a required entry \"aws\"");
}

using Catch::Matchers::StartsWith;

TEST_CASE("Loading an empty config file", "[config]") {
  REQUIRE_THROWS_AS(Config("./config/test_config_empty.json"), ConfigError);
  REQUIRE_THROWS_WITH(
    Config("./config/test_config_empty.json"),
    StartsWith("The following error occured while reading the "
               "configuration file:\n\t"));
}

TEST_CASE("Loading a config file which doesn't exist", "[config]") {
  REQUIRE_THROWS_AS(Config("./config/test_config_non_existent.json"),
                    ConfigError);
  REQUIRE_THROWS_WITH(Config("./config/test_config_non_existent.json"),
                      StartsWith("Could not open the configuration file"));
}

TEST_CASE("Loading a config file with an incorrect value type", "[config]") {
  REQUIRE_THROWS_AS(Config("./config/test_config_incorrect_type.json"),
                    ConfigError);
  REQUIRE_THROWS_WITH(Config("./config/test_config_incorrect_type.json"),
                      StartsWith("Config file has value of incorrect type"));
}