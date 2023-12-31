/*
  Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
  https://github.com/WolfSource/wolf
*/

#include <wolf/system/w_leak_detector.hpp>
#include <wolf/wolf.hpp>

#define BOOST_TEST_MODULE wolf_tests
#include <boost/test/included/unit_test.hpp>
#include <cstdlib>
#include <filesystem>
#include <string_view>

BOOST_AUTO_TEST_CASE(init) {
  const wolf::system::w_leak_detector _detector = {};

  std::cout << "entering test case 'wolf_test'\n";

  const auto _wolf_version = wolf::w_init();
  BOOST_TEST(_wolf_version.empty() == false);

  std::cout << "leaving test case 'wolf_test'\n";
}
