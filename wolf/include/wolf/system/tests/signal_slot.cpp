/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#if defined(WOLF_TEST) && defined(WOLF_SYSTEM_SIG_SLOT)

#include <boost/signals2.hpp>
#include <boost/test/unit_test.hpp>
#include <system/w_leak_detector.hpp>
#include <wolf.hpp>

BOOST_AUTO_TEST_CASE(sig_slot_test) {
  const wolf::system::w_leak_detector _detector = {};

  std::cout << "entering test case 'sig_slot_test'" << std::endl;

  auto sig = boost::signals2::signal<void()>();
  sig.connect([]() { std::cout << "hello from slot 1" << std::endl; });
  sig.connect([]() { std::cout << "hello from slot 2" << std::endl; });
  sig();

  std::cout << "leaving test case 'sig_slot_test'" << std::endl;
}

#endif  // defined(WOLF_TEST) && defined(WOLF_SYSYEM_SIG_SLOT)