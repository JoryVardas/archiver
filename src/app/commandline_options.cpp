#include "commandline_options.hpp
#include "common.h"
#include <algorithm>
#include <filesystem>
#include <ranges>

cxxopts::Options registerCommandlineOptions() {
    cxxopts::Options options("Archiver",
                             "Custom archival solution for large "
                             "amounts of highly compressible files.");
    options.add_options()                                //
        ("archive", "Archive staged files/directories.") //
        ("stage", "Prepare the specified files/directories for archival.",
         cxxopts::value<std::vector<std::string>>()) //
        ("dearchive", "Dearchive the specified files/directories.",
         cxxopts::value<std::vector<std::string>>()) //
        ("verbose", "Enable verbose output.",
         cxxopts::value<bool>()->default_value("false")) //
        ("prefix",
         "Specify a prefix to remove from the paths of files/directories "
         "being staged.",
         cxxopts::value<std::string>()->default_value("")) //
        ("upload", "Upload to staged files to off site file storage.",
         cxxopts::value<bool>()->default_value("false")) //
                                                         /*("base",
                                                          "Specify where the files/directories should be placed in the archive "
                                                          "structure.",
                                                          cxxopts::value<std::string>()->default_value("/"))*/
        ;
};

void parseLocationsToStage(auto parseOptions,
                           CommandlineOptions& commandlineOptions) {
    auto logger = spdlog::get(applicationLoggerName);
    const auto locationPrefix = parsedOptions["prefix"].as<std::string>();
    // While std::filesystem::path can be directly parsed, using std::string
    // prevents having to later cast to a std::string in order to remove the
    // prefix.
    const auto locationStrings =
        parsedOptions["stage"].as<std::vector<std::string>>();
    const auto toFilesystemPath = [](const auto& location) {
        return std::filesystem::path(location);
    };
    const auto removeLocationPrefix = [&locationPrefix,
                                       &logger](const auto& location) {
        std::string_view projection{location};
        if (location.starts_with(locationPrefix)) {
            projection.remove_prefix(std::size(locationPrefix));
        } else {
            logger->warning("The path \"{}\" did not contain the prefix "
                            "\"{}\", and as such the full path will be used.",
                            location, locationPrefix);
        }
        return projection;
    };
    std::ranges::transform(
        locationStrings,
        std::back_inserter(commandlineOptions.locationsToStage),
        toFilesystemPath, removeLocationPrefix);
}
void parseLocationsToDearchive(auto parseOptions,
                               CommandlineOptions& commandlineOptions) {
    commandlineOptions.locationsToDearchive =
        parsedOptions["dearchive"].as<std::vector<std::filesystem::path>>();
}

CommandlineOptions
parseCommandlineOptions(cxxopts::Options& commandlineOptions,
                        CArray<CString, std::size_t> parameters) {
    auto parsedOptions = options.parse(parameters.size(), parameters.rawData());
    CommandlineOptions commandlineOptions;
    if (parsedOptions.count("stage")) {
        commandlineOptions.parse = true;
        parseLocationsToStage(parsedOptions, commandlineOptions);
    }
    commandlineOptions.archive = parsedOptions.count("archive") > 0;
    if (parsedOptions.count("dearchive")) {
        commandlineOptions.dearchive = true;
        parseLocationsToDearchive(parsedOptions, commandlineOptions);
    }
    commandlineOptions.uploadStaged = parsedOptions.count("upload") > 0;

    auto logger = spdlog::get(applicationLoggerName);
    if (commandlineOptions.upload && !commandlineOptions.stage) {
        logger->warning("--upload flag is only valid if --stage is used, as "
                        "such it will be ignored.");
        commandlineOptions.upload = false;
    }
};