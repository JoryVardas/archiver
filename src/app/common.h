#ifndef COMMON_H
#define COMMON_H

#define SPDLOG_FMT_EXTERNAL

// Neither Clang nor GCC have standard libraries which support the <format>
// library, so use fmtlib which implements it.
#if __has_include(<format>)
#include <format>
// Clang has a <format> hearder but the contents are incomplete
#if defined(_LIBCPP_VERSION)
#if !defined(_LIBCPP_HAS_NO_INCOMPLETE_FORMAT)
// The FMT library is no longer needed now that there is a native <format>
// library.
//  Please update the usage from fmt::format and fmt::formatter to std::format
//  and std::formatter.
#define ARCHIVER_USING_STD_FORMAT
#endif
#else
#define ARCHIVER_USING_STD_FORMAT
#endif
#endif
#ifdef ARCHIVER_USING_STD_FORMAT
#define FORMAT_LIB std
#define SPDLOG_USE_STD_FORMAT
#else
// #define FMT_HEADER_ONLY
#include <fmt/format.h>

#include <fmt/chrono.h>

#define FORMAT_LIB fmt
#endif

#include <filesystem>
#include <iostream>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <vector>

using ID = uint64_t;
using Size = uint64_t;
using Extension = std::string;
using TimeStamp = std::chrono::time_point<std::chrono::system_clock>;

#define _make_formatter_for_exception_(type)                                   \
  template <typename CharT>                                                    \
  struct FORMAT_LIB::formatter<type, CharT>                                    \
    : public FORMAT_LIB::formatter<const char*, CharT> {                       \
    template <typename FormatContext>                                          \
    auto format(const type& err, FormatContext& fc) {                          \
      return FORMAT_LIB::formatter<const char*, CharT>::format(err.what(),     \
                                                               fc);            \
    };                                                                         \
  }

#define _make_exception_(name)                                                 \
  struct name : public std::runtime_error {                                    \
    name(const std::string& msg) : std::runtime_error(msg){};                  \
    template <typename T, typename... Ts>                                      \
    name(const std::string& formattedMessage, T&& arg, Ts&&... args)           \
      : std::runtime_error(FORMAT_LIB::vformat(                                \
          formattedMessage,                                                    \
          FORMAT_LIB::make_format_args(std::forward<T>(arg),                   \
                                       std::forward<Ts>(args)...))){};         \
  };                                                                           \
  _make_formatter_for_exception_(name)

_make_formatter_for_exception_(std::exception);

template <typename CharT>
struct FORMAT_LIB::formatter<std::filesystem::path, CharT>
  : public FORMAT_LIB::formatter<std::string, CharT> {
  template <typename FormatContext>
  auto format(const std::filesystem::path& path, FormatContext& fc) {
    return FORMAT_LIB::formatter<std::string, CharT>::format(path.string(), fc);
  };
};

// Format vectors, the first character in the format specifier is the separator.
template <typename T, typename CharT>
struct FORMAT_LIB::formatter<std::vector<T>, CharT>
  : public FORMAT_LIB::formatter<T, CharT> {

  constexpr auto parse(FORMAT_LIB::format_parse_context& ctx) {
    // Get the separator
    auto it = std::begin(ctx);
    if (it == std::end(ctx) || *it == '}')
      return it;
    separator = *std::begin(ctx);
    ctx.advance_to(std::begin(ctx) + 1);

    // Pass the rest of the format string to formatter for the individual type.
    return FORMAT_LIB::formatter<T, CharT>::parse(ctx);
  }

  template <typename FormatContext>
  auto format(const std::vector<T>& vector, FormatContext& fc) {
    auto outputIterator = fc.out();
    *outputIterator++ = '{';

    auto it = std::begin(vector);
    auto end = std::end(vector);
    if (it != end) {
      FORMAT_LIB::formatter<T, CharT>::format(*it, fc);
      for (++it; it != std::end(vector); ++it) {
        *outputIterator++ = separator;
        *outputIterator++ = ' ';
        FORMAT_LIB::formatter<T, CharT>::format(*it, fc);
      }
    }

    *outputIterator++ = '}';
    return outputIterator;
  };

private:
  char separator;
};

#define abstract  = 0
#define interface struct

#endif