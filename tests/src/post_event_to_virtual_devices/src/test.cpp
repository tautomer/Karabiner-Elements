#include "../../share/json_helper.hpp"
#include "../../share/manipulator_helper.hpp"
#include "dispatcher_utility.hpp"
#include "manipulator/manipulators/post_event_to_virtual_devices/post_event_to_virtual_devices.hpp"
#include "run_loop_thread_utility.hpp"
#include <boost/ut.hpp>

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  auto scoped_dispatcher_manager = krbn::dispatcher_utility::initialize_dispatchers();
  auto scoped_run_loop_thread_manager = krbn::run_loop_thread_utility::initialize_scoped_run_loop_thread_manager(
      pqrs::cf::run_loop_thread::failure_policy::abort);

  "actual examples"_test = [] {
    auto helper = std::make_unique<krbn::unit_testing::manipulator_helper>();

    helper->run_tests(krbn::unit_testing::json_helper::load_jsonc("json/tests.json"));

    helper = nullptr;
  };

  "unmapped K95 G keys never reach virtual HID or change repeat state"_test = [] {
    namespace post = krbn::manipulator::manipulators::post_event_to_virtual_devices;
    namespace keys = krbn::momentary_switch_event_details::corsair_k95_g_key;
    post::queue queue(std::make_shared<krbn::pressed_keys_manager>(),
                      std::make_shared<krbn::keyboard_suppression>());
    for (int32_t i = 1; i <= 18; ++i) {
      auto usage = pqrs::hid::usage_pair(keys::usage_page, pqrs::hid::usage::value_t(i));
      queue.emplace_back_key_event(usage, krbn::event_type::key_down, krbn::absolute_time_point(100));
      queue.emplace_back_key_event(usage, krbn::event_type::key_up, krbn::absolute_time_point(200));
      expect(queue.get_events().empty());
      expect(!queue.get_keyboard_repeat_detector().is_repeating());
    }
  };

  "mouse_key_handler.count_converter"_test = [] {
    {
      krbn::manipulator::manipulators::post_event_to_virtual_devices::mouse_key_handler::count_converter count_converter(64);
      expect(count_converter.update(32) == static_cast<uint8_t>(0));
      expect(count_converter.update(32) == static_cast<uint8_t>(1));
      expect(count_converter.update(32) == static_cast<uint8_t>(0));
      expect(count_converter.update(32) == static_cast<uint8_t>(1));

      expect(count_converter.update(16) == static_cast<uint8_t>(0));
      expect(count_converter.update(16) == static_cast<uint8_t>(0));
      expect(count_converter.update(16) == static_cast<uint8_t>(0));
      expect(count_converter.update(16) == static_cast<uint8_t>(1));

      expect(count_converter.update(-16) == static_cast<uint8_t>(0));
      expect(count_converter.update(-16) == static_cast<uint8_t>(0));
      expect(count_converter.update(-16) == static_cast<uint8_t>(0));
      expect(count_converter.update(-16) == static_cast<uint8_t>(-1));
    }
    {
      krbn::manipulator::manipulators::post_event_to_virtual_devices::mouse_key_handler::count_converter count_converter(64);
      expect(count_converter.update(128) == static_cast<uint8_t>(2));
      expect(count_converter.update(128) == static_cast<uint8_t>(2));
      expect(count_converter.update(-128) == static_cast<uint8_t>(-2));
      expect(count_converter.update(-128) == static_cast<uint8_t>(-2));
    }
  };

  return 0;
}
