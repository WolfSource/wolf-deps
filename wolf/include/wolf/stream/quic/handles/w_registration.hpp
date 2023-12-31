#pragma once

#include <msquic.h>

#include <boost/leaf.hpp>

#include "stream/quic/datatypes/w_registration_config.hpp"
#include "stream/quic/internal/common.hpp"
#include "stream/quic/w_quic_context.hpp"

namespace wolf::stream::quic {

/**
 * @brief Wrapper of registration handle of msquic.
 *
 * registration is the execution context of a quic application.
 * all connections, worker threads, internal allocations, etc
 * are controlled by registration.
 *
 * one likely won't need more than one registration in their application.
 *
 * @note make sure that the configuration handle has
 *       a shorter lifetime and closes before this registration,
 *       or there might be deadlock.
 */
class W_API w_registration {
  friend internal::w_raw_access;

 public:
  /**
   * @brief constructs an empty unusable handle.
   * @note use static factory function `open` to create a valid handle.
   */
  w_registration() {}

  w_registration(const w_registration&) = delete;
  w_registration(w_registration&& p_other) noexcept
      : _handle(std::exchange(p_other._handle, nullptr)),
        _api(std::move(p_other._api)) {}

  w_registration& operator=(const w_registration&) = delete;
  w_registration& operator=(w_registration&& p_other) noexcept {
    std::swap(_handle, p_other._handle);
    std::swap(_api, p_other._api);
    return *this;
  }

  ~w_registration() { close(); }

  /**
   * @brief whether the handle is open/valid or not.
   */
  [[nodiscard]] bool is_valid() const noexcept { return _handle; }

  /**
   * @brief open/create a regisration with default registration config.
   */
  [[nodiscard]] static auto open(w_quic_context p_context) noexcept
      -> boost::leaf::result<w_registration>;

  /**
   * @brief open/create a registration with given registration config.
   * @param p_config registration configuration.
   */
  [[nodiscard]] static auto open(w_quic_context p_context,
                                 const w_registration_config& p_config) noexcept
      -> boost::leaf::result<w_registration>;

  /**
   * @brief shutdown the execution context along with its connections.
   * @param p_flags connection shutdown flags.
   * @param p_error_code error code to send to peers.
   */
  void shutdown(
      QUIC_CONNECTION_SHUTDOWN_FLAGS p_flags =
          QUIC_CONNECTION_SHUTDOWN_FLAGS::QUIC_CONNECTION_SHUTDOWN_FLAG_NONE,
      std::size_t p_error_code = 0);

  /**
   * @brief close the registration and free up its memory.
   * @note the associated configuration handle
   *       must be closed before closing the registration.
   */
  void close();

 private:
  auto raw() noexcept { return _handle; }
  auto raw() const noexcept { return _handle; }

  explicit w_registration(internal::w_raw_tag, HQUIC p_handle,
                          std::shared_ptr<const QUIC_API_TABLE> p_api) noexcept
      : _handle(p_handle), _api(std::move(p_api)) {}

  HQUIC _handle = nullptr;
  std::shared_ptr<const QUIC_API_TABLE> _api{};
};

}  // namespace wolf::stream::quic
