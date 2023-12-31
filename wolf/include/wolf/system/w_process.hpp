/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#pragma once

#include <fstream>
#include <wolf/wolf.hpp>

namespace wolf::system {
class w_process {
 public:
  // default constructor
  W_API w_process() noexcept = default;
  // destructor
  W_API virtual ~w_process() noexcept = default;
  // move constructor
  W_API w_process(w_process &&p_src) noexcept = default;
  // move assignment operator.
  W_API auto operator=(w_process &&p_src) noexcept -> w_process & = default;

  // copy constructor.
  w_process(const w_process &) = delete;
  // copy assignment operator.
  auto operator=(const w_process &) -> w_process & = delete;

  /**
   * get current exe path
   * @returns current exe path
   */
  W_API static auto current_path()
      -> boost::leaf::result<std::filesystem::path> {
    auto _path = std::filesystem::current_path();
    return W_SUCCESS(_path);
  }

  /**
   * get current exe path
   * @returns current exe path
   */
  W_API static auto current_exe_path()
      -> boost::leaf::result<std::filesystem::path> {
    using path = std::filesystem::path;

#ifdef WIN64
    std::array<wchar_t, W_MAX_PATH> _buffer = {};
    std::ignore = GetModuleFileNameW(nullptr, _buffer.data(), sizeof(_buffer));
    const auto _ret = GetLastError();

    if (_ret != S_OK) {
      return W_FAILURE(_ret, "GetModuleFileNameW failed because:" +
                                 get_last_win_error(_ret));
    }

    auto _path = path(_buffer.data(), path::auto_format).parent_path();
    return W_SUCCESS(_path);
#else
    std::stringstream buffer;
    buffer << std::ifstream("/proc/self/exe").rdbuf();
    return std::filesystem::path(buffer.str());
#endif
  }
};
}  // namespace wolf::system
