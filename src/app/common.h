#ifndef COMMON_H
#define COMMON_H

#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unfmt.h>
#include <vector>

// Neither Clang nor GCC have standard libraries which support the <format>
// library, so use fmtlib which implements it.
#if __has_include(<format>)
#include <format>
// The FMT library is no longer needed now that there is a native <format>
// library.
//  Please update the usage from fmt::format and fmt::formatter to std::format
//  and std::formatter.
#define fmt std
#else
#define FMT_HEADER_ONLY
#include <fmt/format.h>
#endif

using CString = const char*;
using ID = uint64_t;
using Size = uint64_t;
using Extension = std::string;
using TimeStamp = std::chrono::time_point<std::chrono::system_clock>;

template <typename T, typename Size_T> struct CArray {
    CArray(T* data, Size_T size) : _data(data), _size(size) {}

    Size_T size() { return _size; }
    T* rawData() { return _data; }

    T& operator[](Size_T index) {
        if (index >= _size)
            throw std::out_of_range(
                "Index into CArray is outside the size of the CArray.");

        return _data[index];
    }

  private:
    T* _data;
    Size_T _size;
};

#define _make_formatter_for_exception_(type)                                   \
    template <typename CharT>                                                  \
    struct fmt::formatter<type, CharT>                                         \
        : public fmt::formatter<const char*, CharT> {                          \
        template <typename FormatContext>                                      \
        auto format(const type& err, FormatContext& fc) {                      \
            return fmt::formatter<const char*, CharT>::format(err.what(), fc); \
        };                                                                     \
    }

#define _make_exception_(name)                                                 \
    struct name : public std::runtime_error {                                  \
        name(const std::string& msg) : std::runtime_error(msg){};              \
    };                                                                         \
    _make_formatter_for_exception_(name)

bool isDirectory(const std::string& path);
bool isFile(const std::string& path);
bool pathExists(const std::string& path);

std::string toString(const TimeStamp& timestamp);
TimeStamp toTimestamp(const std::string& str);

#define _log_ std::cout
#define abstract = 0

using namespace std::string_literals;

_make_formatter_for_exception_(std::exception);

template <typename CharT>
struct fmt::formatter<std::filesystem::path, CharT>
    : public fmt::formatter<std::string, CharT> {
    template <typename FormatContext>
    auto format(const std::filesystem::path& path, FormatContext& fc) {
        return fmt::formatter<std::string, CharT>::format(path.string(), fc);
    };
};

std::filesystem::path operator"" _path(const char* str, std::size_t) {
    return std::filesystem::path{str};
};
std::filesystem::path operator"" _path_generic(const char* str, std::size_t) {
    return std::filesystem::path{str,
                                 std::filesystem::path::format::generic_format};
};
std::filesystem::path operator"" _path_native(const char* str, std::size_t) {
    return std::filesystem::path{str,
                                 std::filesystem::path::format::native_format};
};

#endif