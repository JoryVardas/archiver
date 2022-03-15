#include "commandline_options.hpp"
#include "../config/config.h"
#include "../database/mysql_implementation/archived_database.hpp"
#include "../database/mysql_implementation/staged_database.hpp"
#include "archiver.hpp"
#include "stager.hpp"
#include "util/get_file_read_buffer.hpp"
#include <algorithm>
#include <filesystem>
#include <ranges>
#include <span>
#include <spdlog/spdlog.h>

namespace ranges = std::ranges;
using namespace database::mysql;

StageCommand::StageCommand()
  : cxxsubs::IOptions({"stage"}, "Stage files to be archived") {
  options.positional_help("<paths>").show_positional_help();

  // clang-format off
  options.add_options()
    ("help", "Print help")
    ("v, verbose", "Enable verbose output")
    ("prefix", "Specify a prefix to remove from the paths of files/directories "
      "being staged",
      cxxopts::value<std::filesystem::path>()->default_value("")
    )
    ("paths", "List of paths to stage",
      cxxopts::value<std::vector<std::filesystem::path>>(), "<paths>"
    )
    (
      "config", "Configuration file to use",
      cxxopts::value<std::filesystem::path>()->default_value("config.json")
    );
  // clang-format on

  options.parse_positional({"paths"});
}
int StageCommand::validate() {
  if (this->parse_result->count("help"))
    return EXIT_SUCCESS;

  if (this->parse_result->count("paths") < 1)
    throw CommandValidateException(
      "stage command requires at least one path in <paths>");

  return EXIT_SUCCESS;
}
int StageCommand::exec() {
  if (this->parse_result->count("help")) {
    std::cout << this->options.help({""}) << std::endl;
    return EXIT_SUCCESS;
  }

  if (this->parse_result->count("verbose") > 0)
    spdlog::set_level(spdlog::level::info);

  const auto& paths =
    (*this->parse_result)["paths"].as<std::vector<std::filesystem::path>>();

  const auto config =
    Config((*this->parse_result)["config"].as<std::filesystem::path>());

  const auto prefix = (*this->parse_result)["prefix"]
                        .as<std::filesystem::path>()
                        .generic_string();

  auto [dataPointer, size] = getFileReadBuffer(config);
  std::span span{dataPointer.get(), size};

  auto databaseConnectionConfig =
    std::make_shared<StagedDatabase::ConnectionConfig>();
  databaseConnectionConfig->database = config.database.location.schema;
  databaseConnectionConfig->host = config.database.location.host;
  databaseConnectionConfig->port = config.database.location.port;
  databaseConnectionConfig->user = config.database.user;
  databaseConnectionConfig->password = config.database.password;

  auto stagedDatabase =
    std::make_shared<StagedDatabase>(databaseConnectionConfig);

  Stager stager(stagedDatabase, stagedDatabase,
                std::span{dataPointer.get(), size},
                config.stager.stage_directory);

  stager.stage(paths, prefix);

  return EXIT_SUCCESS;
}

ArchiveCommand::ArchiveCommand()
  : cxxsubs::IOptions({"archive"}, "Archive staged files") {
  // clang-format off
  options.add_options()
    ("help", "Print help")
    ("v, verbose", "Enable verbose output")
    (
      "config", "Configuration file to use",
      cxxopts::value<std::filesystem::path>()->default_value("config.json")
    );
  // clang-format on
}
int ArchiveCommand::validate() {
  if (this->parse_result->count("help"))
    return EXIT_SUCCESS;
  return EXIT_SUCCESS;
}
int ArchiveCommand::exec() {
  if (this->parse_result->count("help")) {
    std::cout << this->options.help({""}) << std::endl;
    return EXIT_SUCCESS;
  }

  if (this->parse_result->count("verbose") > 0)
    spdlog::set_level(spdlog::level::info);

  const auto config =
    Config((*this->parse_result)["config"].as<std::filesystem::path>());

  auto fakeReadBuffer = std::array<char, 1>{0};

  auto databaseConnectionConfig =
    std::make_shared<StagedDatabase::ConnectionConfig>();
  databaseConnectionConfig->database = config.database.location.schema;
  databaseConnectionConfig->host = config.database.location.host;
  databaseConnectionConfig->port = config.database.location.port;
  databaseConnectionConfig->user = config.database.user;
  databaseConnectionConfig->password = config.database.password;

  auto stagedDatabase =
    std::make_shared<StagedDatabase>(databaseConnectionConfig);
  auto archivedDatabase = std::make_shared<ArchivedDatabase>(
    databaseConnectionConfig, config.archive.target_size);

  Stager stager(stagedDatabase, stagedDatabase,
                std::span{fakeReadBuffer.data(), 1},
                config.stager.stage_directory);
  Archiver<ArchivedDatabase, ArchivedDatabase, ArchivedDatabase> archiver(
    archivedDatabase, archivedDatabase, archivedDatabase,
    config.stager.stage_directory, config.archive.temp_archive_directory,
    config.archive.single_archive_size);

  archiver.archive(stager.getDirectoriesSorted(), stager.getFilesSorted());

  return EXIT_SUCCESS;
}

UploadCommand::UploadCommand()
  : cxxsubs::IOptions({"upload"}, "Upload archived or staged files to AWS S3") {
  // clang-format off
  options.add_options()
    ("help", "Print help")
    ("v, verbose", "Enable verbose output")
    ("staged", "Upload staged files")
    ("archived", "Upload archived files")
    (
      "config", "Configuration file to use",
      cxxopts::value<std::filesystem::path>()->default_value("config.json")
    );
  // clang-format on
}
int UploadCommand::validate() {
  if (this->parse_result->count("help"))
    return EXIT_SUCCESS;

  if (!(this->parse_result->count("staged") ||
        this->parse_result->count("archived")))
    throw CommandValidateException(
      "upload command requires at least one of -staged or -archived");
  return EXIT_SUCCESS;
}
int UploadCommand::exec() {
  if (this->parse_result->count("help")) {
    std::cout << this->options.help({""}) << std::endl;
    return EXIT_SUCCESS;
  }

  if (this->parse_result->count("verbose") > 0)
    spdlog::set_level(spdlog::level::info);

  const auto config =
    Config((*this->parse_result)["config"].as<std::filesystem::path>());

  // TODO Upload archived, or staged files
  return EXIT_SUCCESS;
}

DearchiveCommand::DearchiveCommand()
  : cxxsubs::IOptions({"dearchive"}, "Dearchive files") {
  options.positional_help("<paths>").show_positional_help();

  // clang-format off
  options.add_options()
    ("help", "Print help")
    ("v, verbose", "Enable verbose output")
    ("o, output",
      "Specify the output directory to place dearchived files into",
      cxxopts::value<std::filesystem::path>()->default_value("")
    )
    ("paths", "List of paths to dearchive",
      cxxopts::value<std::vector<std::string>>(), "<paths>"
    )
    (
      "config", "Configuration file to use",
      cxxopts::value<std::string>()->default_value("config.json")
    );
  // clang-format on

  options.parse_positional({"paths"});
}
int DearchiveCommand::validate() {
  if (this->parse_result->count("help"))
    return EXIT_SUCCESS;

  if (this->parse_result->count("paths") < 1)
    throw CommandValidateException(
      "dearchive command requires at least one path in <paths>");
  return EXIT_SUCCESS;
}
int DearchiveCommand::exec() {
  if (this->parse_result->count("help")) {
    std::cout << this->options.help({""}) << std::endl;
    return EXIT_SUCCESS;
  }

  if (this->parse_result->count("verbose") > 0)
    spdlog::set_level(spdlog::level::info);

  const auto config =
    Config((*this->parse_result)["config"].as<std::filesystem::path>());

  // TODO dearchive
  return EXIT_SUCCESS;
}