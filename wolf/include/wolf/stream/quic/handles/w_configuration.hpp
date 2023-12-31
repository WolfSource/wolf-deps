#pragma once

#include "stream/quic/internal/common.hpp"
#include "stream/quic/datatypes/w_status.hpp"
#include "stream/quic/datatypes/w_settings.hpp"
#include "stream/quic/datatypes/w_alpn.hpp"
#include "stream/quic/datatypes/w_credential_config.hpp"
#include "stream/quic/handles/w_registration.hpp"

#include <boost/leaf.hpp>
#include <msquic.h>

#include <span>

namespace wolf::stream::quic {

/**
 * @brief The wrapper of configuration concept of msquic.
 *
 * The configuration is quite bound to the registration,
 * it must be closed or destructed before registration being closed/destructed.
 *
 * @note make sure your registration handle outlives the configuration handle,
 *       or you might face deadlocks.
 */
class W_API w_configuration {
    friend class internal::w_raw_access;

public:
    /**
     * @brief constructs an empty handle.
     * @note use static factory function `open` to create valid handle.
     */
    w_configuration() {}

    w_configuration(const w_configuration&) = delete;
    w_configuration(w_configuration&& p_other) noexcept
        : _handle(std::exchange(p_other._handle, nullptr))
        , _api(std::move(p_other._api))
    {}

    w_configuration& operator=(const w_configuration&) = delete;
    w_configuration& operator=(w_configuration&& p_other) noexcept
    {
        std::swap(_handle, p_other._handle);
        std::swap(_api, p_other._api);
        return *this;
    }

    ~w_configuration() { close(); }

    /**
     * @brief whether the handle is open/valid or not.
     */
    [[nodiscard]] bool is_valid() const noexcept { return _handle; }

    /**
     * @brief open/create a configuration with given registration, settings and alpn.
     */
    [[nodiscard]] static auto open(w_quic_context p_context,
                                   w_registration& p_reg,
                                   const w_settings& p_settings,
                                   w_alpn_view p_alpn) noexcept
        -> boost::leaf::result<w_configuration>;

    /**
     * @brief open/create a configuration with given registration, settings,
     *        alpn and credential config.
     */
    [[nodiscard]] static auto open(w_quic_context p_context,
                                   w_registration& p_reg,
                                   const w_settings& p_settings,
                                   w_alpn_view p_alpn,
                                   const w_credential_config& p_cred_conf) noexcept
        -> boost::leaf::result<w_configuration>;

    /**
     * @brief load the given certificate credential into the configuration.
     * @param p_creds_config  certificate credential config.
     * @return status whether it succeeded or failed.
     */
    w_status load_credential(const w_credential_config& p_creds_config);

    /**
     * @brief close the configuration handle.
     */
    void close();

private:
    auto raw() noexcept { return _handle; }
    auto raw() const noexcept { return _handle; }

    explicit w_configuration(internal::w_raw_tag,
                             HQUIC p_handle,
                             std::shared_ptr<const QUIC_API_TABLE> p_api) noexcept
        : _handle(p_handle)
        , _api(std::move(p_api))
    {}

    HQUIC _handle = nullptr;
    std::shared_ptr<const QUIC_API_TABLE> _api{};
};

}  // namespace wolf::stream::quic
