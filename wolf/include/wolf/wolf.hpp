/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#ifndef _WOLF_HPP_
#define _WOLF_HPP_

#pragma once

#ifdef __SANITIZE_ADDRESS__
#ifndef _DISABLE_VECTOR_ANNOTATION
#define _DISABLE_VECTOR_ANNOTATION
#endif
#endif

#ifdef _MSC_VER
#include <codeanalysis/warnings.h>
#pragma warning( \
    disable : 26408)  // disable warning for using malloc() and free()
#pragma warning(push)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif

// NOLINTBEGIN

#ifdef WIN64
#include <Windows.h>
#endif

#ifndef EMSCRIPTEN
#include <filesystem>
#endif

#if defined(WOLF_SYSTEM_SOCKET) || defined(WOLF_SYSTEM_HTTP_WS)
#include <boost/asio.hpp>
#endif

#ifdef WOLF_SYSTEM_COROUTINE
#include <boost/asio/experimental/awaitable_operators.hpp>
using namespace boost::asio::experimental::awaitable_operators;
#endif

#include <boost/leaf.hpp>
#include <chrono>
#include <exception>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifdef _MSC_VER
#include <format>
#else
#include <fmt/chrono.h>
#include <fmt/format.h>
#endif

#include <gsl/gsl>

// NOLINTEND

#ifdef _MSC_VER
#pragma warning(pop)
#endif

constexpr auto W_MAX_PATH = 260;
constexpr auto W_MAX_BUFFER_SIZE = 1024;

#define DEFER auto _ = std::shared_ptr<void>(nullptr, [&](...)

struct wolf_buffer {
    wolf_buffer() noexcept = default;

  explicit wolf_buffer(const std::string_view p_str) { from_string(p_str); }

  wolf_buffer(std::array<char, W_MAX_BUFFER_SIZE> &&p_array,
           const size_t p_used_bytes) noexcept {
    this->buf = std::move(p_array);
    this->used_bytes = p_used_bytes;
  }

  void from_string(const std::string_view p_str) {
    const auto _size = p_str.size();
    this->used_bytes = _size > 1024 ? 1024 : _size;
    std::copy(p_str.cbegin(), p_str.cbegin() + this->used_bytes, buf.begin());
  }

  std::string to_string() {
    if (this->buf.size() && this->used_bytes) {
      return std::string(this->buf.data(), this->used_bytes);
    }
    return std::string();
  }

  std::array<char, W_MAX_BUFFER_SIZE> buf = {0};
  size_t used_bytes = 0;
};

// #ifdef __clang__
// #define W_ALIGNMENT_16 __attribute__((packed)) __attribute__((aligned(16)))
// #define W_ALIGNMENT_32 __attribute__((packed)) __attribute__((aligned(32)))
// #define W_ALIGNMENT_64 __attribute__((packed)) __attribute__((aligned(64)))
// #define W_ALIGNMENT_128 __attribute__((packed)) __attribute__((aligned(128)))
// #else
// #define W_ALIGNMENT_16
// #define W_ALIGNMENT_32
// #define W_ALIGNMENT_64
// #define W_ALIGNMENT_128
// #endif

#ifdef _MSC_VER

#define W_API __declspec(dllexport)
#define ASM __asm

#else

#define W_API
#define ASM __asm__

// define dummy SAL
#define _In_
#define _In_z_
#define _Out_
#define _Inout_
#define _Inout_z_
#define _In_opt_
#define _In_opt_z_
#define _Inout_opt_

#endif

#include <wolf/system/invocable.hpp>
#include <wolf/system/w_trace.hpp>

namespace wolf {
template <class T>
using w_function = ofats::any_invocable<T>;
using w_binary = std::vector<std::byte>;

/**
 * make a string via format
 * @param p_fmt, the fmt
 * @param p_args, the args
 * @return a string
 */
#ifdef _MSC_VER
using std::format;
#else
using fmt::format;
#endif  // _MSC_VER

/**
 * returns wolf version
 * @return string format with the following style
 * "<major>.<minor>.<patch>.<debug>"
 */
W_API inline std::string w_init() {
#if defined(WOLF_SYSTEM_STACKTRACE) && !defined(WOLF_TESTS)
  std::ignore = signal(SIGSEGV, &w_signal_handler);
  std::ignore = signal(SIGABRT, &w_signal_handler);
#endif

  // Making incompatible API changes
  constexpr auto _WOLF_MAJOR_VERSION = 3;
  // Adding functionality in a backwards - compatible manner
  constexpr auto _WOLF_MINOR_VERSION = 0;
  // bug fixes
  constexpr auto _WOLF_PATCH_VERSION = 0;
  // for debugging
  constexpr auto _WOLF_DEBUG_VERSION = 0;

#ifdef _MSC_VER
#pragma warning(disable : 26481)  // disable warning for use span instead
                                  // (bounds.1) for std::format
#pragma warning(push)
#endif
  // NOLINTBEGIN
  auto _version = format("%d.%d.%d.%d", _WOLF_MAJOR_VERSION, _WOLF_MINOR_VERSION,
                         _WOLF_PATCH_VERSION, _WOLF_DEBUG_VERSION);
  // NOLINTEND
#ifdef _MSC_VER
#pragma warning(pop)
#endif

  return _version;
}

/*
 * get environment variable
 * @param p_env, the env name
 */
W_API inline std::string get_env(const std::string_view p_env) {
  std::string val;
  if (p_env.empty()) {
    return val;
  }

#ifdef _MSC_VER
  char *buf = nullptr;
  size_t size = 0;
  if (_dupenv_s(&buf, &size, p_env.data()) == 0 && buf != nullptr) {
    val = buf;
    free(buf);
  }
#else
  const auto buf = std::getenv(p_env.data());
  if (buf != nullptr) {
    val = buf;
  }
#endif

  return val;
}

/*
 * get wolf content path
 * @param p_subpath, the subpath of content folder
 */
W_API inline std::filesystem::path get_content_path(
    const std::string_view p_subpath) {
  constexpr const char *ENVKEY_CONTENT_PATH = "WOLF_CONTENT_PATH";
  auto env_content_path = wolf::get_env(ENVKEY_CONTENT_PATH);
  return env_content_path.empty()
             ? std::filesystem::current_path().append(p_subpath)
             : std::filesystem::path(env_content_path).append(p_subpath);
}
}  // namespace wolf

#endif  // _WOLF_HPP_
