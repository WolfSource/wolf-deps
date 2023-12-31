#include "stream/quic/handles/w_listener.hpp"

// NOTE: this header must be included after msquic.h
//       or there would be a redefinition error.
#include "wolf.hpp"

namespace wolf::stream::quic {

QUIC_STATUS w_listener::internal_raw_callback(HQUIC p_listener_raw,
                                              void* p_context_raw,
                                              QUIC_LISTENER_EVENT* p_event_raw)
{
    if (!p_context_raw) {
        return QUIC_STATUS_INTERNAL_ERROR;
    }

    auto context = static_cast<context_bundle*>(p_context_raw);
    auto api = internal::w_raw_access::raw(context->api_context);

    auto listener = w_listener(internal::w_raw_tag{}, p_listener_raw, api);
    auto event = internal::w_raw_access::from_raw<w_listener_event>(p_event_raw);

    w_status status = context->callback(listener, event);

    if (p_event_raw->Type == QUIC_LISTENER_EVENT_STOP_COMPLETE) {
        context->running = false;
        --context->refcount;
    }

    return status;
}

bool w_listener::is_running() const noexcept
{
    if (!_handle) {
        return false;
    }

    auto context = static_cast<context_bundle*>(_api->GetContext(_handle));
    return context->running;
}

auto w_listener::open(w_quic_context p_context, w_registration& p_reg, callback_type p_callback) noexcept
    -> boost::leaf::result<w_listener>
{
    HQUIC handle = nullptr;

    auto api = internal::w_raw_access::raw(p_context);

    auto reg_raw = internal::w_raw_access::raw(p_reg);
    if (!reg_raw) {
        return W_FAILURE(std::errc::invalid_argument,
                         "given registration is not valid.");
    }

    auto context_ptr = new context_bundle{};
    if (!context_ptr) {
        return W_FAILURE(std::errc::not_enough_memory,
                         "couldn't allocate for callback context.");
    }

    w_status status = api->ListenerOpen(reg_raw,
                                        (QUIC_LISTENER_CALLBACK_HANDLER)internal_raw_callback,
                                        (void*)context_ptr,
                                        &handle);
    if (status.failed()) {
        delete context_ptr;
        return W_FAILURE(std::errc::operation_canceled,
                         wolf::format("couldn't open/create listener: {}", status_to_str(status)));
    }

    context_ptr->callback = std::move(p_callback);
    context_ptr->api_context = std::move(p_context);

    return w_listener(internal::w_raw_tag{}, handle, std::move(api));
}

auto w_listener::set_callback(callback_type p_callback) -> boost::leaf::result<void>
{
    if (!_handle) {
        return W_FAILURE(std::errc::operation_canceled, "listener is closed/destroyed.");
    }

    auto context = static_cast<context_bundle*>(_api->GetContext(_handle));

    if (!context) {
        return W_FAILURE(std::errc::operation_canceled, "listener is in invalid state.");
    }

    if (context->running) {
        return W_FAILURE(std::errc::operation_canceled, "listener is running.");
    }

    context->callback = std::move(p_callback);

    return {};
}

w_status w_listener::start(const w_address& p_address, w_alpn_view p_alpn)
{
    if (!_handle) {
        return w_status_code::InvalidState;
    }

    context_bundle* context = static_cast<context_bundle*>(_api->GetContext(_handle));
    if (!context) {
        return w_status_code::InvalidState;
    }

    auto address_raw = internal::w_raw_access::raw(p_address);
    auto alpn  = QUIC_BUFFER{
        .Length = (unsigned int)p_alpn.size(),
        .Buffer = p_alpn.data()
    };

    w_status status = _api->ListenerStart(_handle, &alpn, 1, address_raw);
    if (status.failed()) {
        return status;
    }

    context->running = true;
    ++context->refcount;

    return status;
}

void w_listener::stop()
{
    if (!_handle) {
        return;
    }

    auto context = static_cast<context_bundle*>(_api->GetContext(_handle));

    // if the context doesn't exist, it's already closed.
    if (!context || !context->running) {
        return;
    }

    _api->ListenerStop(_handle);
}

void w_listener::close()
{
    if (!_handle) {
        return;
    }

    auto context = static_cast<context_bundle*>(_api->GetContext(_handle));
    if (!context || context->closed) {
        return;
    }

    if (--context->refcount > 0) {
        return;
    }

    // making sure if callback calls close,
    // it wouldn't result in reentrancy in msquic api.
    context->closed = true;

    _api->ListenerClose(_handle);
    _handle = nullptr;
    delete context;
}

w_listener::w_listener(internal::w_raw_tag,
                       HQUIC p_handle,
                       std::shared_ptr<const QUIC_API_TABLE> p_api) noexcept
    : _handle(p_handle)
    , _api(std::move(p_api))
{
    auto context = static_cast<context_bundle*>(_api->GetContext(p_handle));
    ++context->refcount;
}

}  // namespace wolf::stream::quic
