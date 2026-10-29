#pragma once

#include "hid_report_only_events/corsair/k95_decoder.hpp"
#include "hid_report_only_events/report_handler.hpp"
#include "types/momentary_switch_event_details/corsair_k95_g_key.hpp"
#include <memory>
#include <pqrs/hid.hpp>
#include <span>
#include <vector>

namespace krbn::hid_report_only_events::corsair::k95 {
[[nodiscard]] inline bool is_target_device(pqrs::hid::vendor_id::value_t vendor,
                                           pqrs::hid::product_id::value_t product) noexcept {
  return vendor == pqrs::hid::vendor_id::value_t(vendor_id) &&
         product == pqrs::hid::product_id::value_t(product_id);
}

// Validate the semantic report layout, not merely the VID/PID. On the K95 RGB
// keyboard interface report 3 has one vendor-defined input field, consisting
// of 63 bytes in application collection ffC0:0002. Its input bitmap follows the
// leading report-ID byte delivered by IOHIDDeviceRegisterInputReportCallback.
// A different interface or changed firmware falls back to normal HID handling.
[[nodiscard]] inline bool matches_report_descriptor(
    std::span<const uint8_t> report_descriptor) {
  auto parsed = pqrs::hid::report_descriptor::parse(report_descriptor);
  if (!parsed) {
    return false;
  }

  bool found = false;
  constexpr auto vendor_page = pqrs::hid::usage_page::value_t(0xffc0);
  constexpr auto vendor_usage = pqrs::hid::usage::value_t(2);
  using namespace pqrs::hid::report_descriptor;
  for (const auto& field : parsed->get_descriptor().get_report_fields()) {
    if (field.get_report_type() != report_type::input ||
        field.get_report_id() != pqrs::hid::report_id::value_t(input_report_id)) {
      continue;
    }

    const auto& path = field.get_collection_path();
    const auto& usages = field.get_usages();
    if (found ||
        field.get_bit_offset() != 0 ||
        field.get_size_bits() != 8 ||
        field.get_count() != 63 ||
        field.get_raw_flags() != 0x02 || // Data, Variable, Absolute.
        field.get_usage_page() != vendor_page ||
        field.get_logical_minimum() != 0 ||
        field.get_logical_maximum() != 255 ||
        usages.size() != 1 ||
        usages[0] != pqrs::hid::usage_pair(vendor_page, vendor_usage) ||
        field.get_usage_minimum() || field.get_usage_maximum() ||
        path.size() != 1 || path[0].get_type() != 1 || !path[0].get_usage() ||
        *path[0].get_usage() != pqrs::hid::usage_pair(vendor_page, vendor_usage)) {
      return false;
    }
    found = true;
  }
  return found;
}

class handler final : public krbn::hid_report_only_events::report_handler {
public:
  handler()
      : decoder_(device_identity{vendor_id, product_id}) {
  }

  [[nodiscard]] bool should_accept_report(uint32_t report_id,
                                          std::span<const uint8_t> report) override {
    return report_id == input_report_id &&
           report.size() == report_size &&
           report[0] == input_report_id;
  }

  [[nodiscard]] std::vector<pqrs::osx::iokit_hid_value> handle(
      uint32_t report_id,
      std::span<const uint8_t> report,
      pqrs::osx::chrono::absolute_time_point time_stamp) override {
    // decoder::handle also validates its input, preserving state on malformed or
    // unrelated reports even if a future caller bypasses the monitor's filter.
    auto decoded = decoder_.handle(report_id, report);
    std::vector<pqrs::osx::iokit_hid_value> result;
    result.reserve(decoded.events.size());
    for (const auto& event : decoded.events) {
      result.emplace_back(time_stamp,
                          event.type == transition::down ? 1 : 0,
                          krbn::momentary_switch_event_details::corsair_k95_g_key::usage_page,
                          pqrs::hid::usage::value_t(event.key),
                          1,
                          0);
    }
    return result;
  }

  void reset() override {
    // The device grabber sends a device_ungrabbed event when the monitor stops,
    // releasing any mapped output keys. The handler itself is reset at start.
    (void)decoder_.reset();
  }

private:
  decoder decoder_;
};

[[nodiscard]] inline std::shared_ptr<krbn::hid_report_only_events::report_handler>
make_report_handler(pqrs::hid::vendor_id::value_t vendor,
                    pqrs::hid::product_id::value_t product,
                    std::span<const uint8_t> report_descriptor) {
  if (is_target_device(vendor, product) && matches_report_descriptor(report_descriptor)) {
    return std::make_shared<handler>();
  }
  return nullptr;
}
} // namespace krbn::hid_report_only_events::corsair::k95
