#include "commandline_options.hpp"
#include "../config/config.h"
#include <algorithm>
#include <filesystem>
#include <ranges>
#include <spdlog/spdlog.h>

namespace ranges = std::ranges;

StageCommand::StageCommand()
  : cxxsubs::IOptions({"stage"}, "Stage files to be archived") {
  options.positional_help("<paths>").show_positional_help();

  // clang-format off
  options.add_options()
    ("help", "Print help")
    ("v, verbose", "Enable verbose output")
    ("prefix", "Specify a prefix to remove from the paths of files/directories "
      "being staged",
      cxxopts::value<std::string>()->default_value("")
    )
    ("paths", "List of paths to stage",
      cxxopts::value<std::vector<std::string>>(), "<paths>"
    )
    (
      "config", "Configuration file to use",
      cxxopts::value<std::string>()->default_value("config.json")
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
    (*this->parse_result)["paths"].as<std::vector<std::string>>();

  const auto config =
    Config((*this->parse_result)["config"].as<std::filesystem::path>());

  const auto prefix = (*this->parse_result)["prefix"].as<std::string>();

  // TODO stage(paths, prefix)
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
      cxxopts::value<std::string>()->default_value("config.json")
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

  // TODO archive
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
      cxxopts::value<std::string>()->default_value("config.json")
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
      cxxopts::value<std::string>()->default_value("")
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