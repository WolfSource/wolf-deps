#pragma once

#include <msquic.h>

#include "stream/quic/datatypes/w_status.hpp"
#include "stream/quic/handles/w_stream.hpp"
#include "stream/quic/internal/common.hpp"

// NOTE windows.h must be included after msquic.h
//      due to some declaration issues.
#include <string_view>

#include "wolf.hpp"

namespace wolf::stream::quic {

/**
 * @brief Wrapper around CONNECTED variation of QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_connected {
  friend class internal::w_raw_access;

 public:
  w_connection_event_connected() = delete;
  w_connection_event_connected(const w_connection_event_connected&) = delete;
  w_connection_event_connected(w_connection_event_connected&&) = delete;

  [[nodiscard]] bool session_resumed() const noexcept {
    return _event->CONNECTED.SessionResumed;
  }

  [[nodiscard]] std::string_view negotiated_alpn() const noexcept {
    return std::string_view((const char*)_event->CONNECTED.NegotiatedAlpn,
                            _event->CONNECTED.NegotiatedAlpnLength);
  }

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_connected(internal::w_raw_tag,
                               QUIC_CONNECTION_EVENT* p_event)
      : _event(p_event) {}

  QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around SHUTDOWN_INITIATED_BY_PEER variation of
 * QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_shutdown_initiated_by_peer {
  friend class internal::w_raw_access;

 public:
  w_connection_event_shutdown_initiated_by_peer() = delete;
  w_connection_event_shutdown_initiated_by_peer(
      const w_connection_event_shutdown_initiated_by_peer&) = delete;
  w_connection_event_shutdown_initiated_by_peer(
      w_connection_event_shutdown_initiated_by_peer&&) = delete;

  [[nodiscard]] auto error_code() const noexcept {
    return _event->SHUTDOWN_INITIATED_BY_PEER.ErrorCode;
  }

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_shutdown_initiated_by_peer(internal::w_raw_tag,
                                                QUIC_CONNECTION_EVENT* p_event)
      : _event(p_event) {}

  QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around LOCAL_ADDRESS_CHANGED variation of
 * QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_local_address_changed {
  friend class internal::w_raw_access;

 public:
  w_connection_event_local_address_changed() = delete;
  w_connection_event_local_address_changed(
      const w_connection_event_local_address_changed&) = delete;
  w_connection_event_local_address_changed(
      w_connection_event_local_address_changed&&) = delete;

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_local_address_changed(internal::w_raw_tag,
                                           QUIC_CONNECTION_EVENT* /*p_event*/)
  // : _event(p_event)
  {}

  // QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around PEER_ADDRESS_CHANGED variation of
 * QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_peer_address_changed {
  friend class internal::w_raw_access;

 public:
  w_connection_event_peer_address_changed() = delete;
  w_connection_event_peer_address_changed(
      const w_connection_event_peer_address_changed&) = delete;
  w_connection_event_peer_address_changed(
      w_connection_event_peer_address_changed&&) = delete;

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_peer_address_changed(internal::w_raw_tag,
                                          QUIC_CONNECTION_EVENT* /*p_event*/)
  // : _event(p_event)
  {}

  // QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around PEER_STREAM_STARTED variation of QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_peer_stream_started {
  friend class internal::w_raw_access;

 public:
  w_connection_event_peer_stream_started() = delete;
  w_connection_event_peer_stream_started(
      const w_connection_event_peer_stream_started&) = delete;
  w_connection_event_peer_stream_started(
      w_connection_event_peer_stream_started&&) = delete;

  ~w_connection_event_peer_stream_started() {
    if (_event->PEER_STREAM_STARTED.Stream) {
      reject_stream();
    }
  }

  [[nodiscard]] bool is_unidirectional() const noexcept {
    return _event->PEER_STREAM_STARTED.Flags &
           QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL;
  }

  [[nodiscard]] bool is_bidirectional() const noexcept {
    return !(_event->PEER_STREAM_STARTED.Flags &
             QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL);
  }

  [[nodiscard]] QUIC_STREAM_OPEN_FLAGS flags() const noexcept {
    return _event->PEER_STREAM_STARTED.Flags;
  }

  /**
   * @brief reject the incoming stream.
   */
  void reject_stream() const noexcept {
    if (!_event->PEER_STREAM_STARTED.Stream) {
      return;
    }

    const QUIC_API_TABLE* api = nullptr;
    if (QUIC_FAILED(MsQuicOpen2(&api))) {
      return;
    }

    api->StreamClose(_event->PEER_STREAM_STARTED.Stream);
    _event->PEER_STREAM_STARTED.Stream = nullptr;  // to avoid Use-After-Free.

    MsQuicClose(api);
  }

  /**
   * @brief accept the incoming stream and set its handler callback.
   * @param p_callback  event handler callback.
   */
  template <typename HandlerF>
  auto accept_stream(HandlerF&& p_callback) const noexcept
      -> boost::leaf::result<w_stream> {
    if (!_event->PEER_STREAM_STARTED.Stream) {
      return W_FAILURE(std::errc::operation_canceled,
                       "stream is already accepted/rejected.");
    }

    BOOST_LEAF_AUTO(context, w_quic_context::make());

    BOOST_LEAF_AUTO(stream,
                    w_stream::setup_new_raw_stream(
                        std::move(context), _event->PEER_STREAM_STARTED.Stream,
                        std::forward<HandlerF>(p_callback)));

    _event->PEER_STREAM_STARTED.Stream = nullptr;

    return stream;
  }

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_peer_stream_started(internal::w_raw_tag,
                                         QUIC_CONNECTION_EVENT* p_event)
      : _event(p_event) {}

  QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around STREAMS_AVAILABLE variation of QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_streams_available {
  friend class internal::w_raw_access;

 public:
  w_connection_event_streams_available() = delete;
  w_connection_event_streams_available(
      const w_connection_event_streams_available&) = delete;
  w_connection_event_streams_available(w_connection_event_streams_available&&) =
      delete;

  [[nodiscard]] auto bidirectional_count() const noexcept {
    return _event->STREAMS_AVAILABLE.BidirectionalCount;
  }

  [[nodiscard]] auto unidirectional_count() const noexcept {
    return _event->STREAMS_AVAILABLE.UnidirectionalCount;
  }

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_streams_available(internal::w_raw_tag,
                                       QUIC_CONNECTION_EVENT* p_event)
      : _event(p_event) {}

  QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around IDEAL_PROCESSOR_CHANGED variation of
 * QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_ideal_processor_changed {
  friend class internal::w_raw_access;

 public:
  w_connection_event_ideal_processor_changed() = delete;
  w_connection_event_ideal_processor_changed(
      const w_connection_event_ideal_processor_changed&) = delete;
  w_connection_event_ideal_processor_changed(
      w_connection_event_ideal_processor_changed&&) = delete;

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_ideal_processor_changed(internal::w_raw_tag,
                                             QUIC_CONNECTION_EVENT* /*p_event*/)
  // : _event(p_event)
  {}

  // QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around DATAGRAM_STATE_CHANGED variation of
 * QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_datagram_state_changed {
  friend class internal::w_raw_access;

 public:
  w_connection_event_datagram_state_changed() = delete;
  w_connection_event_datagram_state_changed(
      const w_connection_event_datagram_state_changed&) = delete;
  w_connection_event_datagram_state_changed(
      w_connection_event_datagram_state_changed&&) = delete;

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_datagram_state_changed(internal::w_raw_tag,
                                            QUIC_CONNECTION_EVENT* /*p_event*/)
  // : _event(p_event)
  {}

  // QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around RESUMED variation of QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_resumed {
  friend class internal::w_raw_access;

 public:
  w_connection_event_resumed() = delete;
  w_connection_event_resumed(const w_connection_event_resumed&) = delete;
  w_connection_event_resumed(w_connection_event_resumed&&) = delete;

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_resumed(internal::w_raw_tag,
                             QUIC_CONNECTION_EVENT* /*p_event*/)
  // : _event(p_event)
  {}

  // QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around RESUMPTION_TICKET_RECEIVED variation of
 * QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_resumption_ticket_received {
  friend class internal::w_raw_access;

 public:
  w_connection_event_resumption_ticket_received() = delete;
  w_connection_event_resumption_ticket_received(
      const w_connection_event_resumption_ticket_received&) = delete;
  w_connection_event_resumption_ticket_received(
      w_connection_event_resumption_ticket_received&&) = delete;

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_resumption_ticket_received(
      internal::w_raw_tag, QUIC_CONNECTION_EVENT* /*p_event*/)
  // : _event(p_event)
  {}

  // QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around PEER_CERTIFICATE_RECEIVED variation of
 * QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_peer_certificate_received {
  friend class internal::w_raw_access;

 public:
  w_connection_event_peer_certificate_received() = delete;
  w_connection_event_peer_certificate_received(
      const w_connection_event_peer_certificate_received&) = delete;
  w_connection_event_peer_certificate_received(
      w_connection_event_peer_certificate_received&&) = delete;

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_peer_certificate_received(
      internal::w_raw_tag, QUIC_CONNECTION_EVENT* /*p_event*/)
  // : _event(p_event)
  {}

  // QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around DATAGRAM_RECEIVED variation of QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_datagram_received {
  friend class internal::w_raw_access;

 public:
  w_connection_event_datagram_received() = delete;
  w_connection_event_datagram_received(
      const w_connection_event_datagram_received&) = delete;
  w_connection_event_datagram_received(w_connection_event_datagram_received&&) =
      delete;

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_datagram_received(internal::w_raw_tag,
                                       QUIC_CONNECTION_EVENT* /*p_event*/)
  // : _event(p_event)
  {}

  // QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around SHUTDOWN_INITIATED_BY_TRANSPORT variation of
 * QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_shutdown_initiated_by_transport {
  friend class internal::w_raw_access;

 public:
  w_connection_event_shutdown_initiated_by_transport() = delete;
  w_connection_event_shutdown_initiated_by_transport(
      const w_connection_event_shutdown_initiated_by_transport&) = delete;
  w_connection_event_shutdown_initiated_by_transport(
      w_connection_event_shutdown_initiated_by_transport&&) = delete;

  [[nodiscard]] w_status status() const noexcept {
    return static_cast<w_status_code>(
        _event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);
  }

  [[nodiscard]] auto error_code() const noexcept {
    return _event->SHUTDOWN_INITIATED_BY_TRANSPORT.ErrorCode;
  }

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_shutdown_initiated_by_transport(
      internal::w_raw_tag, QUIC_CONNECTION_EVENT* p_event)
      : _event(p_event) {}

  QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around DATAGRAM_SEND_STATE_CHANGED variation of
 * QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_datagram_send_state_changed {
  friend class internal::w_raw_access;

 public:
  w_connection_event_datagram_send_state_changed() = delete;
  w_connection_event_datagram_send_state_changed(
      const w_connection_event_datagram_send_state_changed&) = delete;
  w_connection_event_datagram_send_state_changed(
      w_connection_event_datagram_send_state_changed&&) = delete;

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_datagram_send_state_changed(
      internal::w_raw_tag, QUIC_CONNECTION_EVENT* /*p_event*/)
  // : _event(p_event)
  {}

  // QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around PEER_NEEDS_STREAMS variation of QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_peer_needs_streams {
  friend class internal::w_raw_access;

 public:
  w_connection_event_peer_needs_streams() = delete;
  w_connection_event_peer_needs_streams(
      const w_connection_event_peer_needs_streams&) = delete;
  w_connection_event_peer_needs_streams(
      w_connection_event_peer_needs_streams&&) = delete;

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_peer_needs_streams(internal::w_raw_tag,
                                        QUIC_CONNECTION_EVENT* /*p_event*/)
  // : _event(p_event)
  {}

  // QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around SHUTDOWN_COMPLETE variation of QUIC_CONNECTION_EVENT.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event_shutdown_complete {
  friend class internal::w_raw_access;

 public:
  w_connection_event_shutdown_complete() = delete;
  w_connection_event_shutdown_complete(
      const w_connection_event_shutdown_complete&) = delete;
  w_connection_event_shutdown_complete(w_connection_event_shutdown_complete&&) =
      delete;

  [[nodiscard]] bool handshake_completed() const noexcept {
    return _event->SHUTDOWN_COMPLETE.HandshakeCompleted;
  }

  [[nodiscard]] bool peer_acknowledged_shutdown() const noexcept {
    return _event->SHUTDOWN_COMPLETE.PeerAcknowledgedShutdown;
  }

  [[nodiscard]] bool app_close_in_progress() const noexcept {
    return _event->SHUTDOWN_COMPLETE.AppCloseInProgress;
  }

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event_shutdown_complete(internal::w_raw_tag,
                                       QUIC_CONNECTION_EVENT* p_event)
      : _event(p_event) {}

  QUIC_CONNECTION_EVENT* _event = nullptr;
};

/**
 * @brief Wrapper around QUIC_CONNECTION_EVENT variant.
 *
 * @note This class is non-owning and also is not move or copy-able,
 *       but only referencible.
 *       please refer to its provider for lifetime guarantees.
 */
class W_API w_connection_event {
  friend class internal::w_raw_access;

 public:
  w_connection_event() = delete;
  w_connection_event(const w_connection_event&) = delete;
  w_connection_event(w_connection_event&&) = delete;

  /**
   * @brief get string name of the underlying event.
   */
  [[nodiscard]] auto name() const noexcept -> std::string_view {
    switch (_event->Type) {
      case QUIC_CONNECTION_EVENT_CONNECTED:
        return "connected";

      case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
        return "shutdown-initiated-by-peer";

      case QUIC_CONNECTION_EVENT_LOCAL_ADDRESS_CHANGED:
        return "local-address-changed";

      case QUIC_CONNECTION_EVENT_PEER_ADDRESS_CHANGED:
        return "peer-address-changed";

      case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
        return "peer-stream-started";

      case QUIC_CONNECTION_EVENT_STREAMS_AVAILABLE:
        return "streams-available";

      case QUIC_CONNECTION_EVENT_IDEAL_PROCESSOR_CHANGED:
        return "ideal-processor-changed";

      case QUIC_CONNECTION_EVENT_DATAGRAM_STATE_CHANGED:
        return "datagram-state-changed";

      case QUIC_CONNECTION_EVENT_RESUMED:
        return "resumed";

      case QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED:
        return "resumption-ticket-received";

      case QUIC_CONNECTION_EVENT_PEER_CERTIFICATE_RECEIVED:
        return "peer-certificate-received";

      case QUIC_CONNECTION_EVENT_DATAGRAM_RECEIVED:
        return "datagram-received";

      case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
        return "shutdown-initiated-by-transport";

      case QUIC_CONNECTION_EVENT_DATAGRAM_SEND_STATE_CHANGED:
        return "datagram-send-state-changed";

      case QUIC_CONNECTION_EVENT_PEER_NEEDS_STREAMS:
        return "peer-needs-streams";

      case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        return "shutdown-complete";
    }

    return "unknown";
  }

  /**
   * @brief visit the variant based on event type.
   * @param p_visitor  overload to handle different variations of the event
   * instnace.
   */
  template <typename F>
  auto visit(F&& p_visitor) {
    using accessor = internal::w_raw_access;

    switch (_event->Type) {
      case QUIC_CONNECTION_EVENT_CONNECTED:
        return p_visitor(
            accessor::from_raw<w_connection_event_connected>(_event));

      case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
        return p_visitor(
            accessor::from_raw<w_connection_event_shutdown_initiated_by_peer>(
                _event));

      case QUIC_CONNECTION_EVENT_LOCAL_ADDRESS_CHANGED:
        return p_visitor(
            accessor::from_raw<w_connection_event_local_address_changed>(
                _event));

      case QUIC_CONNECTION_EVENT_PEER_ADDRESS_CHANGED:
        return p_visitor(
            accessor::from_raw<w_connection_event_peer_address_changed>(
                _event));

      case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
        return p_visitor(
            accessor::from_raw<w_connection_event_peer_stream_started>(_event));

      case QUIC_CONNECTION_EVENT_STREAMS_AVAILABLE:
        return p_visitor(
            accessor::from_raw<w_connection_event_streams_available>(_event));

      case QUIC_CONNECTION_EVENT_IDEAL_PROCESSOR_CHANGED:
        return p_visitor(
            accessor::from_raw<w_connection_event_ideal_processor_changed>(
                _event));

      case QUIC_CONNECTION_EVENT_DATAGRAM_STATE_CHANGED:
        return p_visitor(
            accessor::from_raw<w_connection_event_datagram_state_changed>(
                _event));

      case QUIC_CONNECTION_EVENT_RESUMED:
        return p_visitor(
            accessor::from_raw<w_connection_event_resumed>(_event));

      case QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED:
        return p_visitor(
            accessor::from_raw<w_connection_event_resumption_ticket_received>(
                _event));

      case QUIC_CONNECTION_EVENT_PEER_CERTIFICATE_RECEIVED:
        return p_visitor(
            accessor::from_raw<w_connection_event_peer_certificate_received>(
                _event));

      case QUIC_CONNECTION_EVENT_DATAGRAM_RECEIVED:
        return p_visitor(
            accessor::from_raw<w_connection_event_datagram_received>(_event));

      case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
        return p_visitor(
            accessor::from_raw<
                w_connection_event_shutdown_initiated_by_transport>(_event));

      case QUIC_CONNECTION_EVENT_DATAGRAM_SEND_STATE_CHANGED:
        return p_visitor(
            accessor::from_raw<w_connection_event_datagram_send_state_changed>(
                _event));

      case QUIC_CONNECTION_EVENT_PEER_NEEDS_STREAMS:
        return p_visitor(
            accessor::from_raw<w_connection_event_peer_needs_streams>(_event));

      case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        return p_visitor(
            accessor::from_raw<w_connection_event_shutdown_complete>(_event));
    }

    return p_visitor(std::monostate{});
  }

 private:
  /**
   * @brief wrap the given raw event as non-owning.
   * @param p_event  pointer to event.
   */
  w_connection_event(internal::w_raw_tag, QUIC_CONNECTION_EVENT* p_event)
      : _event(p_event) {}

  QUIC_CONNECTION_EVENT* _event = nullptr;
};

}  // namespace wolf::stream::quic
