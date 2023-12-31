#pragma once

#include <msquic.h>

#include <atomic>
#include <boost/leaf.hpp>
#include <span>
#include <tuple>

#include "stream/quic/datatypes/w_status.hpp"
#include "stream/quic/handles/w_connection.hpp"
#include "stream/quic/internal/common.hpp"

namespace wolf::stream::quic {

class w_stream_event;
class w_connection_event_peer_stream_started;

/**
 * @brief The wrapper of stream handle of msquic.
 */
class W_API w_stream {
  friend class internal::w_raw_access;
  friend class w_connection_event_peer_stream_started;

 public:
  using callback_type = std::function<w_status(w_stream&, w_stream_event&)>;

 private:
  using raw_callback_type = QUIC_STATUS(HQUIC, void*, QUIC_STREAM_EVENT*);

  struct context_bundle {
    callback_type callback;
    std::shared_ptr<const QUIC_API_TABLE> api_table;
    std::atomic<bool> running = false;
    std::atomic<int> refcount = 0;
    bool closed =
        false;  // to avoid double free in reentrancy of close() in callback.
  };

  static QUIC_STATUS internal_raw_callback(HQUIC listener_raw,
                                           void* context_raw,
                                           QUIC_STREAM_EVENT* event_raw);

 public:
  /**
   * @brief constructs an empty handle.
   * @note use static factory function `open` to create a valid handle.
   */
  w_stream() {}

  w_stream(const w_stream&) = delete;
  w_stream(w_stream&& p_other) noexcept
      : _handle(std::exchange(p_other._handle, nullptr)),
        _api(std::move(p_other._api)) {}

  w_stream& operator=(const w_stream&) = delete;
  w_stream& operator=(w_stream&& p_other) noexcept {
    std::swap(_handle, p_other._handle);
    std::swap(_api, p_other._api);
    return *this;
  }

  ~w_stream() { close(); }

  /**
   * @brief whether the handle is open/valid or not.
   */
  [[nodiscard]] bool is_valid() const noexcept { return _handle; }

  /**
   * @brief whether it has been started and running.
   */
  [[nodiscard]] bool is_running() const noexcept;

  /**
   * @brief open/create a stream on a connection.
   */
  [[nodiscard]] static auto open(
      w_quic_context p_context, w_connection& p_conn, callback_type p_callback,
      QUIC_STREAM_OPEN_FLAGS p_flags = QUIC_STREAM_OPEN_FLAGS::QUIC_STREAM_OPEN_FLAG_NONE) noexcept
      -> boost::leaf::result<w_stream>;

  /**
   * @brief set callback.
   */
  auto set_callback(callback_type p_callback) -> boost::leaf::result<void>;

  /**
   * @brief start the stream.
   */
  w_status start(
      QUIC_STREAM_START_FLAGS p_flags = QUIC_STREAM_START_FLAGS::QUIC_STREAM_START_FLAG_NONE);

  /**
   * @brief send one or more string buffers into stream.
   */
  w_status send(std::string_view p_str,
                QUIC_SEND_FLAGS p_flags = QUIC_SEND_FLAGS::QUIC_SEND_FLAG_NONE);

  /**
   * @brief send one or more buffers into stream.
   */
  w_status send(std::span<std::uint8_t> p_buffer,
                QUIC_SEND_FLAGS p_flags = QUIC_SEND_FLAGS::QUIC_SEND_FLAG_NONE);

  /**
   * @brief shutdown the stream.
   *
   * This is a non-blocking call, completion will be notified
   * by shutdown_complete event in callback.
   */
  void shutdown(QUIC_STREAM_SHUTDOWN_FLAGS p_flags =
                    QUIC_STREAM_SHUTDOWN_FLAGS::QUIC_STREAM_SHUTDOWN_FLAG_GRACEFUL,
                std::size_t p_error_code = 0);
  /**
   * @brief close the connection handle.
   *
   * after this the instance will be unusable.
   */
  void close();

 private:
  /**
   * @brief setup the new raw stream created by msquic.
   */
  [[nodiscard]] static auto setup_new_raw_stream(
      w_quic_context p_context, HQUIC p_stream_raw,
      callback_type p_callback) noexcept -> boost::leaf::result<w_stream>;

  template <typename HandlerF>
  [[nodiscard]] static auto make_context_callback_ptrs(HandlerF&& p_callback)
      -> std::pair<context_bundle*, QUIC_STREAM_CALLBACK_HANDLER> {
    return {new context_bundle{.callback = std::forward<HandlerF>(p_callback)},
            internal_raw_callback};
  }

  auto raw() noexcept { return _handle; }
  auto raw() const noexcept { return _handle; }

  explicit w_stream(internal::w_raw_tag, HQUIC p_handle,
                    std::shared_ptr<const QUIC_API_TABLE>) noexcept;

  HQUIC _handle = nullptr;
  std::shared_ptr<const QUIC_API_TABLE> _api;
};

}  // namespace wolf::stream::quic
