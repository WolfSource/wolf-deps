#include "uws.hpp"

#ifdef WOLF_ENABLE_HTTP1_1_WS

#include <w_io/w_io.h>

uws::uws()
{
}

uws::~uws()
{
}

int uws::run(const bool pSSL,
    const char* pCertFilePath,
    const char* pPrivateKeyFilePath,
    const char* pPassPhrase,
    const char* pRoot,
    const int pPort,
    uWS::CompressOptions pCompression,
    unsigned int pMaxPayloadLength,
    unsigned short pIdleTimeout,
    unsigned int pMaxBackPressure,
    std::function<void(const int)> pOnListened,
    std::function<bool(w_arg*)> pOnOpened,
    std::function<const char* (const char*, size_t, int*, w_arg*)> pOnMessage,
    std::function<void(const char*, size_t, int, w_arg*)> pOnClosed)
{
    if (pSSL)
    {
        if (w_io_file_check_is_file(pCertFilePath))
        {
            return W_FAILURE;
        }
        if (w_io_file_check_is_file(pPrivateKeyFilePath))
        {
            return W_FAILURE;
        }

        uWS::SSLApp(
            {
                .key_file_name = pPrivateKeyFilePath,
                .cert_file_name = pCertFilePath,
                .passphrase = pPassPhrase,
                .ssl_prefer_low_memory_usage = 1
            }).ws<w_arg*>(pRoot,
                {
                    /* Settings */
                    .compression = pCompression,
                    .maxPayloadLength = pMaxPayloadLength,
                    .idleTimeout = pIdleTimeout,
                    .maxBackpressure = pMaxBackPressure,
                    /* Handlers */
                    .open = [&](auto* pWS)
                    {
                        //get per socket data
                        w_arg _ptr = NULL;
                        auto _ret = pOnOpened(&_ptr);
                        if (_ret)
                        {
                            //ok
                            auto _per_socket_data = (w_arg*)pWS->getUserData();
                            *_per_socket_data = _ptr;
                        }
                        else
                        {
                            //close websocket
                            pWS->close();
                        }
                    },
                    .message = [&](auto* pWS, std::string_view pMessage, uWS::OpCode pOpCode)
                    {
                        auto _per_socket_data = (w_arg*)pWS->getUserData();
                        auto _res = pOnMessage(pMessage.data(), pMessage.size(), (int*)&pOpCode, _per_socket_data);
                        if (_res == NULL)
                        {
                            //close websocket
                            pWS->close();
                        }
                        else
                        {
                            pWS->send(_res, pOpCode, /*compress*/true);
                        }
                    },
                    .close = [&](auto* pWS, int pExitCode, std::string_view pMessage)
                    {
                        auto _per_socket_data = (w_arg*)pWS->getUserData();
                        pOnClosed(pMessage.data(), pMessage.size(), pExitCode, _per_socket_data);
                    }
                }).listen(pPort, [&](auto* pToken)
                    {
                        pOnListened(pPort);
                    }).run();
    }
    else
    {
        uWS::App(
            {
                .passphrase = pPassPhrase
            }).ws<w_arg*>(pRoot,
                {
                    /* Settings */
                    .compression = pCompression,
                    .maxPayloadLength = pMaxPayloadLength,
                    .idleTimeout = pIdleTimeout,
                    .maxBackpressure = pMaxBackPressure,
                    /* Handlers */
                    .open = [&](auto* pWS)
                    {
                    //get per socket data
                    w_arg _ptr = NULL;
                    pOnOpened(&_ptr);
                    auto _per_socket_data = (w_arg*)pWS->getUserData();
                    *_per_socket_data = _ptr;
                },
                .message = [&](auto* pWS, std::string_view pMessage, uWS::OpCode pOpCode)
                {
                    auto _per_socket_data = (w_arg*)pWS->getUserData();
                    auto _res = pOnMessage(pMessage.data(), pMessage.size(), (int*)&pOpCode, _per_socket_data);
                    if (_res == NULL)
                    {
                        //close websocket
                        pWS->close();
                    }
                    else
                    {
                        pWS->send(_res, pOpCode, /*compress*/true);
                    }
                },
                .close = [&](auto* pWS, int pExitCode, std::string_view pMessage)
                {
                    auto _per_socket_data = (w_arg*)pWS->getUserData();
                    pOnClosed(pMessage.data(), pMessage.size(), pExitCode, _per_socket_data);
                }
                }).listen(pPort, [&](auto* pToken)
                    {
                        pOnListened(pPort);
                    }).run();
    }
    return W_SUCCESS;
}

void uws::stop()
{
    auto loop = uWS::Loop::get();
    if (loop)
    {
        loop->free();
    }
}

#endif