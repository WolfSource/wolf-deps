﻿#ifdef WOLF_MEDIA_FFMPEG

#include "w_ffmpeg.hpp"

extern "C" {
#include <libavutil/opt.h>
}

//#include <android/log.h>

using w_av_codec_opt = wolf::media::ffmpeg::w_av_codec_opt;
using w_av_config = wolf::media::ffmpeg::w_av_config;
using w_av_set_opt = wolf::media::ffmpeg::w_av_set_opt;
using w_decoder = wolf::media::ffmpeg::w_decoder;
using w_encoder = wolf::media::ffmpeg::w_encoder;
using w_ffmpeg = wolf::media::ffmpeg::w_ffmpeg;
using w_ffmpeg_ctx = wolf::media::ffmpeg::w_ffmpeg_ctx;

static boost::leaf::result<AVDictionary *> s_set_dict(
    _In_ const std::vector<w_av_set_opt> &p_opts) noexcept {
  AVDictionary *_dict = nullptr;
  if (p_opts.empty()) {
    return _dict;
  }

  auto _ret = av_dict_set(&_dict, nullptr, nullptr, 0);
  if (_ret < 0) {
    return W_FAILURE(std::errc::operation_canceled,
                     "could not allocate memory for AVDictionary because: " +
                         w_ffmpeg_ctx::get_av_error_str(_ret));
  }

  try {
    for (const auto &_opt : p_opts) {
      if (_opt.name.empty()) {
        continue;
      }

      auto _name_str = _opt.name.c_str();
      if (std::holds_alternative<int>(_opt.value)) {
        // set an integer value
        const auto _value = std::get<int>(_opt.value);
        const auto _ret = av_dict_set_int(&_dict, _name_str, _value, 0);
        if (_ret < 0) {
          return W_FAILURE(std::errc::invalid_argument,
                           "could not set int value for " + _opt.name + ":" +
                               std::to_string(_value) + " because " +
                               w_ffmpeg_ctx::get_av_error_str(_ret));
        }
      } else {
        // set string value
        const auto _value_str = &std::get<std::string>(_opt.value);
        if (_value_str && !_value_str->empty()) {
          const auto _ret =
              av_dict_set(&_dict, _opt.name.c_str(), _value_str->c_str(), 0);
          if (_ret < 0) {
            return W_FAILURE(std::errc::invalid_argument,
                             "could not set string value for " + _opt.name +
                                 ":" + *_value_str + " because " +
                                 w_ffmpeg_ctx::get_av_error_str(_ret));
          }
        }
      }
    }
  } catch (const std::exception &p_exc) {
    return W_FAILURE(std::errc::operation_canceled,
                     "s_set_dict failed because: " + std::string(p_exc.what()));
  }
  return _dict;
}

static boost::leaf::result<int> s_create(
    _Inout_ w_ffmpeg_ctx &p_ctx, _In_ const AVCodecParameters* p_params) noexcept {
  p_ctx.codec_ctx = avcodec_alloc_context3(p_ctx.codec);
  if (p_ctx.codec_ctx == nullptr) {
    return W_FAILURE(std::errc::not_enough_memory,
                     "could not allocate memory for avcodec context3");
  }

  bool _has_error = false;
  DEFER {
    if (_has_error && p_ctx.codec_ctx) {
      auto _ptr = p_ctx.codec_ctx;
      avcodec_free_context(&_ptr);
      p_ctx.codec_ctx = nullptr;
    }
  });

  /* Copy codec parameters from input stream to output codec context */
  auto _ret = avcodec_parameters_to_context(p_ctx.codec_ctx, p_params);
  if (_ret < 0) {
    return W_FAILURE(std::errc::operation_canceled,
                     "failed to copy" + std::string(av_get_media_type_string(AVMEDIA_TYPE_AUDIO)) + " because " +
                         w_ffmpeg_ctx::get_av_error_str(_ret));
  }

  // open avcodec
  _ret =
      avcodec_open2(p_ctx.codec_ctx, p_ctx.codec_ctx->codec, nullptr);
  if (_ret < 0) {
    return W_FAILURE(std::errc::operation_canceled,
                     "could not open avcodec because " +
                         w_ffmpeg_ctx::get_av_error_str(_ret));
  }
  return 0;
}

boost::leaf::result<w_encoder> w_ffmpeg::create_encoder(_In_ const AVCodecParameters *p_params,
                                                        _In_ AVCodecID p_id) noexcept {
  w_encoder _encoder = {};

  _encoder.ctx.codec = avcodec_find_encoder(p_id);
  if (_encoder.ctx.codec == nullptr) {
    return W_FAILURE(
        std::errc::invalid_argument,
        "could not find encoder codec id: " + std::to_string(p_id));
  }

  BOOST_LEAF_CHECK(s_create(_encoder.ctx, p_params));

  return _encoder;
}

boost::leaf::result<w_encoder> w_ffmpeg::create_encoder(
    _In_ const AVCodecParameters *p_params,
    _In_ const std::string &p_id) noexcept {
  w_encoder _encoder = {};

  _encoder.ctx.codec = avcodec_find_encoder_by_name(p_id.c_str());
  if (_encoder.ctx.codec == nullptr) {
    return W_FAILURE(std::errc::invalid_argument,
                     "could not find encoder codec id: " + p_id);
  };

  BOOST_LEAF_CHECK(s_create(_encoder.ctx, p_params));

  return _encoder;
}

boost::leaf::result<w_decoder> w_ffmpeg::create_decoder(
    _In_ const AVCodecParameters *p_params,
    _In_ const AVCodecID p_id) noexcept {
  w_decoder _decoder = {};

  _decoder.ctx.codec = avcodec_find_decoder(p_id);
  if (_decoder.ctx.codec == nullptr) {
    return W_FAILURE(
        std::errc::invalid_argument,
        "could not find decoder codec id: " + std::to_string(p_id));
  }

  BOOST_LEAF_CHECK(s_create(_decoder.ctx, p_params));

  return _decoder;
}

boost::leaf::result<w_decoder> w_ffmpeg::create_decoder(
    _In_ const AVCodecParameters *p_params,
    _In_ const std::string &p_id) noexcept {
  w_decoder _decoder = {};

  _decoder.ctx.codec = avcodec_find_decoder_by_name(p_id.c_str());
  if (_decoder.ctx.codec == nullptr) {
    return W_FAILURE(std::errc::invalid_argument,
                     "could not find decoder codec id: " + p_id);
  }

  _decoder.ctx.parser = av_parser_init(_decoder.ctx.codec->id);
  if (_decoder.ctx.parser == nullptr) {
    return W_FAILURE(std::errc::invalid_argument,
                     "could not initialize parser for codec id: " + p_id);
  }

  BOOST_LEAF_CHECK(s_create(_decoder.ctx, p_params));

  return _decoder;
}

boost::leaf::result<int> w_ffmpeg::open_stream(
    _In_ const std::string &p_url, _In_ const std::vector<w_av_set_opt> &p_opts,
    _In_ const std::function<bool(
        const w_av_packet & /*p_packet*/, const AVStream * /*p_audio_stream*/,
        const AVStream * /*p_video_stream*/)> &p_on_frame) noexcept {
  try {
    // url is invalid
    if (p_url.empty()) {
      return W_FAILURE(std::errc::invalid_argument,
                       "could not allocate memory for av format context");
    }

    AVFormatContext *_fmt_ctx = nullptr;

    DEFER {
      if (_fmt_ctx != nullptr) {
        // free av format context
        avformat_free_context(_fmt_ctx);
        _fmt_ctx = nullptr;
      }
    });

    // allocate memory for packet
    auto _packet = w_av_packet();
    BOOST_LEAF_CHECK(_packet.init());

    // set options to av format context
    BOOST_LEAF_AUTO(_dict, s_set_dict(p_opts));

    // open input url
    int _ret = avformat_open_input(&_fmt_ctx, p_url.c_str(), nullptr, nullptr);

    if (_ret < 0) {
      //__android_log_print(ANDROID_LOG_INFO, "avformat_open_input failed", "%d", _ret);

      return W_FAILURE(std::errc::operation_canceled,
                       "could not open input url: " + p_url +
                           " because: " + w_ffmpeg_ctx::get_av_error_str(_ret));
    }

    //__android_log_print(ANDROID_LOG_INFO, "avformat_open_input success", "%d", _ret);

    // find the stream info
    _ret = avformat_find_stream_info(_fmt_ctx, nullptr);
    if (_ret < 0) {
      //__android_log_print(ANDROID_LOG_INFO, "avformat_find_stream_info failed", "%d", _ret);

      return W_FAILURE(std::errc::operation_canceled,
                       "could not find stream info from the url: " + p_url);
    }

    if (_fmt_ctx->nb_streams == 0) {
      //__android_log_print(ANDROID_LOG_INFO, "_fmt_ctx->nb_streams failed", " ");

      return W_FAILURE(std::errc::operation_canceled,
                       "missing stream for the url: " + p_url);
    }

    // search for audio & video stream
    const auto _video_stream_index =
        av_find_best_stream(_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    const auto _audio_stream_index =
        av_find_best_stream(_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if (_audio_stream_index < 0 && _video_stream_index < 0) {
      //__android_log_print(ANDROID_LOG_INFO, "_video_stream_index failed", " ");

      return W_FAILURE(
          std::errc::operation_canceled,
          "could not find any video or audio stream from the url: " + p_url);
    }

    AVStream *_audio_stream = nullptr;
    AVStream *_video_stream = nullptr;

    if (_audio_stream_index >= 0) {
      _audio_stream = _fmt_ctx->streams[_audio_stream_index];
    }
    if (_video_stream_index >= 0) {
      _video_stream = _fmt_ctx->streams[_video_stream_index];
    }

    for (;;) {
      // unref packet
      _packet.unref();
      // read packet
      _ret = av_read_frame(_fmt_ctx, _packet._packet);
      if (_ret < 0) {
        //__android_log_print(ANDROID_LOG_INFO, "av_read_frame failed", "%d", _ret);

        break;
      }

      if (p_on_frame && !p_on_frame(_packet, _audio_stream, _video_stream)) {
        break;
      }
    }
    return 0;
  } catch (const std::exception &p_exc) {
    return W_FAILURE(std::errc::operation_canceled,
                     "caught an exception: " + std::string(p_exc.what()));
  }
}

#endif  // WOLF_MEDIA_FFMPEG