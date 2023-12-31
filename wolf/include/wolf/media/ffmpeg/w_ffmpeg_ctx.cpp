#ifdef WOLF_MEDIA_FFMPEG

#include "w_ffmpeg_ctx.hpp"

using w_ffmpeg_ctx = wolf::media::ffmpeg::w_ffmpeg_ctx;

std::string w_ffmpeg_ctx::get_av_error_str(_In_ int p_error_code) noexcept {
  std::array<char, W_MAX_PATH> _error[] = {'\0'};
  try {
    std::ignore =
        av_make_error_string(_error->data(), W_MAX_PATH, p_error_code);
    return std::string(_error->data());
  } catch (...) {
    return std::string();
  }
}

#endif  // WOLF_MEDIA_FFMPEG