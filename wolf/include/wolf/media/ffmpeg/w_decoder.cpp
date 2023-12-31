﻿#ifdef WOLF_MEDIA_FFMPEG

#include "w_decoder.hpp"

using w_decoder = wolf::media::ffmpeg::w_decoder;

boost::leaf::result<int> w_decoder::decode_frame_from_packet(
    _In_ AVPacket *p_packet, _Inout_ w_av_frame &p_frame) {
  // start decoding
  auto _ret = avcodec_send_packet(this->ctx.codec_ctx, p_packet);
  if (_ret < 0) {
    return W_FAILURE(std::errc::operation_canceled,
                     "could not parse packet for decoding because:\"" +
                         w_ffmpeg_ctx::get_av_error_str(_ret) + "\"");
  }

  for (;;) {
    _ret = avcodec_receive_frame(this->ctx.codec_ctx, p_frame._av_frame);
    if (_ret == 0 || _ret == AVERROR(EAGAIN) || _ret == AVERROR_EOF) {
      break;
    }
    if (_ret < 0) {
      return W_FAILURE(std::errc::operation_canceled,
                       "error happened during the encoding because:\"" +
                           w_ffmpeg_ctx::get_av_error_str(_ret) + "\"");
    }
  }
  return 0;
}

boost::leaf::result<int> w_decoder::decode(_In_ const w_av_packet &p_packet,
                                           _Inout_ w_av_frame &p_frame,
                                           _In_ bool p_flush) noexcept {
  auto _dst_packet = w_av_packet();
  _dst_packet.init();

    BOOST_LEAF_CHECK(decode_frame_from_packet(p_packet._packet, p_frame));

  if (p_flush) {
    // flush the decoder
    BOOST_LEAF_CHECK(decode_frame_from_packet(nullptr, p_frame));
  }

  return 0;
}

#endif  // WOLF_MEDIA_FFMPEG