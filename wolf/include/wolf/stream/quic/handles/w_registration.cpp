#include "stream/quic/handles/w_registration.hpp"

#include "stream/quic/datatypes/w_status.hpp"
#include "stream/quic/w_quic_context.hpp"

#include "wolf.hpp"

namespace wolf::stream::quic {

auto w_registration::open(w_quic_context p_context) noexcept
    -> boost::leaf::result<w_registration> {
  HQUIC handle = nullptr;
  auto api = internal::w_raw_access::raw(p_context);

  w_status status = api->RegistrationOpen(nullptr, &handle);
  if (status.failed()) {
    return W_FAILURE(std::errc::operation_canceled,
                     wolf::format("couldn't open/create registration: {}", status_to_str(status)));
  }

  return w_registration(internal::w_raw_tag{}, handle, std::move(api));
}

auto w_registration::open(w_quic_context p_context, const w_registration_config& p_config) noexcept
    -> boost::leaf::result<w_registration> {
  HQUIC handle = nullptr;
  auto api = internal::w_raw_access::raw(p_context);

  auto config_raw = internal::w_raw_access::raw(p_config);

  w_status status = api->RegistrationOpen(config_raw, &handle);
  if (status.failed()) {
    return W_FAILURE(std::errc::operation_canceled,
                     wolf::format("couldn't open/create registration: {}", status_to_str(status)));
  }

  return w_registration(internal::w_raw_tag{}, handle, std::move(api));
}

void w_registration::shutdown(QUIC_CONNECTION_SHUTDOWN_FLAGS p_flags, std::size_t p_error_code) {
  if (!_handle) {
    return;
  }

  _api->RegistrationShutdown(_handle, p_flags, p_error_code);
}

void w_registration::close() {
  if (!_handle) {
    return;
  }

  _api->RegistrationClose(_handle);
  _handle = nullptr;
}

}  // namespace wolf::stream::quic
