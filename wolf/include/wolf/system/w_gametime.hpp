/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#pragma once

#include <cmath>
#include <wolf/wolf.hpp>

namespace wolf::system {
constexpr auto MAX_DELTA_TIME_IN_SECS = 60000.0;  // 60 minutes
constexpr auto TARGET_ELAPSED_SECS = 1.0 / 60.0;

class w_gametime {
 public:
  // default constructor
  W_API w_gametime() noexcept = default;

  // destructor
  W_API virtual ~w_gametime() noexcept = default;

  // move constructor
  W_API w_gametime(w_gametime &&p_src) noexcept = default;

  // move assignment operator.
  W_API auto operator=(w_gametime &&p_src) noexcept -> w_gametime & = default;

  // copy constructor.
  w_gametime(const w_gametime &) = delete;
  // copy assignment operator.
  auto operator=(const w_gametime &) -> w_gametime & = delete;

  // reset the gametime
  W_API void reset() noexcept {
    this->_last_time = std::chrono::steady_clock::now();
    this->_left_over_ticks = 0.0;
    this->_fps = 0;
    this->_frames_this_sec = 0;
    this->_secs_counter = 0.0;
  }

  [[nodiscard]] W_API auto get_elapsed_secs() const noexcept -> double {
    return this->_elapsed_secs;
  }

  [[nodiscard]] W_API auto get_total_secs() const noexcept -> double {
    return this->_total_secs;
  }

  [[nodiscard]] W_API auto get_frames_count() const noexcept -> uint32_t {
    return this->_frames_count;
  }

  [[nodiscard]] W_API auto get_fps() const noexcept -> uint32_t {
    return this->_fps;
  }

  [[nodiscard]] W_API auto get_is_fixed_time_step() const noexcept -> bool {
    return this->_fixed_time_step;
  }

  W_API void set_fixed_time_step(bool p_value) noexcept {
    this->_fixed_time_step = p_value;
  }

  W_API void set_target_elapsed_secs(double p_value) noexcept {
    this->_target_elapsed_secs = p_value;
  }

  template <typename F>
  // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
  W_API void tick(F &&p_tick_function) {
    using steady_clock = std::chrono::steady_clock;

    // Query the current time.
    const auto current_time = steady_clock::now();
    constexpr auto milli_in_sec = 1000.000;
    auto delta = std::chrono::duration<double, std::milli>(current_time -
                                                           this->_last_time)
                     .count() /
                 milli_in_sec;

    this->_last_time = current_time;
    this->_secs_counter += delta;

    // clamp excessively large time deltas (e.g. after paused in the debugger).
    if (delta > MAX_DELTA_TIME_IN_SECS) {
      delta = MAX_DELTA_TIME_IN_SECS;
    }

    const auto last_frames_count = this->_frames_count;
    if (this->_fixed_time_step) {
      /*
         If the app is running very close to the target elapsed time (within
         1/4 of a millisecond (400 microseconds)) just clamp the clock to
         exactly match the target value. This prevents tiny and irrelevant
         errors from accumulating over time. Without this clamping, a game that
         requested a 60 fps fixed update, running with vsync enabled on a 59.94
         NTSC display, would eventually accumulate enough tiny errors that it
         would drop a frame. It is better to just round small deviations down to
         zero to leave things running smoothly.
      */
      constexpr auto quarter_of_one_milli_in_sec = 0.0004;
      if (std::abs(delta - this->_target_elapsed_secs) <
          quarter_of_one_milli_in_sec) {
        delta = this->_target_elapsed_secs;
      }

      this->_left_over_ticks += delta;
      this->_elapsed_secs = this->_target_elapsed_secs;

#ifdef __clang__
#pragma unroll
#endif
      while (this->_left_over_ticks >= this->_target_elapsed_secs) {
        this->_total_secs += this->_target_elapsed_secs;
        this->_left_over_ticks -= this->_target_elapsed_secs;

        this->_frames_count++;
        p_tick_function();
      }
    } else {
      // variable timestep update logic.
      this->_left_over_ticks = 0.0;
      this->_total_secs += delta;
      this->_elapsed_secs = delta;
      this->_frames_count++;
      p_tick_function();
    }

    // track the current framerate.
    this->_frames_this_sec += (this->_frames_count - last_frames_count);

    auto one_sec = 1.0;
    if (this->_secs_counter >= one_sec) {
      this->_fps = this->_frames_this_sec;
      this->_frames_this_sec = 0;
      this->_secs_counter = std::modf(this->_secs_counter, &one_sec);
    }
  }

  W_API void tick() {
    tick([] {});
  }

 private:
  // configuring fixed timestep mode.
  bool _fixed_time_step = false;

  double _target_elapsed_secs = {TARGET_ELAPSED_SECS};
  double _elapsed_secs = 0.0;
  double _total_secs = 0.0;
  double _left_over_ticks = 0.0;
  double _secs_counter = 0.0;
  uint32_t _fps = 0;
  uint32_t _frames_count = 0;
  uint32_t _frames_this_sec = 0;

  std::chrono::steady_clock::time_point _last_time = {
      std::chrono::steady_clock::now()};
};
}  // namespace wolf::system