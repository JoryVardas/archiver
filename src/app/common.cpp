#include "common.h"

bool isDirectory(const std::filesystem::path& path) {
  return std::filesystem::is_directory(std::filesystem::status(path));
};
bool isFile(const std::filesystem::path& path) {
  return std::filesystem::is_regular_file(std::filesystem::status(path));
};
bool pathExists(const std::filesystem::path& path) {
  return std::filesystem::exists(std::filesystem::status(path));
};