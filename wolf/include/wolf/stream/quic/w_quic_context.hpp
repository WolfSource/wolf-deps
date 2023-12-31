#pragma once

#include "stream/quic/internal/common.hpp"
#include "stream/quic/datatypes/w_status.hpp"

#include <msquic.h>

#include "wolf.hpp"

#include <memory>

namespace wolf::stream::quic {

class w_quic_context {
    friend class internal::w_raw_access;

public:
    static auto make() -> boost::leaf::result<w_quic_context>
    {
        const QUIC_API_TABLE* api_table = nullptr;

        if (w_status status = MsQuicOpen2(&api_table); status.failed()) {
            auto err_str = wolf::format("failed to initialize msquic library: {}",
                                        status_to_str(status));
            return W_FAILURE(std::errc::operation_canceled, std::move(err_str));
        }

        return w_quic_context(internal::w_raw_tag{}, api_table);
    }

    w_quic_context() {}

private:
    auto raw() noexcept { return _api_table; }
    auto raw() const noexcept { return _api_table; }

    static void free_api_table(const QUIC_API_TABLE* p_api_table)
    {
        if (!p_api_table) {
            return;
        }

        MsQuicClose(p_api_table);
    }

    w_quic_context(internal::w_raw_tag, const QUIC_API_TABLE* p_api_table)
        : _api_table(p_api_table, free_api_table)
    {}

    std::shared_ptr<const QUIC_API_TABLE> _api_table{};
};

}  // namespace wolf::stream::quic
