/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#ifdef WOLF_MEDIA_FFMPEG

#pragma once

#include <wolf.hpp>

#include "w_av_config.hpp"

extern "C" {
#include <libavformat/avformat.h>
}

#include <vector>
#include <memory>

namespace wolf::media::ffmpeg {

class w_decoder;
class w_encoder;

class w_av_frame {
  friend w_decoder;
  friend w_encoder;

 public:
  /**
   * constructor the av_frame with specific config
   * @param p_config, the av audio config
   */
  W_API explicit w_av_frame(_In_ w_av_config &&p_config) noexcept;

  // destructor
  W_API virtual ~w_av_frame() noexcept { release(); }

  // move constructor.
  W_API w_av_frame(w_av_frame &&p_other) noexcept {
    move(std::forward<w_av_frame &&>(p_other));
  }
  // move assignment operator.
  W_API w_av_frame &operator=(w_av_frame &&p_other) noexcept {
    move(std::forward<w_av_frame &&>(p_other));
    return *this;
  }

  /**
   * initialize the avframe
   * @returns zero on success
   */
  W_API
  boost::leaf::result<int> init() noexcept;

  /**
   * set the AVFrame data
   * @param p_data, the initial data of ffmpeg AVFrame
   * @param p_alignment, the alignment
   * @returns zero on success
   */
  W_API boost::leaf::result<int> set_video_frame(
      _Inout_ std::vector<uint8_t> &&p_data) noexcept;

  /**
   * set the AVFrame's pts
   * @param p_pts, the pts data
   * @returns void
   */
  W_API void set_pts(_In_ int64_t p_pts) noexcept;

  /**
   * get data and linesize as a tuple
   * @returns tuple<int*[8], int[8]>
   */
  W_API
  std::tuple<uint8_t **, int> get() const noexcept;

  W_API
  std::tuple<uint8_t *, int> get_y_plane() const noexcept;

  W_API
  std::tuple<uint8_t *, int> get_u_plane() const noexcept;

  W_API
  std::tuple<uint8_t *, int> get_v_plane() const noexcept;

  /**
   * convert the ffmpeg video AVFrame
   * @returns the converted instance of AVFrame
   */
  W_API
  boost::leaf::result<std::shared_ptr<w_av_frame>> convert_video(
      _In_ w_av_config &&p_dst_config);

  /**
   * convert the ffmpeg audio AVFrame
   * @returns the converted instance of AVFrame
   */
  W_API
  boost::leaf::result<std::shared_ptr<w_av_frame>> convert_audio(
      _In_ w_av_config &&p_dst_config);

  /**
   * @returns config
   */
  W_API w_av_config get_config() const noexcept;

  W_API AVFrame *get_frame() const noexcept;

  /**
   * create w_av_frame from image file path
   * @returns the AVFrame
   */
  W_API
  static boost::leaf::result<w_av_frame> load_video_frame_from_img_file(
      _In_ const std::filesystem::path &p_path, _In_ AVPixelFormat p_pixel_fmt);

  /**
   * save to to the image file
   * @param p_quality, quality will be used only for jpeg and is between 1 and
   * 100
   * @returns zero on success
   */
  W_API
  boost::leaf::result<int> save_video_frame_to_img_file(
      _In_ const std::filesystem::path &p_path, int p_quality = 100) noexcept;

  void release() noexcept {
    if (this->_av_frame != nullptr) {
      if (this->_config.nb_channels > 0) {
        av_channel_layout_uninit(&this->_av_frame->ch_layout);
      }
      av_frame_free(&this->_av_frame);
    }
  }

  void move(w_av_frame &&p_other) noexcept {
    if (this == &p_other) {
      return;
    }
    this->_av_frame = std::exchange(p_other._av_frame, nullptr);
    this->_config = std::move(p_other._config);
    this->_data = std::move(p_other._data);
  }

 private:
  // copy constructor.
  w_av_frame(const w_av_frame &) = delete;
  // copy assignment operator.
  w_av_frame &operator=(const w_av_frame &) = delete;
  // the channel layout of the audio
  AVChannelLayout _channel_layout = {};
  // the AVFrame config
  w_av_config _config = {};
  // the ffmpeg AVFrame
  gsl::owner<AVFrame *> _av_frame = nullptr;
  // the ffmpeg AVFrame data
  std::vector<uint8_t> _data = {};
  // the ffmpeg AVFrame data size
  int _data_size = 0;
};
}  // namespace wolf::media::ffmpeg

#endif  // WOLF_MEDIA_FFMPEG
