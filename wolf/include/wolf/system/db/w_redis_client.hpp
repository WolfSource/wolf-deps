/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#if defined(WOLF_SYSTEM_REDIS) && defined(WOLF_SYSTEM_COROUTINE)

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/detached.hpp>
#include <boost/redis.hpp>
#include <boost/redis/connection.hpp>
#include <boost/redis/src.hpp>
#include <wolf/wolf.hpp>

namespace wolf::system::db {
class w_redis_client {
 public:
  // redis client constructor
  W_API explicit w_redis_client(_In_ boost::redis::config p_config)
      : _config(std::move(p_config)) {}
  // redis client destructor
  W_API ~w_redis_client() noexcept { cancel(); }

  // disable copy constructor
  w_redis_client(const w_redis_client &) = delete;
  // disable copy operator
  auto operator=(const w_redis_client &) -> w_redis_client & = delete;

  // move constructor
  W_API w_redis_client(w_redis_client &&p_other) noexcept {
    _move(std::forward<w_redis_client &&>(p_other));
  }

  // move operator
  W_API auto operator=(w_redis_client &&p_other) noexcept -> w_redis_client & {
    _move(std::forward<w_redis_client &&>(p_other));
    return *this;
  }

  // connect to redis server
  W_API auto connect() -> boost::asio::awaitable<boost::leaf::result<int>> {
    try {
      if (!this->_conn) {
        this->_conn = std::make_shared<boost::redis::connection>(
            co_await boost::asio::this_coro::executor);
        if (!this->_conn) {
          co_return W_FAILURE(std::errc::not_enough_memory,
                              "could not allocate memory for redis connection");
        }
      }

      this->_conn->async_run(
          this->_config, {},
          boost::asio::consign(boost::asio::detached, this->_conn));

      boost::redis::response<std::string> res;
      boost::redis::request req;
      req.push("PING");

      std::ignore = co_await this->_conn->async_exec(
          req, res, boost::asio::use_awaitable);

      if (std::get<0>(res).value() != "PONG") {
        const auto msg = wolf::format(
            "could not connect to redis host '{}:{}' because 'PONG' response "
            "was "
            "expected",
            this->_config.addr.host, this->_config.addr.port);
        co_return W_FAILURE(std::errc::operation_canceled, msg);
      }

      co_return 0;
    } catch (const std::exception &e) {
      const auto msg = wolf::format(
          "could not connect to redis host '{}:{}' because of {} ",
          this->_config.addr.host, this->_config.addr.port, e.what());
      co_return W_FAILURE(std::errc::operation_canceled, msg);
    }
  }

  // disconnect from redis server
  W_API void cancel() noexcept {
    if (this->_conn) {
      this->_conn->cancel();
    }
  }

  // execute redis command
  template <class REDIS_RESPONSE>
  W_API auto exec(_In_ boost::redis::request &p_req)
      -> boost::asio::awaitable<boost::leaf::result<REDIS_RESPONSE>> {
    if (!this->_conn) {
      co_return W_FAILURE(std::errc::operation_canceled,
                          "redis connection is not established");
    }

    try {
      REDIS_RESPONSE result;
      std::ignore = co_await this->_conn->async_exec(
          p_req, result, boost::asio::use_awaitable);

      co_return std::move(result);
    } catch (const std::exception &e) {
      const auto msg = wolf::format(
          "could not execute redis command on host '{}:{}' because of {} ",
          this->_config.addr.host, this->_config.addr.port, e.what());
      co_return W_FAILURE(std::errc::operation_canceled, msg);
    }
  }

  // execute redis command
  W_API auto exec(_In_ std::string_view &p_req)
      -> boost::asio::awaitable<boost::leaf::result<std::string>> {
    if (!this->_conn) {
      co_return W_FAILURE(std::errc::operation_canceled,
                          "redis connection is not established");
    }

    try {
      boost::redis::response<std::string> res;

      boost::redis::request req;
      req.push(p_req.data());

      std::ignore = co_await this->_conn->async_exec(
          req, res, boost::asio::use_awaitable);

      co_return std::get<0>(res).value();
    } catch (const std::exception &e) {
      const auto msg = wolf::format(
          "could not execute redis command on host '{}:{}' because of {} ",
          this->_config.addr.host, this->_config.addr.port, e.what());
      co_return W_FAILURE(std::errc::operation_canceled, msg);
    }
  }

 private:
  void _move(w_redis_client &&p_other) noexcept {
    if (this == &p_other) {
      return;
    }
    this->_config = std::move(p_other._config);
    this->_conn = std::exchange(p_other._conn, nullptr);

    p_other._config = {};
  }

  // redis config
  boost::redis::config _config;
  // shared connection to redis server for batch async operations
  std::shared_ptr<boost::redis::connection> _conn = nullptr;
};

}  // namespace wolf::system::db

#endif  // defined(WOLF_SYSTEM_REDIS) && defined(WOLF_SYSTEM_COROUTINE)