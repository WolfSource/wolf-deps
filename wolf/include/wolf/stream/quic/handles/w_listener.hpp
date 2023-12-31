#pragma once

#include "stream/quic/internal/common.hpp"
#include "stream/quic/datatypes/w_status.hpp"
#include "stream/quic/datatypes/w_alpn.hpp"
#include "stream/quic/events/w_listener_event.hpp"
#include "stream/quic/handles/w_registration.hpp"
#include "stream/quic/w_quic_context.hpp"

#include <boost/leaf.hpp>
#include <msquic.h>

#include <atomic>
#include <span>
#include <tuple>

namespace wolf::stream::quic {

/**
 * @brief The wrapper of listener handle of msquic.
 */
class W_API w_listener {
    friend internal::w_raw_access;

public:
    using callback_type = std::function<w_status(w_listener&, w_listener_event&)>;

private:
    using raw_callback_type = QUIC_STATUS(HQUIC, void*, QUIC_LISTENER_EVENT*);

    struct context_bundle {
        callback_type callback;
        w_quic_context api_context;
        std::atomic<bool> running = false;
        std::atomic<int> refcount = 0;
        bool closed = false; // to avoid double free in reentrancy of close() in callback.
    };

    static QUIC_STATUS internal_raw_callback(HQUIC p_listener_raw,
                                             void* p_context_raw,
                                             QUIC_LISTENER_EVENT* p_event_raw);

public:
    /**
     * @brief construct an empty handle.
     * @note use static factory function `open` to create a valid handle.
     */
    w_listener() {}

    w_listener(const w_listener&) = delete;
    w_listener(w_listener&& p_other) noexcept
        : _handle(std::exchange(p_other._handle, nullptr))
        , _api(std::move(p_other._api))
    {}

    w_listener& operator=(const w_listener&) = delete;
    w_listener& operator=(w_listener&& p_other) noexcept
    {
        std::swap(_handle, p_other._handle);
        std::swap(_api, p_other._api);
        return *this;
    }

    ~w_listener() { close(); }

    /**
     * @brief whether the handle is open/valid or not.
     */
    [[nodiscard]] bool is_valid() const noexcept { return _handle; }

    /**
     * @brief whether it has been started and running.
     */
    [[nodiscard]] bool is_running() const noexcept;

    /**
     * @brief open/create a listener using a default context.
     */
    [[nodiscard]] static auto open(w_registration& p_reg,
                                   callback_type p_callback) noexcept
        -> boost::leaf::result<w_listener>
    {
        BOOST_LEAF_AUTO(context, w_quic_context::make());
        return open(std::move(context), p_reg, std::move(p_callback));
    }

    /**
     * @brief open/create a listener.
     */
    [[nodiscard]] static auto open(w_quic_context p_context,
                                   w_registration& p_reg,
                                   callback_type p_callback) noexcept
        -> boost::leaf::result<w_listener>;

    /**
     * @brief set callback.
     */
    auto set_callback(callback_type p_callback) -> boost::leaf::result<void>;

    /**
     * @brief start to listen.
     * @param p_address  address and port to listen on.
     * @param p_alpn     application-layer protocol negotiation.
     */
    w_status start(const w_address& p_address, w_alpn_view p_alpn);

    /**
     * @brief stop the listener.
     *
     * This is a non-blocking call, completion will be notified
     * by stop_complete event in callback.
     */
    void stop();

    /**
     * @brief close the listener handle.
     *
     * after this the instance will be unusable.
     */
    void close();

private:
    auto raw() noexcept { return _handle; }
    auto raw() const noexcept { return _handle; }

    explicit w_listener(internal::w_raw_tag,
                        HQUIC p_handle,
                        std::shared_ptr<const QUIC_API_TABLE> p_api) noexcept;

    HQUIC _handle = nullptr;
    std::shared_ptr<const QUIC_API_TABLE> _api{};
};

}  // namespace wolf::stream::quic
