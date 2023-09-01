#pragma region Includes

#include "w_net.h"

//apr
#include <apr.h>
#include <apr_general.h>
#include <apr_network_io.h>

#include <w_io/w_io.h>
#include <w_log/w_log.h>
#include <w_memory/hash/uthash.h>
#include <w_concurrency/libev/ev.h>
#include <w_concurrency/w_thread.h>

#ifdef W_PLATFORM_WIN

#include <ws2tcpip.h>
#include <io.h>//_open_osfhandle
#include <wincrypt.h>
#include <inttypes.h>
#include <arch/win32/apr_arch_networkio.h>
#else
#include <arpa/inet.h>
#include <arch/unix/apr_arch_networkio.h>
#endif

#ifdef WOLF_ENABLE_QUIC
#include <quiche.h>
#endif

#ifdef WOLF_ENABLE_CURL
#include <curl/curl.h>
#endif

#ifdef WOLF_ENABLE_SSL
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#endif

#ifdef W_PLATFORM_UNIX
    #define SOCKET_ERROR    (-1)
    #include <netdb.h>
    #include <sys/fcntl.h>
#endif

#pragma endregion

#pragma region Defines

#ifdef WOLF_ENABLE_CURL

typedef struct
{
    w_mem_pool  pool;
    char*       memory;
    size_t      size;
} curl_memory;

#endif

#ifdef WOLF_ENABLE_QUIC

#define     QUICHE_LOCAL_CONN_ID_LEN            16
#define     QUICHE_MAX_DATAGRAM_SIZE            1350
#define     QUICHE_MAX_BUFFER_SIZE              65535
#define     QUICHE_MAX_TOKEN_LEN                \
            sizeof("quiche") - 1 +              \
            sizeof(struct sockaddr_storage) +   \
            QUICHE_MAX_CONN_ID_LEN

struct quic_connection
{
#ifdef W_PLATFORM_WIN
    SOCKET
#else
    int
#endif
        socket;//socket
    struct conn_io* connection_io;//connection io
#ifndef  W_PLATFORM_ANDROID
    quiche_config* config;//config
#endif
};

struct conn_io
{
    ev_timer                        timer;//timer
#ifdef W_PLATFORM_WIN
    SOCKET
#else
    int
#endif
        socket;//socket
    uint8_t                         connection_id[QUICHE_LOCAL_CONN_ID_LEN];//connection id
#ifndef  W_PLATFORM_ANDROID
    quiche_conn* connection;//quiche connection
#endif
    quic_stream_callback_fn         on_sending_stream_callback_fn;
    quic_stream_callback_fn         on_receiving_stream_callback_fn;
    struct sockaddr_storage         peer_addr;//peer address
    socklen_t                       peer_addr_len;//peer address len
    UT_hash_handle                  hh;//hash handle
};

static struct quic_connection* s_quic_conns = NULL;

#endif

#ifdef WOLF_ENABLE_SSL

typedef struct w_ssl_socket_t
{
    apr_socket_t*   socket;
    WOLFSSL*        ssl;
    WOLFSSL_CTX*    ssl_ctx;
} w_ssl_socket_t;
typedef w_ssl_socket_t* w_ssl_socket;

#endif

#pragma endregion

#pragma region Socket

W_RESULT w_net_get_ip_from_hostname(
    _In_z_ const char* pHostName,
    _Inout_ w_socket_address pSocketAddress)
{
    //NOTE: NOT IMPLEMENTED YET
    //const char* _trace_info = "w_net_get_ip_from_hostname";

    //if (!pHostName || !pSocketAddress)
    //{
    //    W_ASSERT_P(false, "invalid parameters! trace info: %s", _trace_info);
    //    return W_BAD_ARG;
    //}

    //return apr_sockaddr_ip_get(
    //    pHostName,
    //    (apr_sockaddr_t*)pSocketAddress);

    return W_FAILURE;
}

W_RESULT w_net_get_hostname_from_ip(
    _Inout_ w_socket_address pSocketAddress,
    _Inout_z_ char** pHostName)
{
    //NOTE: NOT IMPLEMENTED YET
    /*apr_getnameinfo(
        pHostName,
        apr_sockaddr_t * sa,
        apr_int32_t flags);*/
    return W_FAILURE;
}

W_RESULT w_net_socket_open(
    _In_ w_mem_pool pMemPool,
    _In_opt_z_ const char* pEndPoint,
    _In_ int pPort,
    _In_ w_socket_protocol pProtocol,
    _In_ w_socket_type pType,
    _In_ w_socket_family pFamily,
    _In_opt_ w_socket_options* pOptions,
    _Inout_ w_socket* pSocket)
{
    const char* _trace_info = "w_net_open_socket";
    *pSocket = NULL;

    if (!pMemPool || 
        pProtocol == W_SOCKET_PROTOCOL_QUIC_DIALER ||
        pProtocol == W_SOCKET_PROTOCOL_QUIC_LISTENER)
    {
        W_ASSERT_P(false, "invalid parameters! trace info: %s", _trace_info);
        return W_BAD_ARG;
    }

    int _family;
    apr_socket_t* _s = NULL;
    apr_sockaddr_t* _sa;
    apr_status_t _ret = W_FAILURE;
    apr_pool_t* _mem_pool = (apr_pool_t*)w_mem_pool_get_apr_pool(pMemPool);
    if (!_mem_pool)
    {
        return _ret;
    }

    bool _is_listener = (
        pProtocol == W_SOCKET_PROTOCOL_TCP_LISTENER ||
        pProtocol == W_SOCKET_PROTOCOL_UDP_LISTENER ||
        pProtocol == W_SOCKET_PROTOCOL_SCTP_LISTENER);


    int _protocol = pProtocol;
    if (_is_listener)
    {
        _protocol -= 1;
    }

    if (pFamily == W_SOCKET_FAMILY_UNSPEC)
    {
        goto exit;
    }
    else if (pFamily == W_SOCKET_FAMILY_IPV4)
    {
        _family = APR_INET;
    }
    else
    {
        _family = APR_INET6;
    }
    _ret = apr_sockaddr_info_get(
        &_sa, 
        pEndPoint,
        _family,
        pPort, 
        0, 
        _mem_pool);
    if (_ret)
    {
        goto exit;
    }

    _ret = apr_socket_create(
        &_s, 
        _sa->family, 
        pType,
        _protocol,
        _mem_pool);
    if (_ret)
    {
        goto exit;
    }

    if (pOptions)
    {
        _ret = apr_socket_opt_set(_s, APR_SO_NONBLOCK, (apr_int32_t)pOptions->non_blocking);
        if (_ret) goto exit;

        _ret = apr_socket_opt_set(_s, APR_SO_KEEPALIVE, (apr_int32_t)pOptions->keep_alive);
        if (_ret) goto exit;

        _ret = apr_socket_opt_set(_s, APR_TCP_NODELAY, (apr_int32_t)pOptions->no_delay);
        if (_ret) goto exit;

        _ret = apr_socket_timeout_set(_s, pOptions->timeout_microseconds);
        if (_ret) goto exit;
    }

    if (_is_listener)
    {
        _ret = apr_socket_opt_set(_s, APR_SO_REUSEADDR, 1);
        if (_ret) goto exit;

        _ret = apr_socket_bind(_s, _sa);
        if (_ret) goto exit;

        _ret = apr_socket_listen(_s, SOMAXCONN);
        if (_ret) goto exit;
    }
    else
    {
        _ret = apr_socket_connect(_s, _sa);
        if (_ret) goto exit;

        if (pOptions)
        {
            //we have to specify options again 
            _ret = apr_socket_opt_set(_s, APR_SO_NONBLOCK, (apr_int32_t)pOptions->non_blocking);
            if (_ret) goto exit;

            _ret = apr_socket_opt_set(_s, APR_SO_KEEPALIVE, (apr_int32_t)pOptions->keep_alive);
            if (_ret) goto exit;

            _ret = apr_socket_opt_set(_s, APR_TCP_NODELAY, (apr_int32_t)pOptions->no_delay);
            if (_ret) goto exit;

            _ret = apr_socket_timeout_set(_s, pOptions->timeout_microseconds);
            if (_ret) goto exit;
        }
    }

    *pSocket = _s;
    return W_SUCCESS;

exit:
    if (_s)
    {
        apr_socket_close(_s);
    }
    return (W_RESULT)_ret;
}

#ifdef WOLF_ENABLE_SSL

W_RESULT w_net_ssl_socket_open(
    _In_ w_mem_pool pMemPool,
    _In_opt_z_ const char* pEndPoint,
    _In_ int pPort,
    _In_z_ const char* pCertFilePath,
    _In_opt_z_ const char* pPrivateKeyFilePath,
    _In_ w_socket_protocol pProtocol,
    _In_ w_socket_type pType,
    _In_ w_socket_family pFamily,
    _In_opt_ w_socket_options* pOptions,
    _Inout_ w_ssl_socket* pSSLSocket)
{
    const char* _trace_info = "w_net_ssl_socket_open";

    W_RESULT _hr = W_SUCCESS;
    WOLFSSL_METHOD* _method = NULL;
    bool _serving = false;

    if (!pMemPool || 
        pProtocol == W_SOCKET_PROTOCOL_QUIC_DIALER ||
        pProtocol == W_SOCKET_PROTOCOL_QUIC_LISTENER)
    {
        W_ASSERT_P(false, "invalid parameters! trace info: %s", _trace_info);
        return W_BAD_ARG;
    }
    
    if (pSSLSocket && *pSSLSocket)
    {
        w_net_ssl_socket_close(pSSLSocket);
    }

    // we are going to use TLS v1.3
    if (pProtocol == W_SOCKET_PROTOCOL_TCP_LISTENER ||
        pProtocol == W_SOCKET_PROTOCOL_UDP_LISTENER ||
        pProtocol == W_SOCKET_PROTOCOL_SCTP_DIALER)
    {
        _serving = true;
        _method = wolfTLSv1_3_server_method();
    }
    else
    {
        _method = wolfTLSv1_3_client_method();
    }
    
    //first create w_ssl_socket    
    w_ssl_socket _ssl_socket = (w_ssl_socket_t*)w_calloc(pMemPool, sizeof(w_ssl_socket_t));
    if (!_ssl_socket)
    {
        W_ASSERT_P(false,
            "could not allocate memory for w_ssl_socket_t! trace info: %s",
            _trace_info);
        _hr = W_FAILURE;
        goto exit;
    }

    // create wolfSSL_CTX 
    if ((_ssl_socket->ssl_ctx = wolfSSL_CTX_new(_method)) == NULL)
    {
        W_ASSERT_P(false,
            "could not create wolfSSL tls 1.3 context! trace info: %s",
            _trace_info);
        return W_FAILURE;
    }

    if (_serving)
    {
        if (wolfSSL_CTX_use_certificate_file(
            _ssl_socket->ssl_ctx,
            pCertFilePath,
            SSL_FILETYPE_PEM) != SSL_SUCCESS)
        {
            W_ASSERT_P(false,
                "wolfSSL could not use %s for certificate file! trace info: %s",
                pCertFilePath,
                _trace_info);
            _hr = W_FAILURE;
            goto exit;
        }

        // Load server private key into ctx
        if (wolfSSL_CTX_use_PrivateKey_file(
            _ssl_socket->ssl_ctx,
            pPrivateKeyFilePath,
            SSL_FILETYPE_PEM) != SSL_SUCCESS)
        {
            W_ASSERT_P(false,
                "wolfSSL could not use %s for private key file! trace info: %s",
                pPrivateKeyFilePath,
                _trace_info);
            _hr = W_FAILURE;
            goto exit;
        }
    }
    else
    {
        // Add cert to ctx
        if (wolfSSL_CTX_load_verify_locations(
            _ssl_socket->ssl_ctx,
            pCertFilePath, 0) != SSL_SUCCESS)
        {
            W_ASSERT_P(false,
                "wolfSSL could not use %s for certificate file! trace info: %s",
                pCertFilePath,
                _trace_info);
            _hr = W_FAILURE;
            goto exit;
        }
    }
        
    //create socket
    if (w_net_socket_open(
        pMemPool,
        pEndPoint,
        pPort,
        pProtocol,
        pType,
        pFamily,
        pOptions,
        (w_socket*)(&_ssl_socket->socket)))
    {
        W_ASSERT_P(false,
            "could not open socket! trace info: %s",
            _trace_info);
        _hr = W_FAILURE;
        goto exit;
    }

    if (!_serving)
    {
        // create wolfSSL object 
        if ((_ssl_socket->ssl = wolfSSL_new(_ssl_socket->ssl_ctx)) == NULL)
        {
            W_ASSERT_P(false,
                "could not create wolfSSL object from wolfSSL context! trace info: %s",
                _trace_info);
            _hr = W_FAILURE;
            goto exit;
        }

        //attach wolfSSL to socket
        if (wolfSSL_set_fd(
            _ssl_socket->ssl,
            (int)(_ssl_socket->socket->socketdes)) != SSL_SUCCESS)
        {
            W_ASSERT_P(false,
                "could not set wolfSSL to the socket! trace info: %s",
                _trace_info);
            _hr = W_FAILURE;
            goto exit;
        }

        if (wolfSSL_connect(_ssl_socket->ssl) != SSL_SUCCESS)
        {
            W_ASSERT_P(false,
                "could not connect wolfSSL socket! trace info: %s",
                _trace_info);
            _hr = W_FAILURE;
            goto exit;
        }
    }

    *pSSLSocket = _ssl_socket;

    return W_SUCCESS;

exit:
    w_net_ssl_socket_close(&_ssl_socket);
    return _hr;
}

#endif

W_RESULT w_net_socket_close(_Inout_ w_socket* pSocket)
{
    const char* _trace_info = "w_net_close_tcp_socket";
    if (!pSocket || !*pSocket)
    {
        W_ASSERT_P(false, "invalid parameters! trace info: %s", _trace_info);
        return W_BAD_ARG;
    }

    apr_socket_t* _s = (apr_socket_t*)(*pSocket);
    apr_status_t _ret = apr_socket_close(_s);
    *pSocket = NULL;

    return (W_RESULT)_ret;
}

#ifdef WOLF_ENABLE_SSL

W_RESULT w_net_ssl_socket_close(_Inout_ w_ssl_socket* pSSLSocket)
{
    const char* _trace_info = "w_net_ssl_socket_close";
    if (!pSSLSocket || !*pSSLSocket)
    {
        W_ASSERT_P(false, "invalid parameters! trace info: %s", _trace_info);
        return W_BAD_ARG;
    }

    W_RESULT _ret = W_SUCCESS;

    if ((*pSSLSocket)->socket)
    {
        _ret = apr_socket_close((*pSSLSocket)->socket);
        (*pSSLSocket)->socket = NULL;
    }
    if ((*pSSLSocket)->ssl)
    {
        wolfSSL_free((*pSSLSocket)->ssl);
        (*pSSLSocket)->ssl = NULL;
    }

    if ((*pSSLSocket)->ssl_ctx)
    {
        wolfSSL_CTX_free((*pSSLSocket)->ssl_ctx);
        (*pSSLSocket)->ssl_ctx = NULL;
    }
    *pSSLSocket = NULL;

    return (W_RESULT)_ret;
}

void* s_ssl_socket_accept_callback(w_thread pThread, void* pArg)
{
    const char* _trace_info = "s_ssl_socket_accept_callback";

    W_RESULT _hr = W_SUCCESS;
    w_ssl_socket _ssl_socket = NULL;
    w_thread_job _ssl_callback_thread_job = NULL;

    apr_socket_t* _s = (apr_socket_t*)pArg;//accepted socket
    if (!_s)
    {
        W_ASSERT_P(false, "could not cast socket! trace info: %s", _trace_info);
        return NULL;
    }

    sock_userdata_t* _user_data_1 = _s->userdata;
    if (_user_data_1)
    {
        _ssl_socket = _s->userdata->data;
        sock_userdata_t* _user_data_2 = _user_data_1->next;
        if (_user_data_2)
        {
            _ssl_callback_thread_job = _user_data_2->data;
        }
    }

    //if ssl socket is not avaiable, just return
    if (!_ssl_socket) return NULL;
    
    // create wolfSSL object 
    if ((_ssl_socket->ssl = wolfSSL_new(_ssl_socket->ssl_ctx)) == NULL)
    {
        W_ASSERT_P(false,
            "could not create wolfSSL object from wolfSSL context! trace info: %s",
            _trace_info);
        _hr = W_FAILURE;
        goto exit;
    }

    //set wolfSSL fd
    if (wolfSSL_set_fd(
        _ssl_socket->ssl,
        (int)(_ssl_socket->socket->socketdes)) != SSL_SUCCESS)
    {
        W_ASSERT_P(false,
            "could not set wolfSSL to the socket! trace info: %s",
            _trace_info);
        _hr = W_FAILURE;
        goto exit;
    }

    // establish TLS connection
    if (wolfSSL_accept(_ssl_socket->ssl) != SSL_SUCCESS)
    {
        W_ASSERT_P(false,
            "wolfSSL could not accept! trace info: %s",
            _trace_info);
        _hr = W_FAILURE;
        goto exit;
    }

    if (_ssl_callback_thread_job)
    {
        w_thread _t = w_thread_get_current();
        _ssl_callback_thread_job(_t, (void*)_ssl_socket);
    }

    if (_ssl_socket->ssl) 
    {
        // free the wolfSSL object
        wolfSSL_free(_ssl_socket->ssl);
        _ssl_socket->ssl = NULL;
    }

    return NULL;

exit:

    w_net_ssl_socket_close(&_ssl_socket);

    return NULL;
}

#endif

W_RESULT s_socket_accept(
    _Inout_ w_mem_pool pMemPool,
    _In_ w_socket pSocket,
    _In_ w_socket_options* pOptions,
    _In_ bool pAsync,
    _In_ w_thread_job pOnAcceptCallback,
    _In_opt_ void* pUserData1,
    _In_opt_ void* pUserData2)
{
    const char* _trace_info = "s_socket_accept";
    if (!pMemPool || !pSocket || !pOnAcceptCallback)
    {
        W_ASSERT_P(false, "invalid parameters! trace info: %s", _trace_info);
        return W_BAD_ARG;
    }

    apr_pool_t* _pool = w_mem_pool_get_apr_pool(pMemPool);
    if (!_pool)
    {
        return W_BAD_ARG;
    }

    apr_socket_t* _ns;//new accepted socket
    apr_status_t _ret = apr_socket_accept(&_ns, pSocket, _pool);
    if (_ret)
    {
        return _ret;
    }

    if (pOptions)
    {
        _ret = apr_socket_opt_set(_ns, APR_SO_NONBLOCK, (apr_int32_t)pOptions->non_blocking);
        if (_ret) goto exit;

        _ret = apr_socket_opt_set(_ns, APR_SO_KEEPALIVE, (apr_int32_t)pOptions->keep_alive);
        if (_ret) goto exit;

        _ret = apr_socket_opt_set(_ns, APR_TCP_NODELAY, (apr_int32_t)pOptions->no_delay);
        if (_ret) goto exit;

        _ret = apr_socket_timeout_set(_ns, pOptions->timeout_microseconds);
        if (_ret) goto exit;
    }

    //store user data
    sock_userdata_t* _userdata_1 = NULL;
    sock_userdata_t* _userdata_2 = NULL;

    if (pUserData1)
    {
        _userdata_1 = w_malloc(
            pMemPool,
            sizeof(sock_userdata_t));
        _userdata_1->data = pUserData1;
        _userdata_1->key = "pSSLSocket";

        if (pUserData2)
        {
            _userdata_2 = w_malloc(
                pMemPool,
                sizeof(sock_userdata_t));
            _userdata_2->data = pUserData2;
            _userdata_2->key = "pOnAcceptCallback";
            _userdata_1->next = _userdata_2;
        }

        _ns->userdata = _userdata_1;
    }

    if (pAsync)
    {
        //Create the new thread
        w_thread _t = NULL;
        _ret = w_thread_init(pMemPool, &_t, pOnAcceptCallback, _ns);
        if (_ret)
        {
            goto exit;
        }
    }
    else
    {
        w_thread _t = w_thread_get_current();
        pOnAcceptCallback(_t, _ns);
    }

    return W_SUCCESS;

exit:
    if (_ns)
    {
        w_net_socket_close(&_ns);
    }
    return _ret;
}

W_RESULT w_net_socket_accept(
    _Inout_ w_mem_pool pMemPool,
    _In_ w_socket pSocket,
    _In_ w_socket_options* pOptions,
    _In_ bool pAsync,
    _In_ w_thread_job pOnAcceptCallback)
{
    return s_socket_accept(
        pMemPool,
        pSocket,
        pOptions,
        pAsync,
        pOnAcceptCallback,
        NULL,
        NULL);
}

#ifdef WOLF_ENABLE_SSL

W_RESULT w_net_ssl_socket_accept(
    _Inout_ w_mem_pool pMemPool,
    _In_ w_ssl_socket pSSLSocket,
    _In_ w_socket_options* pOptions,
    _In_ bool pAsync,
    _In_ w_thread_job pOnSSLAcceptCallback)
{
    const char* _trace_info = "w_net_ssl_socket_accept";
    if (!pMemPool || !pSSLSocket || !pSSLSocket->socket || !pOnSSLAcceptCallback)
    {
        W_ASSERT_P(false, "invalid parameters! trace info: %s", _trace_info);
        return W_BAD_ARG;
    }

    return s_socket_accept(
        pMemPool,
        pSSLSocket->socket,
        pOptions,
        pAsync,
        s_ssl_socket_accept_callback,
        pSSLSocket,//send ssl accept callback
        pOnSSLAcceptCallback//send SLLSocekt
    );
}

#endif

int w_net_socket_send(
    _Inout_ w_socket pSocket,
    _In_ w_buffer pBuffer)
{
    const char* _trace_info = "w_net_socket_send";
    if (!pSocket || !pBuffer)
    {
        W_ASSERT_P(false, "invalid parameters! trace info: %s", _trace_info);
        return -1;
    }

    if (pBuffer->data && pBuffer->len)
    {
        return apr_socket_send(
            pSocket,
            (const char*)pBuffer->data,
            (apr_size_t*)(&pBuffer->len));
    }
    return -1;
}

#ifdef WOLF_ENABLE_SSL

int w_net_ssl_socket_send(
    _Inout_ w_ssl_socket pSSLSocket,
    _In_ w_buffer pBuffer)
{
    const char* _trace_info = "w_net_ssl_socket_send";
    if (!pSSLSocket || !pBuffer)
    {
        W_ASSERT_P(false, "invalid parameters! trace info: %s", _trace_info);
        return -1;
    }

    if (pBuffer->data && pBuffer->len)
    {
        return wolfSSL_write(
            pSSLSocket->ssl,
            (const void*)pBuffer->data,
            (int)pBuffer->len);
    }
    return -1;
}

#endif

int w_net_socket_read(
    _Inout_ w_socket pSocket,
    _Inout_ w_buffer pBuffer)
{
    const char* _trace_info = "w_net_socket_read";

    if (!pSocket || !pBuffer)
    {
        W_ASSERT_P(false, "invalid parameters! trace info: %s", _trace_info);
        return -1;
    }

    long long _l = (long long)pBuffer->len - 1;// -1 for a '\0'
    if (_l <= 0)
    {
        return -1;
    }

    apr_size_t _len = (apr_size_t)_l;
    apr_status_t _ret = apr_socket_recv(pSocket, (char*)pBuffer->data, &_len);
    pBuffer->len = _len;
    if (_ret == APR_EOF || _len < 0) 
    {
        return -1;
    }
    pBuffer->data[_len] = '\0';
    
    return (int)_len;
}

#ifdef WOLF_ENABLE_SSL

int w_net_ssl_socket_read(
    _Inout_ w_ssl_socket pSSLSocket,
    _Inout_ w_buffer pBuffer)
{
    const char* _trace_info = "w_net_ssl_socket_read";

    if (!pSSLSocket || !pBuffer)
    {
        W_ASSERT_P(false, "invalid parameters! trace info: %s", _trace_info);
        return -1;
    }

    int _buffer_len = (int)pBuffer->len - 1;// -1 for a '\0'
    if (_buffer_len <= 0)
    {
        return -1;
    }

    int _len = wolfSSL_read(pSSLSocket->ssl, (void*)pBuffer->data, _buffer_len);
    pBuffer->len = _len;
    if (_len < 0)
    {
        return -1;
    }
    pBuffer->data[_len] = '\0';

    return _len;
}
#endif

#ifdef WOLF_ENABLE_HTTP1_1_WS

W_RESULT w_net_ws_init(_Inout_ ws* pWebSocket)
{
    ws _ws = ws_init();
    *pWebSocket = _ws;
    return W_SUCCESS;
}

W_RESULT w_net_ws_run(
    _In_ ws pWebSocket,
    _In_ bool pSSL,
    _In_z_ const char* pCertFilePath,
    _In_z_ const char* pPrivateKeyFilePath,
    _In_z_ const char* pPassPhrase,
    _In_z_ const char* pRoot,
    _In_ int pPort,
    _In_ int pCompression,
    _In_ int pMaxPayloadLength,
    _In_ const unsigned short pIdleTimeout,
    _In_ int pMaxBackPressure,
    _In_ ws_on_listened_fn pOnListened,
    _In_ ws_on_opened_fn pOnOpened,
    _In_ ws_on_message_fn pOnMessage,
    _In_ ws_on_closed_fn pOnClosed)
{
    const char* _trace_info = "w_net_run_ws_server";

    W_RESULT _rt;
    
    if (pWebSocket)
    {
        _rt = ws_run(pWebSocket,
            pSSL,
            pCertFilePath,
            pPrivateKeyFilePath,
            pPassPhrase,
            pRoot,
            pPort,
            pCompression,
            pMaxPayloadLength,
            pIdleTimeout,
            pMaxBackPressure,
            pOnListened,
            pOnOpened,
            pOnMessage,
            pOnClosed);
    }
    else
    {
        W_ASSERT_P(false, "websocket is NULL. trace info: %s", _trace_info);
        _rt = W_FAILURE;
    }    
    return _rt;
}

W_RESULT w_net_ws_stop(_In_ ws pWebSocket)
{
    const char* _trace_info = "w_net_ws_close";

    W_RESULT _rt = W_SUCCESS;
    if (pWebSocket)
    {
        ws_stop(pWebSocket);
    }
    else
    {
        W_ASSERT_P(false, "websocket is NULL. trace info: %s", _trace_info);
        _rt = W_FAILURE;
    }
    return _rt;
}

W_RESULT w_net_ws_free(_Inout_ ws* pWebSocket)
{
    const char* _trace_info = "w_net_ws_free";

    W_RESULT _rt = W_SUCCESS;
    if (pWebSocket && *pWebSocket)
    {
        ws_free(*pWebSocket);
        *pWebSocket = NULL;
    }
    else
    {
        W_ASSERT_P(false, "websocket is NULL. trace info: %s", _trace_info);
        _rt = W_FAILURE;
    }
    return _rt;
}

#endif

#pragma endregion

#pragma region QUIC

#ifdef WOLF_ENABLE_QUIC

static void s_quiche_flush(
    struct ev_loop* loop,
    struct conn_io* conn_io,
    w_socket_protocol pProtocol)
{
    const char* _trace_info = "w_net::s_quiche_flush";
	
#ifndef  W_PLATFORM_ANDROID

    static uint8_t _out[QUICHE_MAX_DATAGRAM_SIZE];

    while (1)
    {
        ssize_t _written = quiche_conn_send(
            conn_io->connection,
            _out,
            sizeof(_out));
        if (_written == QUICHE_ERR_DONE)
        {
            //fprintf(stderr, "done writing\n");
            break;
        }

        if (_written < 0)
        {
            W_ASSERT_P(false,
                "failed to create packet: %zd . trace info: %s",
                _written,
                _trace_info);
            return;
        }

        ssize_t sent;
        if (pProtocol == W_SOCKET_PROTOCOL_QUIC_LISTENER)
        {
            sent = sendto(
                conn_io->socket,
                (const char*)_out,
                (int)_written,
                0,
                (struct sockaddr*)&conn_io->peer_addr,
                conn_io->peer_addr_len);
        }
        else
        {
            sent = send(
                conn_io->socket,
                (const char*)_out,
                (int)_written,
                0);
        }

        if (sent != _written)
        {
            LOG_P(
                W_LOG_WARNING,
                "Failed to send. Written bytes %zd . trace info: %s",
                _written,
                _trace_info);
            return;
        }

        //fprintf(stderr, "sent %zd bytes\n", sent);
    }

    double t = quiche_conn_timeout_as_nanos(conn_io->connection) / 1e9f;
    conn_io->timer.repeat = t;
    ev_timer_again(loop, &conn_io->timer);
#endif
}

static void s_quiche_create_conn_timeout_callback(EV_P_ ev_timer* w, int pRevents)
{
    struct conn_io* _conn_io = (struct conn_io*)w->data;
    if (!_conn_io)
    {
        return;
    }
#ifndef  W_PLATFORM_ANDROID
    quiche_conn_on_timeout(_conn_io->connection);
    //fprintf(stderr, "timeout\n");
    s_quiche_flush(loop, _conn_io, W_SOCKET_PROTOCOL_QUIC_LISTENER);

    if (quiche_conn_is_closed(_conn_io->connection))
    {
        quiche_stats stats;

        quiche_conn_stats(_conn_io->connection, &stats);
        //logger(stderr, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu\n",
          //  stats.recv, stats.sent, stats.lost, stats.rtt, stats.cwnd);

        HASH_DELETE(hh, s_quic_conns->connection_io, _conn_io);

        ev_timer_stop(loop, &_conn_io->timer);
        quiche_conn_free(_conn_io->connection);
        free(_conn_io);

        return;
    }
#endif
}

static void s_quiche_dialler_timeout_callback(EV_P_ ev_timer* w, int revents)
{
#ifndef  W_PLATFORM_ANDROID
    struct conn_io* tmp = (struct conn_io*)w->data;
    quiche_conn_on_timeout(tmp->connection);

    fprintf(stderr, "timeout\n");

    s_quiche_flush(loop, tmp, W_SOCKET_PROTOCOL_QUIC_DIALER);

    if (quiche_conn_is_closed(tmp->connection))
    {
        quiche_stats stats;

        quiche_conn_stats(tmp->connection, &stats);

        fprintf(stderr, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns\n",
            stats.recv, stats.sent, stats.lost, stats.rtt);

        ev_break(EV_A_ EVBREAK_ONE);
        return;
    }
#endif
}

static void s_quiche_mint_token(
    const uint8_t* dcid, size_t dcid_len,
    struct sockaddr_storage* addr, socklen_t addr_len,
    uint8_t* token, size_t* token_len)
{
    memcpy(token, "quiche", sizeof("quiche") - 1);
    memcpy(token + sizeof("quiche") - 1, addr, addr_len);
    memcpy(token + sizeof("quiche") - 1 + addr_len, dcid, dcid_len);
    *token_len = sizeof("quiche") - 1 + addr_len + dcid_len;
}

static bool s_quiche_validate_token(
    const uint8_t* token, size_t token_len,
    struct sockaddr_storage* addr, socklen_t addr_len,
    uint8_t* odcid, size_t* odcid_len)
{
    if ((token_len < sizeof("quiche") - 1) ||
        memcmp(token, "quiche", sizeof("quiche") - 1))
    {
        return false;
    }

    token += sizeof("quiche") - 1;
    token_len -= sizeof("quiche") - 1;

    if ((token_len < addr_len) || memcmp(token, addr, addr_len))
    {
        return false;
    }

    token += addr_len;
    token_len -= addr_len;

    if (*odcid_len < token_len)
    {
        return false;
    }

    memcpy(odcid, token, token_len);
    *odcid_len = token_len;

    return true;
}

static struct conn_io* s_quiche_create_connection(
    uint8_t* dcid, size_t dcid_len,
    uint8_t* odcid, size_t odcid_len)
{
    struct conn_io* conn_io = (struct conn_io*)malloc(sizeof(*conn_io));
    if (conn_io == NULL) {
        fprintf(stderr, "failed to allocate connection IO\n");
        return NULL;
    }
	
#ifndef  W_PLATFORM_ANDROID
    
	memcpy(conn_io->connection_id, dcid, QUICHE_LOCAL_CONN_ID_LEN);
    quiche_conn* conn = quiche_accept(
        conn_io->connection_id,
        QUICHE_LOCAL_CONN_ID_LEN,
        odcid, odcid_len,
        s_quic_conns->config);
    if (conn == NULL)
    {
        fprintf(stderr, "failed to create connection\n");
        return NULL;
    }

    conn_io->socket = s_quic_conns->socket;
    conn_io->connection = conn;

    ev_init(&conn_io->timer, s_quiche_create_conn_timeout_callback);
    conn_io->timer.data = conn_io;

    HASH_ADD(hh, s_quic_conns->connection_io, connection_id,
        QUICHE_LOCAL_CONN_ID_LEN, conn_io);

    fprintf(stderr, "new connection\n");
#endif
    return conn_io;
}

static void s_quiche_listener_callback(EV_P_ ev_io* pIO, int pRevents)
{
    const char* _trace_info = "quiche_listener_callback";

    quic_stream_callback_fn _stream_callback_fn = (quic_stream_callback_fn)pIO->data;

    struct conn_io* _tmp, * _conn_io = NULL;
    static uint8_t buf[QUICHE_MAX_BUFFER_SIZE];
    static uint8_t out[QUICHE_MAX_DATAGRAM_SIZE];
#ifndef  W_PLATFORM_ANDROID
    while (1)
    {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(
            s_quic_conns->socket,
            (char*)buf,
            sizeof(buf),
            0,
            (struct sockaddr*)&peer_addr,
            &peer_addr_len);
        if (read == SOCKET_ERROR)
        {
#ifdef W_PLATFORM_WIN
            if (WSAGetLastError() == WSAEWOULDBLOCK)
#elif defined (W_PLATFORM_OSX) || defined (W_PLATFORM_IOS)
            if (errno == EWOULDBLOCK)
#endif
            {
                //logger("recvfrom blocked");
                break;
            }
            //logger ("failed to read");
            return;
        }

        uint8_t type;
        uint32_t version;

        uint8_t scid[QUICHE_MAX_CONN_ID_LEN];
        size_t scid_len = sizeof(scid);

        uint8_t dcid[QUICHE_MAX_CONN_ID_LEN];
        size_t dcid_len = sizeof(dcid);

        uint8_t odcid[QUICHE_MAX_CONN_ID_LEN];
        size_t odcid_len = sizeof(odcid);

        uint8_t token[QUICHE_MAX_TOKEN_LEN];
        size_t token_len = sizeof(token);

        int rc = quiche_header_info(
            buf,
            read,
            QUICHE_LOCAL_CONN_ID_LEN,
            &version,
            &type,
            scid,
            &scid_len,
            dcid,
            &dcid_len,
            token,
            &token_len);
        if (rc < 0)
        {
            W_ASSERT_P(false, "failed to parse header: %d", rc);
            continue;
        }

        HASH_FIND(hh, s_quic_conns->connection_io, dcid, dcid_len, _conn_io);

        if (_conn_io == NULL)
        {
            if (!quiche_version_is_supported(version))
            {
                //logger("quiche version is not supported");
                ssize_t written = quiche_negotiate_version(
                    scid, scid_len,
                    dcid, dcid_len,
                    out, sizeof(out));
                if (written < 0)
                {
                    W_ASSERT_P(false, "quiche_negotiate_version failed. create vneg packet: %zd failed", written);
                    continue;
                }

                ssize_t sent = sendto(
                    s_quic_conns->socket,
                    (const char*)out,
                    (int)written,
                    0,
                    (struct sockaddr*)&peer_addr, peer_addr_len);
                if (sent != written)
                {
                    W_ASSERT(false, "failed to send");
                    continue;
                }

                //"sent %zd bytes", sent;

                continue;
            }

            if (token_len == 0)
            {
                //logger(stderr, "stateless retry\n");

                s_quiche_mint_token(
                    dcid, dcid_len,
                    &peer_addr, peer_addr_len,
                    token, &token_len);

                uint8_t new_cid[QUICHE_LOCAL_CONN_ID_LEN];
                ssize_t written = quiche_retry(
                    scid, scid_len,
                    dcid, dcid_len,
                    new_cid, QUICHE_LOCAL_CONN_ID_LEN,
                    token, token_len,
                    version,
                    out, sizeof(out));
                if (written < 0)
                {
                    W_ASSERT_P(false, "failed to create retry packet: %zd", written);
                    continue;
                }

                ssize_t sent = sendto(
                    s_quic_conns->socket,
                    (const char*)out,
                    (int)written, 0,
                    (struct sockaddr*)&peer_addr, peer_addr_len);
                if (sent != written)
                {
                    W_ASSERT(false, "failed to send");
                    continue;
                }

                //logger (stderr, "sent %zd bytes\n", sent);
                continue;
            }

            if (!s_quiche_validate_token(
                token, token_len,
                &peer_addr, peer_addr_len,
                odcid, &odcid_len))
            {
                W_ASSERT(false, "invalid address validation token");
                continue;
            }

            _conn_io = s_quiche_create_connection(
                dcid, dcid_len,
                odcid, odcid_len);
            if (_conn_io == NULL)
            {
                continue;
            }

            memcpy(&_conn_io->peer_addr,
                &peer_addr,
                peer_addr_len);
            _conn_io->peer_addr_len = peer_addr_len;
        }

        //quiche_conn* _qcon = _qc->connection_io->connection;
        //if (!_qcon)
        //{
        //    W_ASSERT(false, "quiche_conn is NULL!");
        //    continue;
        //}
        ssize_t done = quiche_conn_recv(_conn_io->connection, buf, read);
        if (done < 0)
        {
            //logger(false, "failed to process packet: %zd", done);
            continue;
        }

        //loggerfprintf(stderr, "recv %zd bytes\n", done);

        if (quiche_conn_is_established(_conn_io->connection))
        {
            const uint8_t* _protocol;
            size_t _protocol_len;
            quiche_conn_application_proto(
                _conn_io->connection,
                &_protocol,
                &_protocol_len);

            W_RESULT _ret;
            uint64_t _quic_stream_index = 0;
            quiche_stream_iter* _readable_stream = quiche_conn_readable(_conn_io->connection);
            while (quiche_stream_iter_next(_readable_stream, &_quic_stream_index))
            {
                //fprintf(stderr, "stream %" PRIu64 " is readable\n", _quic_stream_index);

                if (_stream_callback_fn)
                {
                    _ret = _stream_callback_fn(
                        _conn_io->connection_id,
                        _quic_stream_index,
                        _protocol,
                        _protocol_len);
                    if (_ret == W_FAILURE)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }

                /*bool fin = false;
                for (size_t i = 0; i < 100; i++)
                {
                    ssize_t recv_len = quiche_conn_stream_recv(
                        _conn_io->connection,
                        _quic_stream_index,
                        buf, sizeof(buf),
                        &fin);
                    if (recv_len < 0)
                    {
                        break;
                    }
                    printf("\n\n%s\n\n", buf);
                }
                if (fin)
                {
                    static const char* resp = "byez\n";
                    quiche_conn_stream_send(
                        _conn_io->connection,
                        _quic_stream_index,
                        (uint8_t*)resp,
                        5,
                        true);
                }*/
            }
            quiche_stream_iter_free(_readable_stream);
        }
    }

    HASH_ITER(hh, s_quic_conns->connection_io, _conn_io, _tmp)
    {
        s_quiche_flush(loop, _conn_io, W_SOCKET_PROTOCOL_QUIC_LISTENER);

        if (quiche_conn_is_closed(_conn_io->connection))
        {
            quiche_stats stats;

            quiche_conn_stats(_conn_io->connection, &stats);
            // fprintf(stderr, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu\n",
              //   stats.recv, stats.sent, stats.lost, stats.rtt, stats.cwnd);

            HASH_DELETE(hh, s_quic_conns->connection_io, _conn_io);

            ev_timer_stop(loop, &_conn_io->timer);
            quiche_conn_free(_conn_io->connection);
            free(_conn_io);
        }
    }
#endif // ! W_PLATFORM_ANDROID
}

static void s_quiche_dialer_callback(EV_P_ ev_io* pIO, int pRevents)
{
    const char* _trace_info = "s_quiche_dialer_callback";

    if (!pIO)
    {
        W_ASSERT_P(false, "pIO in NULL. trace info: %s", _trace_info);
        return;
    }
	
#ifndef  W_PLATFORM_ANDROID	 
    
	struct conn_io* _conn_io = (struct conn_io*)pIO->data;
    if (!_conn_io)
    {
        W_ASSERT_P(false, "missing conn_io. trace info: %s", _trace_info);
        return;
    }

    static uint8_t buf[QUICHE_MAX_BUFFER_SIZE];

    while (1)
    {
        //read from socket
        ssize_t _read = recv(_conn_io->socket, buf, sizeof(buf), 0);
        if (_read == SOCKET_ERROR)
        {
#ifdef W_PLATFORM_WIN
            if (WSAGetLastError() == WSAEWOULDBLOCK)
#elif defined W_PLATFORM_OSX
            if (errno == EWOULDBLOCK)
#endif
            {
                //logger (stderr, "recv would block\n");
                break;
            }
            perror("failed to read");
            return;
        }

        //is quic connection recieved
        ssize_t _done = quiche_conn_recv(_conn_io->connection, buf, _read);
        if (_done < 0)
        {
            //logger (stderr, "failed to process packet\n");
            continue;
        }
        //logger (stderr, "recv %zd bytes\n", _done);
    }

    //logger (stderr, "done reading\n");

    //break ev loop if the quic connection closed
    if (quiche_conn_is_closed(_conn_io->connection))
    {
        //logger(stderr, "connection closed\n");
        ev_break(EV_A_ EVBREAK_ONE);
        return;
    }

    // on established quic server
    if (quiche_conn_is_established(_conn_io->connection))
    {
        const uint8_t* _protocol;
        size_t _protocol_len;
        quiche_conn_application_proto(
            _conn_io->connection,
            &_protocol,
            &_protocol_len);

        W_RESULT _rt;
        if (_conn_io->on_sending_stream_callback_fn)
        {
            _rt = _conn_io->on_sending_stream_callback_fn(
                _conn_io->connection_id,
                0,
                _protocol,
                _protocol_len);
        }

        uint64_t _stream_index = 0;
        quiche_stream_iter* _readable = quiche_conn_readable(_conn_io->connection);
        while (quiche_stream_iter_next(_readable, &_stream_index))
        {
            //logger fprintf(stderr, "stream %" PRIu64 " is readable\n", _stream_index);
            if (_conn_io->on_receiving_stream_callback_fn)
            {
                _rt = _conn_io->on_receiving_stream_callback_fn(
                    _conn_io->connection_id,
                    _stream_index,
                    _protocol,
                    _protocol_len);
                if (_rt == W_FAILURE)
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        quiche_stream_iter_free(_readable);
    }
    s_quiche_flush(loop, _conn_io, W_SOCKET_PROTOCOL_QUIC_DIALER);
#endif
}

void w_free_s_quic_conns()
{
#if defined (_WIN64) || defined (W_PLATFORM_OSX) || defined (W_PLATFORM_IOS) || defined (W_PLATFORM_LINUX)
    if (s_quic_conns)
    {
        s_quic_conns->config = NULL;
        if (s_quic_conns->connection_io)
        {
            quiche_conn_free(s_quic_conns->connection_io->connection);
            free(s_quic_conns->connection_io);
        }
        //free(s_quic_conns);
    }
#endif
}

W_RESULT w_net_quic_open(
    _In_z_  const char* pAddress,
    _In_        int pPort,
    _In_        w_socket_protocol pProtocol,
    _In_opt_z_  const char* pCertFilePath,
    _In_opt_z_  const char* pPrivateKeyFilePath,
    _In_opt_    quic_debug_log_callback_fn pQuicDebugLogCallback,
    _In_opt_    quic_stream_callback_fn pQuicReceivingStreamCallback,
    _In_opt_    quic_stream_callback_fn pQuicSendingStreamCallback)
{
    const char* _trace_info = "w_net_open_quic_socket";

    W_RESULT _ret = W_SUCCESS;

#if !defined(W_PLATFORM_IOS)
    
    if (pProtocol != W_SOCKET_PROTOCOL_QUIC_DIALER && 
        pProtocol != W_SOCKET_PROTOCOL_QUIC_LISTENER)
    {
        W_ASSERT_P(false,
            "pSocketMode must be one of the following enums: quic_dialer or quic_listener. trace info: %s",
            _trace_info);
        return W_BAD_ARG;
    }

#if defined (_WIN64) || defined (W_PLATFORM_OSX) || defined (W_PLATFORM_IOS) || defined (W_PLATFORM_LINUX)

    quiche_config* _quiche_config = NULL;
    struct addrinfo* _address_info = NULL;

#ifdef W_PLATFORM_WIN
    SOCKET
#else
    int
#endif
        _socket = 0;

    struct conn_io* _quiche_connection_io = NULL;
    struct ev_loop* _ev_loop = EV_DEFAULT;

    const struct addrinfo _hints =
    {
        .ai_family = PF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP
    };

    //convert integer port to string
    char* _port = (char*)malloc(6);//max port number is 65329
    if (!_port)
    {
        return W_FAILURE;
    }
    snprintf(_port, 6, "%d", pPort);

#ifdef W_PLATFORM_WIN
    WSADATA _wsa_data;
    WORD _version_requested = MAKEWORD(2, 2);
    int _result = WSAStartup(_version_requested, &_wsa_data);
    if (_result)
    {
        _ret = W_FAILURE;
        W_ASSERT_P(false, "WSAStartup failed. trace info: %s", _trace_info);
        goto out;
    }
#endif

    if (getaddrinfo(pAddress, _port, &_hints, &_address_info) != 0)
    {
        _ret = W_FAILURE;
        W_ASSERT_P(false, "failed to resolve host. trace info: %s", _trace_info);
        goto out;
    }

    //enable quic debug logging
    if (pQuicDebugLogCallback)
    {
        quiche_enable_debug_logging(pQuicDebugLogCallback, NULL);
    }

    //create socket
    _socket = socket(_address_info->ai_family, SOCK_DGRAM, 0);
    if (
#ifdef W_PLATFORM_WIN
        _socket == 0
#else
        _socket < 0
#endif
        )
    {
        _ret = W_FAILURE;
        W_ASSERT_P(false, "failed to create a socket. trace info: %s", _trace_info);
        goto out;
    }

    //set non blocking option
#ifdef W_PLATFORM_WIN
    u_long _non_blocking_enabled = TRUE;
    if (ioctlsocket(_socket, FIONBIO, &_non_blocking_enabled) != 0)
#else
    if (fcntl(_socket, F_SETFL, O_NONBLOCK) != 0)
#endif
    {
        _ret = W_FAILURE;
        W_ASSERT_P(false, "failed to make non-blocking socket. trace info: %s", _trace_info);
        goto out;
    }

    //bind or connect to endpoint
    uint32_t _quic_version;
    if (pProtocol == W_SOCKET_PROTOCOL_QUIC_LISTENER)
    {
        //bind socket to address
        if (bind(_socket, _address_info->ai_addr, (int)_address_info->ai_addrlen) < 0)
        {
            _ret = W_FAILURE;
            W_ASSERT_P(false, "failed to bind socket. trace info: %s", _trace_info);
            goto out;
        }

        _quic_version = QUICHE_PROTOCOL_VERSION;
    }
    else
    {
        //connect to endpoint address
        if (connect(
            _socket,
            _address_info->ai_addr,
            (int)_address_info->ai_addrlen) < 0)
        {
            _ret = W_FAILURE;
            W_ASSERT_P(false, "failed to connect endpoint socket. trace info: %s", _trace_info);
            goto out;
        }

        _quic_version = 0xbabababa;
    }

    //create quiche config
    _quiche_config = quiche_config_new(_quic_version);
    if (_quiche_config == NULL)
    {
        _ret = W_FAILURE;
        W_ASSERT_P(false, "failed to create config. trace info: %s", _trace_info);
        goto out;
    }

    //check for ssl
    if (pProtocol == W_SOCKET_PROTOCOL_QUIC_LISTENER)
    {
        if (pCertFilePath && pPrivateKeyFilePath &&
            w_io_file_check_is_file(pCertFilePath) == W_SUCCESS &&
            w_io_file_check_is_file(pPrivateKeyFilePath) == W_SUCCESS)
        {
            _ret = quiche_config_load_cert_chain_from_pem_file(
                _quiche_config,
                pCertFilePath);
            if (_ret)
            {
                _ret = W_FAILURE;
                W_ASSERT_P(false, "failed to load cert chain from pem file for quic. trace info: %s", _trace_info);
                goto out;
            }
            _ret = quiche_config_load_priv_key_from_pem_file(
                _quiche_config,
                pPrivateKeyFilePath);
            if (_ret)
            {
                _ret = W_FAILURE;
                W_ASSERT_P(false, "failed to load private key from pem file for quic. trace info: %s", _trace_info);
                goto out;
            }
        }
    }

    //use quic draft 27
    _ret = quiche_config_set_application_protos(_quiche_config,
        (uint8_t*)"\x05hq-29\x05hq-28\x05hq-27\x08http/0.9", 27);
    if (_ret)
    {
        _ret = W_FAILURE;
        W_ASSERT_P(false, "failed to set quic application protos. trace info: %s", _trace_info);
        goto out;
    }

    //set quiche configs
    quiche_config_set_max_idle_timeout(_quiche_config, 5000);
    quiche_config_set_max_udp_payload_size(_quiche_config, QUICHE_MAX_DATAGRAM_SIZE);
    quiche_config_set_initial_max_data(_quiche_config, 10000000);
    quiche_config_set_initial_max_stream_data_bidi_local(_quiche_config, 1000000);
    quiche_config_set_initial_max_streams_bidi(_quiche_config, 100);

    if (pProtocol == W_SOCKET_PROTOCOL_QUIC_LISTENER)
    {
        quiche_config_set_initial_max_stream_data_bidi_remote(_quiche_config, 1000000);
        quiche_config_set_cc_algorithm(_quiche_config, QUICHE_CC_RENO);
    }
    else
    {
        quiche_config_set_initial_max_stream_data_uni(_quiche_config, 1000000);
        quiche_config_set_initial_max_streams_uni(_quiche_config, 100);
        quiche_config_set_disable_active_migration(_quiche_config, true);

        if (getenv("SSLKEYLOGFILE"))
        {
            quiche_config_log_keys(_quiche_config);
        }

        uint8_t _scid[QUICHE_LOCAL_CONN_ID_LEN] = { 0 };

#ifdef W_PLATFORM_WIN

        HCRYPTPROV _crypto;

        if (CryptAcquireContext(
            &_crypto,
            NULL,
            NULL,
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT) == FALSE)
        {
            _ret = W_FAILURE;
            W_ASSERT_P(
                false,
                "CryptAcquireContext failed. trace info: %s",
                _trace_info);
            goto out;
        }

        if (CryptGenRandom(_crypto, sizeof(_scid), (BYTE*)_scid) == FALSE)
        {
            _ret = W_FAILURE;
            W_ASSERT_P(
                false,
                "CryptGenRandom failed. trace info: %s",
                _trace_info);
            goto out;
        }
#else

        int rng = open("/dev/urandom", O_RDONLY);
        if (rng < 0) {
            _ret = W_FAILURE;
            W_ASSERT_P(
                false,
                "failed to open /dev/urandom. trace info: %s",
                _trace_info);
            goto out;
        }

        ssize_t rand_len = read(rng, &_scid, sizeof(_scid));
        if (rand_len < 0) {
            _ret = W_FAILURE;
            W_ASSERT_P(
                false,
                "failed to create connection ID. trace info: %s",
                _trace_info);
            goto out;
        }

#endif

        _quiche_connection_io = (struct conn_io*)malloc(sizeof(struct conn_io));
        if (_quiche_connection_io == NULL)
        {
            _ret = W_FAILURE;
            W_ASSERT_P(
                false,
                "failed to create quiche connection io. trace info: %s",
                _trace_info);
            goto out;
        }

        _quiche_connection_io->connection = quiche_connect(
            pAddress,
            (const uint8_t*)_scid,
            sizeof(_scid),
            _quiche_config);
        if (_quiche_connection_io->connection == NULL)
        {
            _ret = W_FAILURE;
            W_ASSERT_P(
                false,
                "failed to create quiche conn. trace info: %s",
                _trace_info);
            goto out;
        }
    }

#ifdef W_PLATFORM_WIN
    int _osf_handle = _open_osfhandle(_socket, 0);
    if (_osf_handle < 0)
    {
        _ret = W_FAILURE;
        W_ASSERT_P(false, "failed to open_osfhandle. trace info: %s", _trace_info);
        goto out;
    }
#endif

    if (_quiche_connection_io)
    {
        _quiche_connection_io->socket = _socket;
        _quiche_connection_io->on_sending_stream_callback_fn = pQuicSendingStreamCallback;
        _quiche_connection_io->on_receiving_stream_callback_fn = pQuicReceivingStreamCallback;
    }

    if (_ev_loop)
    {
        ev_io _ev_watcher;
        if (pProtocol == W_SOCKET_PROTOCOL_QUIC_LISTENER)
        {
            w_free_s_quic_conns();

            //init ev_io for quic listener
#ifdef W_PLATFORM_WIN
            ev_io_init(
                &_ev_watcher,
                s_quiche_listener_callback,
                _osf_handle,
                EV_READ);
#elif defined W_PLATFORM_OSX
            ev_io_init(
                &_ev_watcher,
                s_quiche_listener_callback,
                _socket,
                EV_READ);
#endif
            ev_io_start(_ev_loop, &_ev_watcher);

            //set private data of ev watcher
            _ev_watcher.data = (void*)pQuicReceivingStreamCallback;
        }
        else
        {
            //init ev_io for quic dialer
#ifdef W_PLATFORM_WIN
            ev_io_init(
                &_ev_watcher,
                s_quiche_dialer_callback,
                _osf_handle,
                EV_READ);
#elif defined W_PLATFORM_OSX
            ev_io_init(
                &_ev_watcher,
                s_quiche_dialer_callback,
                _quiche_connection_io->socket,
                EV_READ);
#endif
            ev_io_start(_ev_loop, &_ev_watcher);

            //set private data of ev watcher
            _ev_watcher.data = _quiche_connection_io;

            ev_init(
                &_quiche_connection_io->timer,
                s_quiche_dialler_timeout_callback);
            _quiche_connection_io->timer.data = _quiche_connection_io;
            s_quiche_flush(_ev_loop, _quiche_connection_io, pProtocol);
        }

        struct quic_connection _c;
        _c.socket = _socket;
        _c.connection_io = _quiche_connection_io;

        s_quic_conns = &_c;
        s_quic_conns->config = _quiche_config;

        //start ev loop
        ev_loop(_ev_loop, 0);
    }

out:
    //free resources
    free(_port);
    if (_address_info)
    {
        freeaddrinfo(_address_info);
    }
    if (_socket)
    {
#ifdef W_PLATFORM_WIN
        closesocket(_socket);
#elif defined W_PLATFORM_OSX
        close(_socket);
#endif
    }
    if (_quiche_config)
    {
        quiche_config_free(_quiche_config);
    }
    w_free_s_quic_conns();

#endif
#endif

#if defined (W_PLATFORM_ANDROID) || defined(W_PLATFORM_IOS)
    return W_FAILURE;
#endif
    return _ret;
}

W_RESULT w_net_quic_close()
{
    if (!s_quic_conns || !s_quic_conns->connection_io)
    {
        return W_FAILURE;
    }

    int _ret = -1;

#if defined (_WIN64) || defined (W_PLATFORM_OSX) || defined (W_PLATFORM_IOS) || defined (W_PLATFORM_LINUX)
    
    quiche_conn* _qc = s_quic_conns->connection_io->connection;
    if (!_qc)
    {
        return W_FAILURE;
    }

    _ret = quiche_conn_close(
        _qc,
        true,
        0,
        NULL,
        0);
#endif
    return _ret < 0 ? W_FAILURE : W_SUCCESS;
}

size_t w_net_quic_send(_In_ uint8_t* pConnectionID,
    _In_ uint64_t pStreamIndex,
    _In_ w_buffer pBuffer,
    _In_ bool pFinish)
{

#if defined (_WIN64) || defined (W_PLATFORM_OSX) || defined (W_PLATFORM_IOS) || defined (W_PLATFORM_LINUX)
    //get quich connection based on pConnectionID
    if (!s_quic_conns || !s_quic_conns->connection_io)
    {
        return 0;
    }
    quiche_conn* _qc = s_quic_conns->connection_io->connection;
    if (!_qc)
    {
        return 0;
    }
    return quiche_conn_stream_send(
        _qc,
        pStreamIndex,
        pBuffer->data,
        pBuffer->len,
        pFinish);
#endif
}

size_t w_net_quic_receive(
    _In_ uint8_t* pConnectionID,
    _In_ uint64_t pStreamIndex,
    _Inout_ w_buffer pReceiveBuffer,
    _Inout_ bool* pIsStreamFinished)
{

#if defined (_WIN64) || defined (W_PLATFORM_OSX) || defined (W_PLATFORM_IOS) || defined (W_PLATFORM_LINUX)
 
    //get quich connection based on pConnectionID
    if (!s_quic_conns || !s_quic_conns->connection_io || !pReceiveBuffer)
    {
        return 0;
    }
    quiche_conn* _qc = s_quic_conns->connection_io->connection;
    if (!_qc)
    {
        return 0;
    }

    return quiche_conn_stream_recv(
        _qc,
        pStreamIndex,
        pReceiveBuffer->data,
        pReceiveBuffer->len,
        pIsStreamFinished);
#endif
}

#endif

#pragma endregion

#pragma region CURL

#ifdef WOLF_ENABLE_CURL

const char* w_net_url_encoded(
    _Inout_ w_mem_pool pMemPool,
    _In_z_ const char* pUrlAddress)
{
    const char* _trace_info = "w_net_url_encoded";
    if (!pMemPool)
    {
        W_ASSERT_P(false, "memory pool is invalid! trace info: %s", _trace_info);
        return NULL;
    }
    char* _encoded = NULL;

#if !defined(W_PLATFORM_IOS) && !defined(W_PLATFORM_ANDROID)

    CURL* _curl = curl_easy_init();
    if (!_curl)
    {
        W_ASSERT(false, "could not create curl handle. trace info: w_net_url_encoded");
        goto _out;
    }

    char* _output = curl_easy_escape(_curl, pUrlAddress, (int)strlen(pUrlAddress));
    if (_output)
    {
        //allocate memory for it from memory pool
        _encoded = w_malloc(pMemPool, strlen(_output));
        // make a copy and free the original string
        strcpy(_encoded, _output);
        curl_free(_output);
    }

_out:
    if (_curl)
    {
        curl_easy_cleanup(_curl);
    }
#endif
    return _encoded;
}

static size_t _curl_write_callback(
    void* pContents,
    size_t pSize,
    size_t pNmemb,
    void* pUserp)
{
    const char* _trace_info = "_curl_write_callback";

    size_t _real_size = pSize * pNmemb;
    if (!_real_size)
    {
        W_ASSERT_P(false, "_real_size is zero. trace info: %s", _trace_info);
        return 0;
    }
    curl_memory* _mem = (curl_memory*)pUserp;
    if (!_mem)
    {
        W_ASSERT_P(false, "missing curl memory. trace info: %s", _trace_info);
        return 0;
    }
    _mem->memory = (char*)w_malloc(
        _mem->pool,
        _mem->size + _real_size + 1);
    if (!_mem->memory)
    {
        //out of memory
        W_ASSERT_P(false,
            "out of curl_memory. trace info: %s",
            _trace_info);
        return 0;
    }

    memcpy(&(_mem->memory[_mem->size]), pContents, _real_size);
    _mem->size += _real_size;
    _mem->memory[_mem->size] = '\0';

    return _real_size;
}

W_RESULT w_net_http_send(
    _Inout_    w_mem_pool pMemPool,
    _In_z_     const char* pURL,
    _In_       w_http_request_type pHttpRequestType,
    _In_       w_array pHttpHeaders,
    _In_z_     const char* pMessage,
    _In_       size_t pMessageLenght,
    _In_       size_t pLowSpeedLimit,
    _In_       size_t pLowSpeedTimeInSec,
    _In_       float pTimeOutInSecs,
    _Inout_    long* pResponseCode,
    _Inout_z_  char** pResponseMessage,
    _Inout_    size_t* pResponseMessageLength)
{
    const char* _trace_info = "w_net_send_http_request";
    if (!pMemPool || !pURL)
    {
        W_ASSERT_P(false, "invalid parameters. trace info: %s", _trace_info);
        return APR_BADARG;
    }
#if !defined(W_PLATFORM_IOS) && !defined(W_PLATFORM_ANDROID)
    CURLcode _rt;
    curl_memory* _curl_mem = NULL;
    //create handle
    CURL* _curl = curl_easy_init();
    if (!_curl)
    {
        _rt = CURLE_FAILED_INIT;
        W_ASSERT(false, "could not create curl handle. trace info: curl_easy_init");
        goto _out;
    }

    _curl_mem = w_malloc(
        pMemPool,
        sizeof(curl_memory));
    if (!_curl_mem)
    {
        _rt = CURLE_OUT_OF_MEMORY;
        goto _out;
    }

    _curl_mem->memory = *pResponseMessage;
    _curl_mem->pool = pMemPool;
    _curl_mem->size = *pResponseMessageLength;

    double _timeout_in_ms = pTimeOutInSecs / 1000;

    //now specify the POST data
    switch (pHttpRequestType)
    {
    default:
    case HTTP_GET:
        break;
    case HTTP_POST:
        curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, pMessage);
        curl_easy_setopt(_curl, CURLOPT_POSTFIELDSIZE, pMessageLenght);
        curl_easy_setopt(_curl, CURLOPT_POST, 1L);
        break;
    }

    //set curl options
    curl_easy_setopt(_curl, CURLOPT_URL, pURL);
    curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(_curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(_curl, CURLOPT_CONNECTTIMEOUT_MS, _timeout_in_ms);
    curl_easy_setopt(_curl, CURLOPT_ACCEPTTIMEOUT_MS, _timeout_in_ms);
    curl_easy_setopt(_curl, CURLOPT_USERAGENT, "Mozilla/5.0");//libcurl-agent/1.0
    // abort if slower than bytes/sec
    curl_easy_setopt(_curl, CURLOPT_LOW_SPEED_LIMIT, pLowSpeedLimit);
    curl_easy_setopt(_curl, CURLOPT_LOW_SPEED_TIME, pLowSpeedTimeInSec);
    // store response message
    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, _curl_write_callback);
    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, (void*)_curl_mem);

    //set http header
    struct curl_slist* _headers = NULL;
    const char** _ppt;
    apr_array_header_t* temp;
    temp = ((apr_array_header_t*)&pHttpHeaders);
    while ((_ppt = apr_array_pop(temp)))
    {
        _headers = curl_slist_append(_headers, *_ppt);
    }
    if (_headers)
    {
        curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _headers);
    }

    // perform, then store the expected code in '_rt'
    _rt = curl_easy_perform(_curl);

    //free headers
    if (_headers)
    {
        curl_slist_free_all(_headers);
    }

    if (_rt == CURLE_OK)
    {
        curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &pResponseCode);
        //TODO: do we need to copy ?
            // _chunk.copyto(pResultPage);
        //        if (this->size && this->memory)
        //        {
        //            pDestination.resize(this->size);
        //            memcpy(&pDestination[0], &this->memory[0], this->size);
        //        }
    }

_out:
    if (_curl)
    {
        curl_easy_cleanup(_curl);
    }
    return _rt;
#endif
    return W_FAILURE;
}

const char* w_net_curl_get_last_error(_In_ W_RESULT pErrorCode)
{
#if !defined(W_PLATFORM_IOS) && !defined(W_PLATFORM_ANDROID)
    return curl_easy_strerror(pErrorCode);
#else
    return NULL;
#endif
}

#endif

#pragma endregion

#pragma region AMQ

#ifdef WOLF_ENABLE_ACTIVEMQ

void	w_net_amq_init()
{
    w_amq_init();
}

W_RESULT	w_net_amq_producer_open(
    _Inout_ w_mem_pool pMemPool,
    _Inout_ w_amq_object* pAMQObject,
    _In_z_ const char* pBrokerURI,
    _In_z_ const char* pUsername,
    _In_z_ const char* pPassword,
    _In_z_ const char* pQueueOrTopicName,
    _In_ amq_connection_type pConnectionType,
    _In_ amq_delivery_mode pDeliveryMode,
    _In_ bool pSessionTransacted)
{
    return w_amq_producer_open(
        pMemPool,
        pAMQObject,
        pBrokerURI,
        pUsername,
        pPassword,
        pQueueOrTopicName,
        pConnectionType,
        pDeliveryMode,
        pSessionTransacted);
}

W_RESULT	w_net_amq_consumer_open(
    _Inout_ w_mem_pool pMemPool,
    _Inout_ w_amq_object* pAMQObject,
    _In_z_  const char* pBrokerURI,
    _In_z_  const char* pUsername,
    _In_z_  const char* pPassword,
    _In_z_  const char* pQueueOrTopicName,
    _In_ amq_connection_type pConnectionType,
    _In_ bool pSessionTransacted,
    _In_ w_amq_consumer_callback_fn pOnMessageCallBack)
{
    return 	w_amq_consumer_open(
        pMemPool,
        pAMQObject,
        pBrokerURI,
        pUsername,
        pPassword,
        pQueueOrTopicName,
        pConnectionType,
        pSessionTransacted,
        pOnMessageCallBack);
}

W_RESULT	w_net_amq_producer_run(_In_ w_amq_object pAMQObject)
{
    return 	w_amq_producer_run(pAMQObject);
}

W_RESULT	w_net_amq_consumer_run(_In_ w_amq_object pAMQObject)
{
    return 	w_amq_consumer_run(pAMQObject);
}

W_RESULT	w_net_amq_producer_send_message(
    _In_ w_amq_object pAMQObject,
    _In_z_  const char* pMessage,
    _In_  int pPriority)
{
    return	w_amq_producer_send_message(
        pAMQObject,
        pMessage,
        pPriority);
}

W_RESULT	w_net_amq_producer_close(_Inout_ w_amq_object pAMQObject)
{
    return	w_amq_producer_close(pAMQObject);
}

W_RESULT	w_net_amq_consumer_close(_Inout_ w_amq_object pAMQObject)
{
    return	w_amq_consumer_close(pAMQObject);
}

void	w_net_amq_fini()
{
    w_amq_fini();
}

#endif

#pragma endregion

#pragma region FIBER

#ifdef WOLF_ENABLE_FIBER

W_RESULT w_net_fiber_async_write(_In_ void* pSocket, _In_ w_buffer pBuffer)
{
    return fiber_async_write(pSocket, pBuffer);
}

W_RESULT w_net_fiber_async_read(
    _In_ void* pSocket,
    _Inout_ w_buffer* pBuffer,
    _Inout_ size_t* pReplyLength)
{
    return fiber_async_read(pSocket, pBuffer, pReplyLength);
}

W_RESULT w_net_fiber_server_run(
    _In_ const w_socket_family pSocketFamily,
    _In_ const uint16_t pPort,
    _In_ int** pID,
    _In_ w_fiber_server_callback_fn pOnReceivedCallback)
{
    return fiber_server_run((int)pSocketFamily, pPort, pID, pOnReceivedCallback);
}

W_RESULT w_net_fiber_server_stop(_In_ const int pID)
{
    return fiber_server_stop(pID);
}

W_RESULT w_net_fiber_clients_connect(
    _In_ const int pIPV4_OR_IPV6,
    _In_ const char* pEndPoint,
    _In_ const uint16_t pPort,
    _In_ const int pNumberOfClients,
    _In_ w_fiber_client_callback_fn pOnSendReceiveCallback)
{
    return fiber_clients_connect(
        pIPV4_OR_IPV6,
        pEndPoint,
        pPort,
        pNumberOfClients,
        pOnSendReceiveCallback);
}

#endif

#pragma endregion

#pragma region HTTP

#ifdef WOLF_ENABLE_HTTP1_1_WS

void w_net_run_http1_1_server(
    _In_ const bool pSSL,
    _In_opt_z_ const char* pCertFilePath,
    _In_opt_z_ const char* pPrivateKeyFilePath,
    _In_opt_z_ const char* pPassPhrase,
    _In_opt_z_ const char* pRoot,
    _In_opt_z_ const char* pURLPath,
    _In_ const int pPort,
    _In_ http_on_listened_fn pOnListened)
{
    run_http1_1_server(
        pSSL,
        pCertFilePath,
        pPrivateKeyFilePath,
        pPassPhrase,
        pRoot,
        pURLPath,
        pPort,
        pOnListened);
}

void w_net_stop_http1_1_server(_Inout_ void* pUSocket, _In_ const bool pSSL)
{
    stop_http1_1_server(pUSocket, pSSL);
}

#endif

#pragma endregion
