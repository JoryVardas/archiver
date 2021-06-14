#include "../config/config.h"
#include "../database/database.h"
#include "archiver.h"
#include "common.h"
#include <cxxopts.hpp>
#include <iostream>
#include <spdlog/spdlog-inl.h>
#include <string>
#include <vector>

auto parseParameters(CArray<CString, int> parameters) {
    cxxopts::Options options("Archiver",
                             "Custom archival solution for large "
                             "amounts of highly compressible files.");
    options.add_options()("A,archive",
                          "Archive the specified files/directories.",
                          cxxopts::value<std::vector<std::string>>())(
        "D,dearchive", "Dearchive the specified files/directories.",
        cxxopts::value<std::vector<
            std::string>>())("v,verbose", "Enable verbose output.",
                             cxxopts::value<bool>()->default_value(
                                 "false"))("p,prefix",
                                           "Specify a prefix to remove from "
                                           "the paths of files/directories "
                                           "being "
                                           "archived.",
                                           cxxopts::value<std::string>()
                                               ->default_value(""));
    return options.parse(parameters.size(), parameters.rawData());
};

int main(int argc, const char* argv[]) {
    auto arguments = parseParameters(CArray{argv, argc});

    std::shared_ptr<Config> defaultConfig =
        std::make_shared<Config>("settings.json"_path);
    // Database database{DatabaseConnectionParameters{
    //     defaultConfig.database.location.host,
    //     defaultConfig.database.location.port,
    //     defaultConfig.database.user,
    //     defaultConfig.database.password,
    //     defaultConfig.database.location.schema,
    // }};

    if (arguments.count("archive")) {
        // Archiver archiver(database);

        auto archiveLocations =
            arguments["archive"].as<std::vector<std::filesystem::path>>();
        // for (auto location : archiveLocations) {
        //     if (arguments.count("prefix")) {
        //         auto prefix =
        //         arguments["prefix"].as<std::filesystem::path>(); if
        //         (location.starts_with(prefix)) {
        //             location.erase(0, prefix.length());
        //         }
        //     }
        //     archiver.addPathToArchive(std::filesystem::path(
        //         location, std::filesystem::path::format::generic_format));
        // }

        // archiver.archive();
    };
    if (arguments.count("dearchive") > 0) {
    };

    return EXIT_SUCCESS;
}
