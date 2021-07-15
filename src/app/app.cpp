#include "../config/config.h"
#include "../database/database.h"
#include "archiver.h"
#include "commandline_options.hpp"
#include "common.h"
#include <cxxopts.hpp>
#include <iostream>
#include <spdlog/spdlog-inl.h>
#include <string>
#include <vector>

int main(int argc, const char* argv[]) {
    auto registeredOptions = registerCommandlineOptions();
    auto options = parseCommandlineOptions(
        registeredOptions, CArray{argv, static_cast<std::size_t>(argc)});

    std::shared_ptr<Config> defaultConfig =
        std::make_shared<Config>("settings.json"_path);

    return EXIT_SUCCESS;
}
