/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#pragma once

#ifdef WOLF_SYSTEM_SOCKET

#include <boost/asio.hpp>
#include <variant>
#include <wolf.hpp>

#include "w_socket_options.hpp"

using tcp = boost::asio::ip::tcp;

namespace wolf::system::socket {
class w_tcp_client {
 public:
  // default constructor
  W_API explicit w_tcp_client(boost::asio::io_context &p_io_context) noexcept
      : _resolver(std::make_unique<tcp::resolver>(p_io_context)),
        _socket(std::make_unique<tcp::socket>(p_io_context)) {}

  // move constructor.
  W_API w_tcp_client(w_tcp_client &&p_other) = default;
  // move assignment operator.
  W_API w_tcp_client &operator=(w_tcp_client &&p_other) = default;

  // destructor
  W_API virtual ~w_tcp_client() noexcept {
    try {
      const auto _resolver_nn = gsl::not_null<tcp::resolver *>(this->_resolver.get());
      const auto _socket_nn = gsl::not_null<tcp::socket *>(this->_socket.get());

      _resolver_nn->cancel();
      if (_socket_nn->is_open()) {
        _socket_nn->close();
      }
      _socket_nn->shutdown(tcp::socket::shutdown_both);
    } catch (...) {
    }
  }

  /*
   * resolve an endpoint asynchronously
   * @param p_endpoint, the endpoint
   * @returns a coroutine contains the results of reolver
   */
  W_API
  boost::asio::awaitable<
      boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp>>
  async_resolve(_In_ const tcp::endpoint &p_endpoint) {
    const auto _resolver_nn = gsl::not_null<tcp::resolver *>(this->_resolver.get());
    return _resolver_nn->async_resolve(p_endpoint, boost::asio::use_awaitable);
  }

  /*
   * resolve an address with port asynchronously
   * @param p_address, the address
   * @param p_port, the port
   * @returns a coroutine contains the results of reolver
   */
  W_API
  boost::asio::awaitable<
      boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp>>
  async_resolve(_In_ const std::string &p_address, _In_ const uint16_t &p_port) {
    const auto _resolver_nn = gsl::not_null<tcp::resolver *>(this->_resolver.get());

    const auto _address = boost::asio::ip::make_address(p_address);
    const tcp::endpoint _endpoint(_address, p_port);
    return _resolver_nn->async_resolve(_endpoint, boost::asio::use_awaitable);
  }

  /*
   * open a socket and connect to the endpoint asynchronously
   * @param p_endpoint, the endpoint of the server
   * @param p_socket_options, the socket options
   * @returns a coroutine
   */
  W_API
  boost::asio::awaitable<void, boost::asio::any_io_executor> async_connect(
      _In_ const tcp::endpoint &p_endpoint, _In_ const w_socket_options &p_socket_options) {
    const gsl::not_null<tcp::socket *> _socket_nn(this->_socket.get());

    // get protocol
    const auto _protocol = p_endpoint.protocol();

    // open socket and connect to the endpoint asynchronously
    _socket_nn->open(_protocol);

    // set socket options
    _socket_nn->set_option(boost::asio::socket_base::reuse_address(p_socket_options.reuse_address));
    _socket_nn->set_option(boost::asio::socket_base::keep_alive(p_socket_options.keep_alive));
    _socket_nn->set_option(tcp::no_delay(p_socket_options.no_delay));

    return this->_socket->async_connect(p_endpoint, boost::asio::use_awaitable);
  }

  /*
   * write buffer data into the socket
   * @param p_buffer, the source buffer which should be written
   * @returns number of the written bytes
   */
  W_API
  boost::asio::awaitable<size_t> async_write(_In_ const w_buffer &p_buffer) {
    const gsl::not_null<tcp::socket *> _socket_nn(this->_socket.get());

    return _socket_nn->async_send(boost::asio::buffer(p_buffer.buf, p_buffer.used_bytes),
                                  boost::asio::use_awaitable);
  }

  /*
   * read from the socket into the buffer
   * @param p_mut_buffer, the destination buffer which will contain bytes
   * @returns number of read bytes
   */
  W_API
  boost::asio::awaitable<size_t> async_read(_Inout_ w_buffer &p_mut_buffer) {
    const gsl::not_null<tcp::socket *> _socket_nn(this->_socket.get());

    return _socket_nn->async_receive(boost::asio::buffer(p_mut_buffer.buf),
                                     boost::asio::use_awaitable);
  }

  /*
   * get whether socket is open
   * @returns true if socket was open
   */
  W_API
  bool get_is_open() const {
    const gsl::not_null<tcp::socket *> _socket_nn(this->_socket.get());
    return _socket_nn->is_open();
  }

 private:
  // copy constructor
  w_tcp_client(const w_tcp_client &) = delete;
  // copy operator
  w_tcp_client &operator=(const w_tcp_client &) = delete;

  std::unique_ptr<boost::asio::ip::tcp::socket> _socket;
  std::unique_ptr<boost::asio::ip::tcp::resolver> _resolver;
};
}  // namespace wolf::system::socket

#endif