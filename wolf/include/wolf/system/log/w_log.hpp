/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#pragma once

#ifdef WOLF_SYSTEM_LOG

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif
// NOLINTBEGIN

#include <wolf/wolf.hpp>

#include "w_log_config.hpp"

#ifdef _MSC_VER
#include <spdlog/sinks/msvc_sink.h>
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
// NOLINTEND

namespace wolf::system::log {
class w_log {
 public:
  // constructor
  W_API explicit w_log(w_log_config &&p_config) noexcept
      : _config(std::move(p_config)) {}

  // destructor
  W_API virtual ~w_log() { flush(); }

  // move constructor.
  W_API w_log(w_log &&p_other) noexcept { _move(p_other); }
  // move assignment operator.
  W_API auto operator=(w_log &&p_other) noexcept -> w_log & {
    _move(p_other);
    return *this;
  }

  // disable copy constructor
  w_log(const w_log &) = delete;
  // disable copy operator
  auto operator=(const w_log &) -> w_log & = delete;

  W_API auto init() -> boost::leaf::result<int> {
    // first create a directory for log
    const auto parent_path = this->_config.path.parent_path();
    const auto is_dir = std::filesystem::is_directory(parent_path);
    if (!is_dir) {
      // try to create a directory for it
      try {
        const auto created = std::filesystem::create_directory(parent_path);
        if (!created) {
          return W_FAILURE(std::errc::invalid_argument,
                           "could not create log directory path");
        }
      } catch (std::exception &p_ex) {
        return W_FAILURE(
            std::errc::operation_canceled,
            wolf::format("could not create log directory path because {}",
                         p_ex.what()));
      }
    }

    // create sinks
    std::vector<spdlog::sink_ptr> sinks;
    if ((this->_config.type & w_log_sink::CONSOLE) != 0) {
      if (this->_config.multi_threaded) {
        sinks.push_back(
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
      } else {
        sinks.push_back(
            std::make_shared<spdlog::sinks::stdout_color_sink_st>());
      }
    }

#if defined(_MSC_VER)
    if (this->_config.type & w_log_sink::VISUAL_STUDIO) {
      if (this->_config.multi_threaded) {
        sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
      } else {
        sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_st>());
      }
    }
#endif

    const auto level = this->_config.level;
    const auto flush_level = this->_config.flush_level;

    const auto filename = this->_config.path.filename().string();
    const auto path = this->_config.path.string();

    if ((this->_config.type & w_log_sink::ASYNC_FILE) != 0) {
      // async file sink
      this->_async_file_logger =
          spdlog::create_async_nb<spdlog::sinks::rotating_file_sink_mt>(
              filename, path, this->_config.max_file_size_in_mb,
              this->_config.max_files, this->_config.rotate_on_open);
      if (this->_async_file_logger == nullptr) {
        return W_FAILURE(std::errc::invalid_argument,
                         "could not allocate memory for async file logger");
      }

      this->_async_file_logger->set_level(level);
      this->_async_file_logger->flush_on(flush_level);
    } else if ((this->_config.type & w_log_sink::ASYNC_DAILY_FILE) != 0) {
      // async daily file sink
      this->_async_file_logger =
          spdlog::create_async_nb<spdlog::sinks::daily_file_sink_mt>(
              filename, path, this->_config.hour, this->_config.minute,
              false,  // truncate
              gsl::narrow_cast<uint16_t>(this->_config.max_files));
      if (this->_async_file_logger == nullptr) {
        return W_FAILURE(
            std::errc::invalid_argument,
            "could not allocate memory for daily async file logger");
      }

      this->_async_file_logger->set_level(level);
      this->_async_file_logger->flush_on(flush_level);
    }

    // create logger for other sinks
    this->_logger =
        std::make_shared<spdlog::logger>(filename, sinks.begin(), sinks.end());
    if (this->_logger == nullptr) {
      return W_FAILURE(std::errc::invalid_argument, "could not create logger");
    }

    this->_logger->set_level(level);
    this->_logger->flush_on(flush_level);

    return 0;
  }

  W_API void write(_In_ const spdlog::level::level_enum &p_level,
                   _In_ const std::string_view &p_fmt) {
    if (this->_logger != nullptr) {
      this->_logger->log(p_level, p_fmt);
    }
    if (this->_async_file_logger != nullptr) {
      this->_async_file_logger->log(p_level, p_fmt);
    }
  }

  W_API void write(_In_ const std::string_view &p_fmt) {
    write(spdlog::level::level_enum::info, p_fmt);
  }

  // NOLINTBEGIN(cppcoreguidelines-missing-std-forward)
#ifdef _MSC_VER

  template <class... Args>
  W_API void write(_In_ const std::string_view p_fmt, _In_ Args &&...p_args) {
    const auto _str = std::vformat(p_fmt, std::make_format_args(p_args...));
    write(_str);
  }

  template <class... Args>
  W_API void write(_In_ const spdlog::level::level_enum &p_level,
                   _In_ const std::string_view p_fmt, _In_ Args &&...p_args) {
    const auto _str = std::vformat(p_fmt, std::make_format_args(p_args...));
    write(p_level, _str);
  }

#else
  template <class... Args>
  W_API void write(_In_ const fmt::format_string<Args...> p_fmt,
                   _In_ Args &&...p_args) {
    const auto _str = fmt::vformat(p_fmt, fmt::make_format_args(p_args...));
    write(_str);
  }

  template <class... Args>
  W_API void write(_In_ const spdlog::level::level_enum &p_level,
                   _In_ const fmt::format_string<Args...> p_fmt,
                   _In_ Args &&...p_args) {
    const auto _str = fmt::vformat(p_fmt, fmt::make_format_args(p_args...));
    write(p_level, _str);
  }

#endif  // _MSC_VER
  // NOLINTEND(cppcoreguidelines-missing-std-forward)

  W_API void flush() {
    if (this->_logger != nullptr) {
      this->_logger->flush();
    }
    if (this->_async_file_logger != nullptr) {
      this->_async_file_logger->flush();
    }
  }

  W_API [[nodiscard]] auto get_config() const -> w_log_config {
    return this->_config;
  }

 private:
  W_API void _move(_Inout_ w_log &p_other) noexcept {
    if (this == &p_other) {
      return;
    }
    // move to new object
    this->_config = std::move(p_other._config);
    this->_logger = std::exchange(p_other._logger, nullptr);
    this->_async_file_logger =
        std::exchange(p_other._async_file_logger, nullptr);

    // reset the other object
    p_other._config = {};
  }

  w_log_config _config = {};
  std::shared_ptr<spdlog::logger> _logger = nullptr;
  std::shared_ptr<spdlog::logger> _async_file_logger = nullptr;
};
}  // namespace wolf::system::log

#endif
