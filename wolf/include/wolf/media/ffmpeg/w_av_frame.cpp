#ifdef WOLF_MEDIA_FFMPEG

#include "w_av_frame.hpp"

#include "w_ffmpeg_ctx.hpp"

extern "C" {
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

#ifdef WOLF_MEDIA_STB
#include <stb_image.h>
#include <stb_image_write.h>
#endif  // WOLF_MEDIA_STB

using w_av_frame = wolf::media::ffmpeg::w_av_frame;
using w_av_config = wolf::media::ffmpeg::w_av_config;

w_av_frame::w_av_frame(_In_ w_av_config &&p_config) noexcept
    : _config(std::move(p_config)) {}

boost::leaf::result<int> w_av_frame::init() noexcept {
  release();

  // allocate memory for AVFrame
  this->_av_frame = av_frame_alloc();
  if (this->_av_frame == nullptr) {
    return W_FAILURE(std::errc::not_enough_memory,
                     "could not allocate memory for AVFrame");
  }

  // set audio
  this->_av_frame->sample_rate = this->_config.sample_rate;
  if (this->_config.nb_channels > 0) {
    av_channel_layout_default(&this->_av_frame->ch_layout,
                              this->_config.nb_channels);
  }

  // set video
  this->_av_frame->format = gsl::narrow_cast<int>(this->_config.format);
  this->_av_frame->width = this->_config.width;
  this->_av_frame->height = this->_config.height;

  return 0;
}

boost::leaf::result<int> w_av_frame::set_video_frame(
    _Inout_ std::vector<uint8_t> &&p_data) noexcept {
  const auto _width = this->_config.width;
  const auto _height = this->_config.height;
  const auto _alignment = this->_config.alignment;

  // check for width and height
  if (_width <= 0 || _height <= 0) {
    return W_FAILURE(std::errc::invalid_argument,
                     "width or height of w_av_frame is zero");
  }

  // move the owenership of data to buffer
  this->_data = std::forward<std::vector<uint8_t> &&>(p_data);

  const auto _buffer_size = this->_config.get_required_video_buffer_size();
  if (this->_data.empty()) {
    this->_data.resize(_buffer_size, 0);
  }

  // if size does not fit
  if (this->_data.size() != _buffer_size) {
    return W_FAILURE(
        std::errc::invalid_argument,
        wolf::format("w_av_frame video buffer size is expected {} but is {}",
                     _buffer_size, this->_data.size()));
  }

  const auto _ret = av_image_fill_arrays(
      this->_av_frame->data, this->_av_frame->linesize,
      gsl::narrow_cast<const uint8_t *>(this->_data.data()),
      this->_config.format, this->_av_frame->width, this->_av_frame->height,
      _alignment);
  if (_ret < 0) {
    return W_FAILURE(std::errc::operation_canceled,
                     "av_image_fill_arrays failed");
  }
  return _ret;
}

void w_av_frame::set_pts(_In_ int64_t p_pts) noexcept {
  this->_av_frame->pts = p_pts;
}

std::tuple<uint8_t **, int> w_av_frame::get() const noexcept {
  if (this->_av_frame) {
    return std::make_tuple(this->_av_frame->data, this->_data_size);
  }
  return std::make_tuple(nullptr, 0);
}

AVFrame* w_av_frame::get_frame() const noexcept { return this->_av_frame; }

w_av_config w_av_frame::get_config() const noexcept { return this->_config; }

boost::leaf::result<std::shared_ptr<w_av_frame>> w_av_frame::convert_audio(
    _In_ w_av_config &&p_dst_config) {
  auto _ret = 0;
  int dst_nb_samples, max_dst_nb_samples, dst_nb_channels = 0;
  int dst_linesize;
  uint8_t **dst_data = NULL;

  SwrContext *swr = nullptr;
  auto _dst_frame = std::make_shared<w_av_frame>(std::move(p_dst_config));
  _dst_frame->init();

  DEFER {
    if (_ret) {
      if (swr) {
        if (swr_is_initialized(swr)) {
          swr_close(swr);
        }
        swr_free(&swr);
      }
    }
  });

  /* create resampler context */
  swr = swr_alloc();
  if (!swr) {
    return W_FAILURE(
        std::errc::operation_canceled,
        "could not allocate resampler context because: " + w_ffmpeg_ctx::get_av_error_str(_ret));
  }

  /* set options */
  av_opt_set_chlayout(swr, "in_chlayout", &this->_av_frame->ch_layout, 0);
  av_opt_set_int(swr, "in_sample_rate", this->_av_frame->sample_rate, 0);
  av_opt_set_sample_fmt(swr, "in_sample_fmt", 
      gsl::narrow_cast<AVSampleFormat>(this->_av_frame->format), 0);

  av_opt_set_chlayout(swr, "out_chlayout", &_dst_frame->_av_frame->ch_layout, 0);
  av_opt_set_int(swr, "out_sample_rate", _dst_frame->_av_frame->sample_rate, 0);
  av_opt_set_sample_fmt(swr, "out_sample_fmt", 
      p_dst_config.sample_fmts, 0);

  /* initialize the resampling context */
  if ((_ret = swr_init(swr)) < 0) {
    fprintf(stderr, "Failed to initialize the resampling context\n");
    return W_FAILURE(
        std::errc::operation_canceled,
        "failed to initialize the resampling context because: " + w_ffmpeg_ctx::get_av_error_str(_ret));
  }

   /* compute the number of converted samples: buffering is avoided
   * ensuring that the output buffer will contain at least all the
   * converted input samples */
  max_dst_nb_samples = dst_nb_samples = av_rescale_rnd(this->_av_frame->nb_samples, _dst_frame->_av_frame->sample_rate,
                     this->_av_frame->sample_rate, AV_ROUND_UP);

  /* buffer is going to be directly written to a rawaudio file, no alignment */
  dst_nb_channels = _dst_frame->_av_frame->ch_layout.nb_channels;
  _ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize,
                                            dst_nb_channels,
                                            dst_nb_samples, 
      p_dst_config.sample_fmts, 0);
  
  _dst_frame->_av_frame->data[0] = std::move(dst_data[0]);
  if (_ret < 0) {
    auto tmp = w_ffmpeg_ctx::get_av_error_str(_ret);
    return W_FAILURE(
        std::errc::operation_canceled,
        "could not allocate destination array because: " + w_ffmpeg_ctx::get_av_error_str(_ret));
  }

  /* compute destination number of samples */
  dst_nb_samples =
      av_rescale_rnd(swr_get_delay(swr, this->_av_frame->sample_rate) + this->_av_frame->nb_samples,
                     _dst_frame->_av_frame->sample_rate, this->_av_frame->sample_rate, AV_ROUND_UP);
  if (dst_nb_samples > max_dst_nb_samples) {
    av_freep(&_dst_frame->_av_frame->data);
    _ret = av_samples_alloc(_dst_frame->_av_frame->data, &dst_linesize, dst_nb_channels,
                            dst_nb_samples, p_dst_config.sample_fmts,
                           1);
    if (_ret < 0)
    {
      auto tmp = w_ffmpeg_ctx::get_av_error_str(_ret);
      return W_FAILURE(
          std::errc::operation_canceled,
          "could not allocate avframe samples because: " + w_ffmpeg_ctx::get_av_error_str(_ret));
    }

    max_dst_nb_samples = dst_nb_samples;
  }

  /* convert to destination format */
  auto size =
      swr_convert(swr, _dst_frame->_av_frame->data, dst_nb_samples,
      (const uint8_t **)(&this->_av_frame->data[0]),
      this->_av_frame->nb_samples);

  if (size < 0) {
    _ret = -1;
    return W_FAILURE(std::errc::operation_canceled,
                     "error while audio converting\n");
  }

  const auto _buffer_size =
      av_samples_get_buffer_size(&dst_linesize, dst_nb_channels, size, p_dst_config.sample_fmts, 1);

  if (_buffer_size < 0) {
    _ret = -1;
    return W_FAILURE(std::errc::operation_canceled,
                     "could not get sample buffer size\n");
  }

  _dst_frame->_data_size = _buffer_size;
  return _dst_frame;
}

boost::leaf::result<std::shared_ptr<w_av_frame>> w_av_frame::convert_video(
    _In_ w_av_config &&p_dst_config) {
  // create a buffer and dst frame
  auto _video_buffer = std::vector<uint8_t>();
  auto _dst_frame = std::make_shared<w_av_frame>(std::move(p_dst_config));
  BOOST_LEAF_CHECK(_dst_frame->init());
  BOOST_LEAF_CHECK(_dst_frame->set_video_frame(std::move(_video_buffer)));

  auto *_context = sws_getContext(
      this->_config.width, this->_config.height, this->_config.format,
                     _dst_frame->_config.width, _dst_frame->_config.height,
                     _dst_frame->_config.format, SWS_BICUBIC, nullptr, nullptr, nullptr);
  if (_context == nullptr) {
    return W_FAILURE(std::errc::not_enough_memory,
                     "could not create sws context");
  }

  auto _dst_frame_nn = gsl::not_null<AVFrame *>(_dst_frame->_av_frame);
  const auto _height = sws_scale(
      _context, gsl::narrow_cast<const uint8_t *const *>(this->_av_frame->data),
      gsl::narrow_cast<const int *>(this->_av_frame->linesize), 0,
      this->_config.height,
      gsl::narrow_cast<uint8_t *const *>(_dst_frame_nn->data),
      gsl::narrow_cast<const int *>(_dst_frame_nn->linesize));

  // free context
  sws_freeContext(_context);

  if (_height < 0) {
    return W_FAILURE(std::errc::invalid_argument,
                     "w_av_frame sws_scale failed because: \"" +
                         w_ffmpeg_ctx::get_av_error_str(_height) + "\"");
  }

  const auto _buffer_size =
      av_image_get_buffer_size(gsl::narrow_cast<AVPixelFormat>(_dst_frame->_av_frame->format),
                               _dst_frame->_av_frame->width, _dst_frame->_av_frame->height, 4);

 if (_buffer_size < 0) {
    return W_FAILURE(std::errc::operation_canceled, "could not get sample buffer size\n");
  }

  _dst_frame->_data_size = _buffer_size;
  return _dst_frame;
}

boost::leaf::result<w_av_frame> w_av_frame::load_video_frame_from_img_file(
    _In_ const std::filesystem::path &p_path, _In_ AVPixelFormat p_pixel_fmt) {
#ifdef WOLF_MEDIA_STB

  // width, height, comp
  int _width = 0;
  int _height = 0;
  int _comp = 0;

  const auto _path = p_path.string();
  if (!std::filesystem::exists(p_path)) {
    return W_FAILURE(std::errc::invalid_argument,
                     " path not exist for av_frame" + _path);
  }

  auto *_raw_img_data = stbi_load(_path.c_str(), &_width, &_height, &_comp, 0);

  if (_raw_img_data == nullptr) {
    return W_FAILURE(std::errc::invalid_argument,
                     "could not load image file " + _path);
  }

  auto _len = gsl::narrow_cast<size_t>(_width * _height * _comp);
  const auto _raw_img_data_span =
      gsl::span(_raw_img_data, _raw_img_data + _len);
  auto _raw_img_data_vec = std::vector<uint8_t>(_raw_img_data_span.begin(),
                                                _raw_img_data_span.end());

  free(_raw_img_data);

  auto _src_config = w_av_config(p_pixel_fmt, _width, _height);
  // create an av_frame from image raw data
  auto _src_frame = w_av_frame(std::move(_src_config));
  BOOST_LEAF_CHECK(_src_frame.init());
  BOOST_LEAF_CHECK(_src_frame.set_video_frame(std::move(_raw_img_data_vec)));

  return _src_frame;
#else
  return W_FAILURE(std::errc::not_supported, "WOLF_MEDIA_STB not defined");
#endif
}

boost::leaf::result<int> w_av_frame::save_video_frame_to_img_file(
    _In_ const std::filesystem::path &p_path, int p_quality) noexcept {
#ifdef WOLF_MEDIA_STB
  try {
    if (this->_av_frame == nullptr || this->_av_frame->width == 0 ||
        this->_av_frame->height == 0) {
      return W_FAILURE(std::errc::invalid_argument,
                       "bad parameters for avframe");
    }

    const auto _path = p_path.string();
    auto _ext = p_path.extension().string();
    std::transform(_ext.cbegin(), _ext.cend(), _ext.begin(), ::tolower);

    const auto _comp = this->_av_frame->linesize[0] / this->_av_frame->width;
    if (_ext == ".bmp") {
      return stbi_write_bmp(_path.c_str(), this->_config.width,
                            this->_config.height, _comp,
                            this->_av_frame->data[0]);
    }
    if (_ext == ".png") {
      return stbi_write_png(
          _path.c_str(), this->_config.width, this->_config.height, _comp,
          this->_av_frame->data[0], this->_av_frame->linesize[0]);
    }
    if (_ext == ".jpg" || _ext == ".jpeg") {
      return stbi_write_jpg(_path.c_str(), this->_config.width,
                            this->_config.height, _comp,
                            this->_av_frame->data[0], p_quality);
    }
    return W_FAILURE(std::errc::invalid_argument,
                     "image format not supported for " + _path);
  } catch (const std::exception &p_exc) {
    return W_FAILURE(std::errc::invalid_argument,
                     "caught an esxception for " + std::string(p_exc.what()));
  }
#else
  return W_FAILURE(std::errc::not_supported, "WOLF_MEDIA_STB not defined");
#endif
}

std::tuple<uint8_t*, int> w_av_frame::get_y_plane() const noexcept
{
  if (this->_av_frame) {
    return std::make_tuple(this->_av_frame->data[0], this->_av_frame->linesize[0]);
  }
  return std::make_tuple(nullptr, 0);
}

std::tuple<uint8_t*, int> w_av_frame::get_u_plane() const noexcept
{
  if (this->_av_frame) {
    return std::make_tuple(this->_av_frame->data[1], this->_av_frame->linesize[1]);
  }
  return std::make_tuple(nullptr, 0);
}

std::tuple<uint8_t*, int> w_av_frame::get_v_plane() const noexcept
{
  if (this->_av_frame) {
    return std::make_tuple(this->_av_frame->data[2], this->_av_frame->linesize[2]);
  }
  return std::make_tuple(nullptr, 0);
}

#endif  // WOLF_MEDIA_FFMPEG