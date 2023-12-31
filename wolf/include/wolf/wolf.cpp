#include "wolf.hpp"

#ifdef WOLF_SYSTEM_MIMALLOC

#include <mimalloc-new-delete.h>
#include <mimalloc-override.h>

#endif

#ifdef WOLF_SYSTEM_STACKTRACE

#include <csignal>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stacktrace>
#include <system/w_process.hpp>

static void w_signal_handler(int p_signum) {
  auto _path = wolf::system::w_process::current_exe_path();
  if (_path.has_error()) {
    return;
  }

  const auto &_path_str = _path.value().append("wolf.dump");

  std::ignore = signal(p_signum, nullptr);

  std::stringstream _traces;

#ifdef __clang__
#pragma unroll
#endif  // __clang__
  for (const auto &trace : std::stacktrace()) {
    _traces << "name: " << trace.description()
            << " source_file: " << trace.source_file() << "("
            << std::to_string(trace.source_line()) << ")\r\n";
  }

  std::fstream _file;
  _file.open(_path_str.string(), std::ios_base::out);
  if (_file.is_open()) {
    _file << _traces.str();
    _file.close();
  }

  std::ignore = raise(SIGABRT);
}

#endif