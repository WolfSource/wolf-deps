#pragma once

#include "stream/quic/internal/common.hpp"
#include "stream/quic/datatypes/w_status.hpp"
#include "stream/quic/handles/w_configuration.hpp"
#include "stream/quic/handles/w_registration.hpp"

#include <boost/leaf.hpp>
#include <msquic.h>

#include <atomic>
#include <span>
#include <tuple>
#include <mutex>

namespace wolf::stream::quic {

class w_connection_event;
class w_listener_event_new_connection;

/**
 * @brief The wrapper of connection handle of msquic.
 */
class W_API w_connection {
  friend class internal::w_raw_access;
  friend class w_listener_event_new_connection;

 public:
  using callback_type = std::function<w_status(w_connection&, w_connection_event&)>;

 private:
  using raw_callback_type = QUIC_STATUS(HQUIC, void*, QUIC_CONNECTION_EVENT*);

  struct context_bundle {
    callback_type callback;
    std::shared_ptr<const QUIC_API_TABLE> api_table;
    std::atomic<bool> running = false;
    std::atomic<int> refcount = 0;
    bool closing = false;  // to avoid double free in reentrancy of close() in callback.
  };

  static QUIC_STATUS internal_raw_callback(HQUIC p_connection_raw, void* p_context_raw,
                                           QUIC_CONNECTION_EVENT* p_event_raw);

 public:
  /**
   * @brief constructs an empty handle.
   * @note use static factory function `open` to create a valid handle.
   */
  w_connection() {}

  w_connection(const w_connection&) = delete;
  w_connection(w_connection&& p_other) noexcept
      : _handle(std::exchange(p_other._handle, nullptr)), _api(std::move(p_other._api)) {}

  w_connection& operator=(const w_connection&) = delete;
  w_connection& operator=(w_connection&& p_other) noexcept {
    std::swap(_handle, p_other._handle);
    std::swap(_api, p_other._api);
    return *this;
  }

  ~w_connection() { close(); }

  /**
   * @brief whether the handle is open/valid or not.
   */
  [[nodiscard]] bool is_valid() const noexcept { return _handle; }

  /**
   * @brief whether it has been started and running.
   */
  [[nodiscard]] bool is_running() const noexcept;

  /**
   * @brief open/create a connection.
   */
  [[nodiscard]] static auto open(w_quic_context p_context, w_registration& p_reg,
                                 callback_type p_callback) noexcept
      -> boost::leaf::result<w_connection>;

  /**
   * @brief set callback.
   */
  auto set_callback(callback_type p_callback) -> boost::leaf::result<void>;

  /**
   * @brief start the connection to connect.
   */
  w_status start(w_configuration& p_config, const char* p_host, std::uint16_t p_port);

  /**
   * @brief shutdown the connection.
   *
   * This is a non-blocking call, completion will be notified
   * by shutdown_complete event in callback.
   */
  void shutdown(QUIC_CONNECTION_SHUTDOWN_FLAGS p_flags =
                    QUIC_CONNECTION_SHUTDOWN_FLAGS::QUIC_CONNECTION_SHUTDOWN_FLAG_NONE,
                std::size_t p_error_code = 0);

  /**
   * @brief close the connection handle.
   *
   * after this the instance will be unusable.
   */
  void close();

 private:
  /**
   * @brief setup the new raw connection created by msquic.
   */
  [[nodiscard]] static auto setup_new_raw_connection(w_quic_context p_context, HQUIC p_conn_raw,
                                                     w_configuration& p_config,
                                                     callback_type p_callback) noexcept
      -> boost::leaf::result<w_connection>;

  template <typename HandlerF>
  [[nodiscard]] static auto make_context_callback_ptrs(HandlerF&& p_callback)
      -> std::pair<context_bundle*, QUIC_CONNECTION_CALLBACK_HANDLER> {
    return {new context_bundle{.callback = std::forward<HandlerF>(p_callback)},
            internal_raw_callback};
  }

  auto raw() noexcept { return _handle; }
  auto raw() const noexcept { return _handle; }

  explicit w_connection(internal::w_raw_tag, HQUIC p_handle,
                        std::shared_ptr<const QUIC_API_TABLE> p_api) noexcept;

  HQUIC _handle = nullptr;
  std::shared_ptr<const QUIC_API_TABLE> _api{};
};

}  // namespace wolf::stream::quic
