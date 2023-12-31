/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#pragma once

#ifdef WOLF_SYSTEM_LOG

#include <spdlog/spdlog.h>

#include <filesystem>
#include <wolf/wolf.hpp>
namespace wolf::system::log {

// create a log with a combination of console, telemetry, visual studio and
// (async file/async daily file) please note that you can't select both async
// file and async daily file
enum w_log_sink {
  CONSOLE = (1U << 0),
  ASYNC_FILE = (1U << 1),
  ASYNC_DAILY_FILE = (1U << 2),
// TELEMETRY = (1u << 3),
#ifdef _MSC_VER
  VISUAL_STUDIO = (1u << 4),
#endif
};

inline auto operator|(w_log_sink p_left, w_log_sink p_right) -> w_log_sink {
  return static_cast<w_log_sink>(static_cast<int>(p_left) |
                                 static_cast<int>(p_right));
}

// constant number for max number of files
constexpr auto const_max_files = 10;
// constant value for max file size in Mb
constexpr auto const_max_file_size_in_mb = 100 * 1048576;

struct w_log_config {
  // create an async logger
  bool async = false;
  // enable multi-threaded
  bool multi_threaded = false;
  // the path of log file
  std::filesystem::path path;
  // the log level
  spdlog::level::level_enum level;
  // the flush level
  spdlog::level::level_enum flush_level;
  // the sinks of log
  w_log_sink type;
  // max file size in Mb (e.g. maximum 100 log files * with size of 100 Mb
  // file)
  size_t max_file_size_in_mb = const_max_file_size_in_mb;
  // max number of files
  size_t max_files = const_max_files;
  // rotate file on open
  bool rotate_on_open = false;
  // start creation time for daily log
  int hour = 0;
  // start creation time for daily log
  int minute = 0;
};
}  // namespace wolf::system::log

#endif