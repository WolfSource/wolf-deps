/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/
#if defined(WOLF_SYSTEM_GAMEPAD_VIRTUAL) && !defined(EMSCRIPTEN)

#pragma once

#include <shared_mutex>

#include "w_gamepad_virtual.hpp"

namespace wolf::system::gamepad {
struct w_gamepad_virtual_bus {
  std::shared_mutex mutex = {};
  PVIGEM_CLIENT driver_handle = nullptr;
};
}  // namespace wolf::system::gamepad

#endif  // defined(WOLF_SYSTEM_VIRTUAL_GAMEPAD) && !defined(EMSCRIPTEN)
