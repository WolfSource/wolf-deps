/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/
#if !defined(__APPLE__) && !defined(ANDROID) && !defined(EMSCRIPTEN)

#pragma once

#include <wolf/wolf.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif

// NOLINTBEGIN
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/asio/steady_timer.hpp>
// NOLINTEND

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace wolf::system {

class w_timer : public boost::asio::high_resolution_timer {
 public:
  W_API explicit w_timer(boost::asio::io_context &p_io_context)
      : boost::asio::high_resolution_timer(p_io_context, 0) {}
};

#ifdef WOLF_SYSTEM_COROUTINE
class w_time {
 public:
  static boost::asio::awaitable<std::errc> timeout(
      _In_ const std::chrono::steady_clock::time_point &p_deadline) noexcept {
    using steady_timer = boost::asio::steady_timer;
    using steady_clock = std::chrono::steady_clock;

    steady_timer _timer(co_await boost::asio::this_coro::executor);
    auto _now = steady_clock::now();

#ifdef __clang__
#pragma unroll
#endif
    while (p_deadline > _now) {
      _timer.expires_at(p_deadline);
      co_await _timer.async_wait(boost::asio::use_awaitable);
      _now = steady_clock::now();
    }
    co_return std::errc::timed_out;
  }
};
#endif

}  // namespace wolf::system

#endif  // !__APPLE__ && !ANDROID && !EMSCRIPTEN