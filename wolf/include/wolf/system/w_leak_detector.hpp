/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#pragma once

#if defined(WIN64) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#endif

#include <wolf/wolf.hpp>

namespace wolf::system {
class w_leak_detector {
 public:
  // default constructor
  W_API w_leak_detector() noexcept
#if defined(WIN64) && defined(_DEBUG)
      : _mem_state()
#endif
  {
#if defined(WIN64) && defined(_DEBUG)
    // take a snapshot from memory
    _CrtMemCheckpoint(&this->_mem_state);
#endif
  }

  // destructor
  W_API virtual ~w_leak_detector() noexcept {
#if defined(WIN64) && defined(_DEBUG)
    try {
      // take a snapshot from memory
      _CrtMemState _diff_mem;
      _CrtMemState _new_mem_state;
      _CrtMemCheckpoint(&_new_mem_state);

      const auto _dif =
          _CrtMemDifference(&_diff_mem, &this->_mem_state, &_new_mem_state);
      if (_dif > 0) {
        _CrtMemDumpStatistics(&_diff_mem);
        _CrtMemDumpAllObjectsSince(&this->_mem_state);
        _CrtDumpMemoryLeaks();
        std::cerr << "Detected memory leak!\r\n";
        assert(false);
        std::terminate();
      }
    } catch (...) {
    }
#endif
  }

  // move constructor
  w_leak_detector(w_leak_detector &&p_src) noexcept = delete;
  // move assignment operator.
  auto operator=(w_leak_detector &&p_src) noexcept
      -> w_leak_detector & = delete;
  // copy constructor.
  w_leak_detector(const w_leak_detector &) = delete;
  // copy assignment operator.
  auto operator=(const w_leak_detector &) -> w_leak_detector & = delete;

 private:
#if defined(WIN64) && defined(_DEBUG)
  _CrtMemState _mem_state;
#endif
};
}  // namespace wolf::system