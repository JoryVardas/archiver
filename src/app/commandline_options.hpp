#ifndef ARCHIVER_COMMANDLINE_OPTIONS_HPP
#define ARCHIVER_COMMANDLINE_OPTIONS_HPP

#include <cxxopts.hpp>
#include <filesystem>
#include <string>
#include <vector>

struct CommandlineOptions {
    std::vector<std::filesystem::path> locationsToStage;
    std::vector<std::filesystem::path> locationsToDearchive;
    bool stage;
    bool archive;
    bool dearchive;
    bool uploadStaged;
};

cxxopts::Options registerCommandlineOptions();
CommandlineOptions
parseCommandlineOptions(cxxopts::Options& commandlineOptions,
                        CArray<CString, std::size_t> parameters);

#endif
