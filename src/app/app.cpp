#include "../config/config.h"
#include "../database/database.h"
#include "archiver.h"
#include "commandline_options.hpp"
#include "common.h"
#include "util/clone_char_ptr_array.hpp"
#include <cxxsubs.hpp>
#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

int main(int argc, const char* const argv[]) {
  using namespace std::literals::string_literals;

  try {
    spdlog::set_default_logger(spdlog::stdout_color_mt(
      "application_logger"s, spdlog::color_mode::always));

    spdlog::set_level(spdlog::level::err);

    auto mutableArgv = cloneCharPtrArray<char*>(argv, argc);

    auto commands =
      cxxsubs::Verbs<StageCommand, ArchiveCommand, DearchiveCommand,
                     UploadCommand>(argc, mutableArgv.get());

    return EXIT_SUCCESS;

  } catch (const spdlog::spdlog_ex& err) {
    std::cerr << "Log initialization failed: " << err.what() << std::endl;
  } catch (const CommandValidateException& err) {
    std::cerr << err.what() << std::endl;
  } catch (const std::exception& err) {
    spdlog::critical(err.what());
  } catch (...) {
    spdlog::critical("An unknown error occurred");
  }
  return EXIT_FAILURE;
}
