/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#ifdef WOLF_MEDIA_FFMPEG

#pragma once

#include <wolf.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace wolf::media::ffmpeg {

class w_ffmpeg_ctx {
 public:
  // constructor
  W_API w_ffmpeg_ctx() = default;
  // destructor
  W_API virtual ~w_ffmpeg_ctx() noexcept { release(); }

  // move constructor.
  W_API w_ffmpeg_ctx(w_ffmpeg_ctx &&p_other) noexcept {
    move(std::forward<w_ffmpeg_ctx &&>(p_other));
  }
  // move assignment operator.
  W_API w_ffmpeg_ctx &operator=(w_ffmpeg_ctx &&p_other) noexcept {
    move(std::forward<w_ffmpeg_ctx &&>(p_other));
    return *this;
  }

  W_API static std::string get_av_error_str(_In_ int p_error_code) noexcept;

  void release() noexcept {
    if (this->parser != nullptr) {
      av_parser_close(this->parser);
      this->parser = nullptr;
    }
    if (this->codec_ctx != nullptr) {
      if (avcodec_is_open(this->codec_ctx) > 0) {
        avcodec_close(this->codec_ctx);
      }
      avcodec_free_context(&this->codec_ctx);
      this->codec_ctx = nullptr;
    }
  }

  void move(w_ffmpeg_ctx &&p_other) noexcept {
    if (this == &p_other) {
      return;
    }
    this->codec_ctx = std::exchange(p_other.codec_ctx, nullptr);
    this->codec = std::exchange(p_other.codec, nullptr);
    this->parser = std::exchange(p_other.parser, nullptr);
  }

  gsl::owner<AVCodecContext *> codec_ctx = {};
  gsl::owner<const AVCodec *> codec = {};
  gsl::owner<AVCodecParserContext *> parser = {};

 private:
  // copy constructor
  w_ffmpeg_ctx(const w_ffmpeg_ctx &) = delete;
  // copy operator
  w_ffmpeg_ctx &operator=(const w_ffmpeg_ctx &) = delete;
};
}  // namespace wolf::media::ffmpeg

#endif  // WOLF_MEDIA_FFMPEG