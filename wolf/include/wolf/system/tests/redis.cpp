/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#if defined(WOLF_TEST) && defined(WOLF_SYSTEM_REDIS) && \
    defined(WOLF_SYSTEM_COROUTINE)

#include <boost/test/unit_test.hpp>
#include <system/db/w_redis_client.hpp>
#include <system/w_leak_detector.hpp>
#include <wolf.hpp>

BOOST_AUTO_TEST_CASE(redis_test) {
  // const wolf::system::w_leak_detector _detector = {};

  std::cout << "entering test case 'redis_test'" << std::endl;

  boost::asio::io_context _io;
  boost::asio::co_spawn(
      _io,
      [&]() -> boost::asio::awaitable<void> {
        using w_redis_client = wolf::system::db::w_redis_client;

        boost::redis::config _config{};
        w_redis_client _redis(_config);
        const auto &_ret = co_await _redis.connect();
        if (!_ret.has_error()) {
          const auto &res_cmd = co_await _redis.exec("PING");
          BOOST_TEST(res_cmd.has_error() == false);
          BOOST_TEST(res_cmd.value() == "PONG");

          boost::redis::request reqs;
          reqs.push("HELLO", 3);
          reqs.push("PING", "Hello world");
          reqs.push("QUIT");

          boost::redis::response<boost::redis::ignore_t, std::string,
                                 boost::redis::ignore_t>
              resp = co_await _redis.exec(reqs);
          BOOST_TEST(_res.has_error() == false);

          std::cout << "redis reply: " << std::get<1>(resp).value()
                    << std::endl;
        }
      },
      boost::asio::detached);

  _io.run();

  std::cout << "leaving test case 'redis_test'" << std::endl;
}

#endif  // defined(WOLF_TEST) && defined(WOLF_SYSTEM_REDIS)