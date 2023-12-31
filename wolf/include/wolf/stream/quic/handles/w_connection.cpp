#include "stream/quic/handles/w_connection.hpp"
#include "stream/quic/events/w_connection_event.hpp"

// NOTE: this header must be included after msquic.h
//       or there would be a redefinition error.
#include "wolf.hpp"

namespace wolf::stream::quic {

QUIC_STATUS w_connection::internal_raw_callback(HQUIC p_connection_raw, void* p_context_raw,
                                                QUIC_CONNECTION_EVENT* p_event_raw) {
  if (!p_context_raw) {
    return QUIC_STATUS_INTERNAL_ERROR;
  }

  auto context = static_cast<context_bundle*>(p_context_raw);

  auto connection = w_connection(internal::w_raw_tag{}, p_connection_raw, context->api_table);
  auto event = internal::w_raw_access::from_raw<w_connection_event>(p_event_raw);

  w_status status = context->callback(connection, event);

  if (p_event_raw->Type == QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE) {
    context->running = false;
    // not running anymore, thus no longer requires to be held for callback.
    --context->refcount;
  }

  return status;
}

bool w_connection::is_running() const noexcept {
  if (!_handle) {
    return false;
  }

  auto context = static_cast<context_bundle*>(_api->GetContext(_handle));
  return context->running;
}

auto w_connection::open(w_quic_context p_context, w_registration& p_reg,
                        callback_type p_callback) noexcept -> boost::leaf::result<w_connection> {
  HQUIC handle = nullptr;

  auto api = internal::w_raw_access::raw(p_context);

  auto reg_raw = internal::w_raw_access::raw(p_reg);
  if (!reg_raw) {
    return W_FAILURE(std::errc::invalid_argument, "given registration is not valid.");
  }

  auto [context_ptr, callback_ptr] = make_context_callback_ptrs(std::move(p_callback));
  if (!context_ptr || !callback_ptr) {
    return W_FAILURE(std::errc::not_enough_memory, "couldn't allocate for callback context.");
  }

  w_status status = api->ConnectionOpen(reg_raw, (QUIC_CONNECTION_CALLBACK_HANDLER)callback_ptr,
                                        (void*)context_ptr, &handle);
  if (status.failed()) {
    delete context_ptr;
    return W_FAILURE(std::errc::operation_canceled,
                     wolf::format("couldn't open/create connection: {}", status_to_str(status)));
  }

  context_ptr->api_table = api;

  return w_connection(internal::w_raw_tag{}, handle, std::move(api));
}

auto w_connection::set_callback(callback_type p_callback) -> boost::leaf::result<void> {
  if (!_handle) {
    return W_FAILURE(std::errc::operation_canceled, "connection is closed/destroyed.");
  }

  auto context = static_cast<context_bundle*>(_api->GetContext(_handle));

  if (!context) {
    return W_FAILURE(std::errc::operation_canceled, "connection is in invalid state.");
  }

  if (context->running) {
    return W_FAILURE(std::errc::operation_canceled,
                     "can't set callback when connection is active/running.");
  }

  context->callback = std::move(p_callback);

  return {};
}

w_status w_connection::start(w_configuration& p_config, const char* p_host, std::uint16_t p_port) {
  if (!_handle) {
    return w_status_code::InvalidState;
  }

  context_bundle* context = static_cast<context_bundle*>(_api->GetContext(_handle));
  if (!context || context->closing || context->running) {
    return w_status_code::InvalidState;
  }

  auto config_raw = internal::w_raw_access::raw(p_config);

  w_status status =
      _api->ConnectionStart(_handle, config_raw, QUIC_ADDRESS_FAMILY_UNSPEC, p_host, p_port);

  if (status.failed()) {
    return status;
  }

  context->running = true;
  // account one more for running connections to
  // be passed into the callback.
  ++context->refcount;

  return status;
}

void w_connection::shutdown(QUIC_CONNECTION_SHUTDOWN_FLAGS p_flags, std::size_t p_error_code) {
  if (!_handle) {
    return;
  }

  auto context = static_cast<context_bundle*>(_api->GetContext(_handle));
  if (!context || !context->running) {
    return;
  }

  _api->ConnectionShutdown(_handle, p_flags, p_error_code);
}

void w_connection::close() {
  if (!_handle) {
    return;
  }

  auto context = static_cast<context_bundle*>(_api->GetContext(_handle));
  if (!context || context->closing) {
    return;
  }

  if (--context->refcount > 0) {
    return;
  }

  // makes sure if callback calls close,
  // it wouldn't result in reentrancy in msquic api.
  context->closing = true;

  _api->ConnectionClose(_handle);
  _handle = nullptr;
  delete context;
}

auto w_connection::setup_new_raw_connection(w_quic_context p_context, HQUIC p_conn_raw,
                                            w_configuration& p_config,
                                            callback_type p_callback) noexcept
    -> boost::leaf::result<w_connection> {
  auto config_raw = internal::w_raw_access::raw(p_config);
  if (!config_raw) {
    return W_FAILURE(std::errc::invalid_argument, "given config is closed or invalid.");
  }

  auto api = internal::w_raw_access::raw(p_context);

  w_status status = api->ConnectionSetConfiguration(p_conn_raw, config_raw);
  if (status.failed()) {
    auto err_str =
        wolf::format("couldn't set the configuration on the connection: {}", status_to_str(status));
    return W_FAILURE(std::errc::operation_canceled, err_str);
  }

  auto [context_ptr, callback_ptr] = make_context_callback_ptrs(std::move(p_callback));
  if (!context_ptr) {
    return W_FAILURE(std::errc::not_enough_memory,
                     "couldn't allocate for connection callback context.");
  }

  api->SetCallbackHandler(p_conn_raw, (void*)callback_ptr, context_ptr);

  context_ptr->api_table = api;
  context_ptr->running = true;
  ++context_ptr->refcount;

  return w_connection(internal::w_raw_tag{}, p_conn_raw, std::move(api));
}

w_connection::w_connection(internal::w_raw_tag, HQUIC p_handle,
                           std::shared_ptr<const QUIC_API_TABLE> p_api) noexcept
    : _handle(p_handle), _api(std::move(p_api)) {
  auto context = static_cast<context_bundle*>(_api->GetContext(p_handle));
  ++context->refcount;
}

}  // namespace wolf::stream::quic
