#include "common.h"

bool isDirectory(const std::string& path) {
    return std::filesystem::is_directory(std::filesystem::status(path));
};
bool isFile(const std::string& path) {
    return std::filesystem::is_regular_file(std::filesystem::status(path));
};
bool pathExists(const std::string& path) {
    return std::filesystem::exists(std::filesystem::status(path));
};

std::string toString(const TimeStamp& timestamp) {
    auto timestampAsMilliseconds =
        std::chrono::time_point_cast<std::chrono::milliseconds>(timestamp);
    auto timestampAsSeconds =
        std::chrono::time_point_cast<std::chrono::seconds>(timestamp);
    auto miliseconds = timestampAsMilliseconds - timestampAsSeconds;
    return fmt::format("{:%Y-%m-%e-%H:%M:%S}.{}", timestamp,
                       miliseconds.count());
};
TimeStamp toTimestamp(const std::string& str){

};