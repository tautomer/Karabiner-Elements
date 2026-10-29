#include "hid_report_only_events.hpp"
#include "hid_report_only_events/corsair/k95.hpp"
#include <algorithm>
#include <array>
#include <boost/ut.hpp>
#include <cstdint>
#include <vector>

namespace {
namespace k95 = krbn::hid_report_only_events::corsair::k95;
namespace logical_key = krbn::momentary_switch_event_details::corsair_k95_g_key;
const auto time_stamp = pqrs::osx::chrono::absolute_time_point(12345);

std::vector<uint8_t> k95_descriptor() {
  // Original K95 RGB keyboard interface. The vendor-defined application
  // collection ffC0:0002 contains report 3, 63 8-bit input values.
  return {
      0x05,
      0x01,
      0x09,
      0x06,
      0xa1,
      0x01,
      0x85,
      0x01,
      0x05,
      0x07,
      0x19,
      0xe0,
      0x29,
      0xe7,
      0x15,
      0x00,
      0x25,
      0x01,
      0x75,
      0x01,
      0x95,
      0x08,
      0x81,
      0x02,
      0x19,
      0x00,
      0x29,
      0x97,
      0x15,
      0x00,
      0x25,
      0x01,
      0x75,
      0x01,
      0x95,
      0x98,
      0x81,
      0x02,
      0x05,
      0x08,
      0x19,
      0x01,
      0x29,
      0x03,
      0x25,
      0x01,
      0x75,
      0x01,
      0x95,
      0x03,
      0x91,
      0x02,
      0x75,
      0x05,
      0x95,
      0x01,
      0x91,
      0x01,
      0xc0,
      0x05,
      0x0c,
      0x09,
      0x01,
      0xa1,
      0x01,
      0x85,
      0x02,
      0x05,
      0x0c,
      0x19,
      0x00,
      0x2a,
      0xff,
      0x0f,
      0x15,
      0x00,
      0x26,
      0xff,
      0x0f,
      0x75,
      0x10,
      0x95,
      0x02,
      0x81,
      0x00,
      0xc0,
      0x06,
      0xc0,
      0xff,
      0x09,
      0x02,
      0xa1,
      0x01,
      0x85,
      0x03,
      0x15,
      0x00,
      0x26,
      0xff,
      0x00,
      0x95,
      0x3f,
      0x75,
      0x08,
      0x09,
      0x02,
      0x81,
      0x02,
      0xc0,
      0x06,
      0xc2,
      0xff,
      0x09,
      0x03,
      0xa1,
      0x01,
      0x85,
      0x0e,
      0x15,
      0x00,
      0x26,
      0xff,
      0x00,
      0x95,
      0x3f,
      0x75,
      0x08,
      0x09,
      0x03,
      0x81,
      0x02,
      0xc0,
  };
}

std::vector<uint8_t> report(uint8_t byte16 = 0,
                            uint8_t byte17 = 0,
                            uint8_t byte18 = 0) {
  std::vector<uint8_t> result(64, 0);
  result[0] = 3;
  result[16] = byte16;
  result[17] = byte17;
  result[18] = byte18;
  return result;
}

pqrs::osx::iokit_hid_value key_value(uint32_t key, bool pressed) {
  return pqrs::osx::iokit_hid_value(time_stamp,
                                    pressed ? 1 : 0,
                                    logical_key::usage_page,
                                    pqrs::hid::usage::value_t(key),
                                    1,
                                    0);
}

krbn::device_identifiers device(uint16_t vendor, uint16_t product, bool keyboard = true, bool virtual_device = false) {
  return krbn::device_identifiers({
      .vendor_id = pqrs::hid::vendor_id::value_t(vendor),
      .product_id = pqrs::hid::product_id::value_t(product),
      .is_keyboard = keyboard,
      .is_virtual_device = virtual_device,
  });
}

// Change a single byte in a distinctive part of the descriptor fixture.
std::vector<uint8_t> mutate(std::vector<uint8_t> source,
                            std::initializer_list<uint8_t> pattern,
                            size_t offset,
                            uint8_t value) {
  auto it = std::search(source.begin(), source.end(), pattern.begin(), pattern.end());
  if (it == source.end() || offset >= pattern.size()) {
    throw std::runtime_error("descriptor mutation pattern not found");
  }
  *(it + static_cast<std::ptrdiff_t>(offset)) = value;
  return source;
}
} // namespace

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "match only the physical original K95 and the expected HID interface"_test = [] {
    const auto descriptor = k95_descriptor();
    const auto matching = device(0x1b1c, 0x1b11);
    expect(krbn::hid_report_only_events::is_target_device(matching));
    expect(krbn::hid_report_only_events::make_report_handler(matching, descriptor) != nullptr);
    for (const auto& other : {
             device(0x1b1c, 0x1b08), // older non-RGB K95
             device(0x1b1c, 0x1b2d), // K95 Platinum
             device(0x1b1c, 0x1b89), // K95 Platinum XT
             device(0x1b1c, 0xffff), // iCUE virtual keyboard
             device(0x05ac, 0x1b11),
             device(0x1b1c, 0x1b11, false),
             device(0x1b1c, 0x1b11, true, true),
         }) {
      expect(!krbn::hid_report_only_events::is_target_device(other));
      expect(krbn::hid_report_only_events::make_report_handler(other, descriptor) == nullptr);
    }
  };

  "reject incompatible, malformed, or ambiguous report descriptors"_test = [] {
    const auto d = k95_descriptor();
    expect(k95::matches_report_descriptor(d));
    expect(!k95::matches_report_descriptor({}));
    auto truncated = d;
    truncated.pop_back();
    expect(!k95::matches_report_descriptor(truncated));
    // Wrong vendor collection page, collection usage, collection type, report ID,
    // count, field usage, data flags, or an extra input field under report 3.
    expect(!k95::matches_report_descriptor(mutate(d, {0x06, 0xc0, 0xff, 0x09, 0x02, 0xa1, 0x01, 0x85, 0x03}, 1, 0xc1)));
    expect(!k95::matches_report_descriptor(mutate(d, {0x06, 0xc0, 0xff, 0x09, 0x02, 0xa1, 0x01, 0x85, 0x03}, 4, 0x01)));
    expect(!k95::matches_report_descriptor(mutate(d, {0x06, 0xc0, 0xff, 0x09, 0x02, 0xa1, 0x01, 0x85, 0x03}, 6, 0x00)));
    expect(!k95::matches_report_descriptor(mutate(d, {0x06, 0xc0, 0xff, 0x09, 0x02, 0xa1, 0x01, 0x85, 0x03}, 8, 0x04)));
    expect(!k95::matches_report_descriptor(mutate(d, {0x95, 0x3f, 0x75, 0x08, 0x09, 0x02, 0x81, 0x02}, 1, 0x3e)));
    expect(!k95::matches_report_descriptor(mutate(d, {0x95, 0x3f, 0x75, 0x08, 0x09, 0x02, 0x81, 0x02}, 5, 0x01)));
    expect(!k95::matches_report_descriptor(mutate(d, {0x95, 0x3f, 0x75, 0x08, 0x09, 0x02, 0x81, 0x02}, 7, 0x00)));
    expect(!k95::matches_report_descriptor(mutate(d, {0x95, 0x3f, 0x75, 0x08, 0x09, 0x02, 0x81, 0x02}, 7, 0x06)));
    auto extra = d;
    const std::array<uint8_t, 4> tail{0x09, 0x02, 0x81, 0x02};
    const auto position = std::search(extra.begin(), extra.end(), tail.begin(), tail.end());
    expect(position != extra.end());
    if (position != extra.end()) {
      extra.insert(position + 4, tail.begin(), tail.end());
      expect(!k95::matches_report_descriptor(extra));
    }
    expect(krbn::hid_report_only_events::make_report_handler(device(0x1b1c, 0x1b11), truncated) == nullptr);
  };

  "captured G1 G2 G3 frames generate separate down/up values"_test = [] {
    k95::handler h;
    for (const auto& [key, mask] : std::array<std::pair<uint32_t, uint8_t>, 3>{{{1, 1}, {2, 2}, {3, 4}}}) {
      auto down = report(mask);
      expect(h.should_accept_report(3, down));
      expect(static_cast<bool>(h.handle(3, down, time_stamp) == std::vector<pqrs::osx::iokit_hid_value>{key_value(key, true)}));
      expect(h.handle(3, down, time_stamp).empty());
      expect(static_cast<bool>(h.handle(3, report(), time_stamp) == std::vector<pqrs::osx::iokit_hid_value>{key_value(key, false)}));
    }
  };

  "all 18 synthetic positions generate correct distinct events"_test = [] {
    constexpr std::array<std::pair<size_t, uint8_t>, 18> positions{{
        {16, 0x01},
        {16, 0x02},
        {16, 0x04},
        {16, 0x08},
        {16, 0x10},
        {16, 0x20},
        {16, 0x40},
        {16, 0x80},
        {17, 0x01},
        {17, 0x02},
        {18, 0x01},
        {18, 0x02},
        {18, 0x04},
        {18, 0x08},
        {18, 0x10},
        {18, 0x20},
        {18, 0x40},
        {18, 0x80},
    }};
    for (size_t i = 0; i < positions.size(); ++i) {
      k95::handler h;
      auto down = report();
      down[positions[i].first] = positions[i].second;
      auto key = static_cast<uint32_t>(i + 1);
      expect(static_cast<bool>(h.handle(3, down, time_stamp) == std::vector<pqrs::osx::iokit_hid_value>{key_value(key, true)}));
      expect(static_cast<bool>(h.handle(3, report(), time_stamp) == std::vector<pqrs::osx::iokit_hid_value>{key_value(key, false)}));
    }
  };

  "simultaneous and changed keys are ordered releases first"_test = [] {
    k95::handler h;
    expect(static_cast<bool>(h.handle(3, report(0x01, 0x02, 0x80), time_stamp) == std::vector<pqrs::osx::iokit_hid_value>{
                                                                                      key_value(1, true), key_value(10, true), key_value(18, true)}));
    expect(static_cast<bool>(h.handle(3, report(0x02, 0x02, 0x00), time_stamp) == std::vector<pqrs::osx::iokit_hid_value>{
                                                                                      key_value(1, false), key_value(18, false), key_value(2, true)}));
    expect(static_cast<bool>(h.handle(3, report(), time_stamp) == std::vector<pqrs::osx::iokit_hid_value>{
                                                                      key_value(2, false), key_value(10, false)}));
  };

  "ignore non-G buttons and ordinary report bytes"_test = [] {
    k95::handler h;
    for (size_t offset = 1; offset < 64; ++offset) {
      for (uint8_t bit = 0; bit < 8; ++bit) {
        if ((offset == 16 || offset == 18) || (offset == 17 && bit < 2)) {
          continue;
        }
        auto bytes = report();
        bytes[offset] = static_cast<uint8_t>(1u << bit);
        expect(h.handle(3, bytes, time_stamp).empty());
      }
    }
  };

  "reject malformed reports without losing held state"_test = [] {
    k95::handler h;
    expect(static_cast<bool>(h.handle(3, report(1), time_stamp) == std::vector<pqrs::osx::iokit_hid_value>{key_value(1, true)}));
    for (size_t size = 0; size <= 128; ++size) {
      if (size == 64) continue;
      std::vector<uint8_t> malformed(size, 0);
      if (!malformed.empty()) malformed[0] = 3;
      expect(!h.should_accept_report(3, malformed));
      expect(h.handle(3, malformed, time_stamp).empty());
    }
    auto wrong_id = report();
    wrong_id[0] = 2;
    expect(!h.should_accept_report(3, wrong_id));
    expect(h.handle(3, wrong_id, time_stamp).empty());
    expect(!h.should_accept_report(2, report()));
    expect(h.handle(2, report(), time_stamp).empty());
    expect(static_cast<bool>(h.handle(3, report(), time_stamp) == std::vector<pqrs::osx::iokit_hid_value>{key_value(1, false)}));
  };

  "reset clears decoder state for the next start"_test = [] {
    k95::handler h;
    expect(h.handle(3, report(1), time_stamp).size() == 1_u);
    h.reset();
    expect(static_cast<bool>(h.handle(3, report(1), time_stamp) == std::vector<pqrs::osx::iokit_hid_value>{key_value(1, true)}));
  };
}
