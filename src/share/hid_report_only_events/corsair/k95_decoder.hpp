#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace krbn::hid_report_only_events::corsair::k95 {

// Original, 18-G-key K95 RGB only. This does not cover the K95 Platinum,
// Platinum XT, or the older non-RGB K95, even if their product names are similar.
inline constexpr std::uint16_t vendor_id = 0x1b1c;
inline constexpr std::uint16_t product_id = 0x1b11;
inline constexpr std::uint32_t input_report_id = 3;
inline constexpr std::size_t report_size = 64;
inline constexpr std::size_t key_count = 18;
inline constexpr std::uint32_t all_keys_mask = (1U << key_count) - 1U;

struct device_identity {
  std::uint16_t vendor;
  std::uint16_t product;
};

[[nodiscard]] constexpr bool is_target_device(device_identity device) noexcept {
  return device.vendor == vendor_id && device.product == product_id;
}

enum class transition { down,
                        up };

struct key_event {
  // Logical key number, 1 through 18. Not an HID usage or Karabiner key code.
  std::uint8_t key;
  transition type;

  bool operator==(const key_event&) const = default;
};

enum class report_status {
  accepted,
  unsupported_device,
  unsupported_report_id,
  invalid_report_length,
  report_id_mismatch,
};

[[nodiscard]] constexpr std::string_view status_name(report_status status) noexcept {
  switch (status) {
    case report_status::accepted:
      return "accepted";
    case report_status::unsupported_device:
      return "unsupported_device";
    case report_status::unsupported_report_id:
      return "unsupported_report_id";
    case report_status::invalid_report_length:
      return "invalid_report_length";
    case report_status::report_id_mismatch:
      return "report_id_mismatch";
  }
  return "invalid_status";
}

struct decode_result {
  report_status status;
  std::vector<key_event> events;

  [[nodiscard]] bool accepted() const noexcept {
    return status == report_status::accepted;
  }
};

// One decoder per physical device/report stream. It performs no I/O, emits no
// system input, and is intentionally independent of IOKit and Karabiner types.
// The caller must serialize handle/reset calls and supply the real device IDs.
class decoder final {
public:
  explicit decoder(device_identity device) noexcept : device_(device) {}

  [[nodiscard]] decode_result handle(std::uint32_t report_id,
                                     std::span<const std::uint8_t> report) {
    if (!is_target_device(device_)) {
      return {report_status::unsupported_device, {}};
    }
    if (report_id != input_report_id) {
      return {report_status::unsupported_report_id, {}};
    }
    // Strictly accept the observed 64-byte, ID-prefixed layout only. In
    // particular, a 63-byte payload without the leading ID is NOT interchangeable.
    if (report.size() != report_size) {
      return {report_status::invalid_report_length, {}};
    }
    if (report[0] != input_report_id) {
      return {report_status::report_id_mismatch, {}};
    }

    // Packet offsets INCLUDE the leading report-ID byte. G9/G10 occupy only
    // bits 0/1 of byte 17; its other bits include non-G controls and are ignored.
    // Bitmap positions in ckb-next's generic NXP key map are 120..129,136..143.
    const auto current = static_cast<std::uint32_t>(report[16]) |
                         (static_cast<std::uint32_t>(report[17] & 0x03U) << 8U) |
                         (static_cast<std::uint32_t>(report[18]) << 10U);

    auto events = changes(held_mask_, current);
    held_mask_ = current;
    return {report_status::accepted, std::move(events)};
  }

  // A caller handling disconnect, shutdown, or stream replacement must consume
  // these releases. Invalid reports themselves never release a held key.
  [[nodiscard]] std::vector<key_event> reset() {
    auto events = changes(held_mask_, 0);
    held_mask_ = 0;
    return events;
  }

  // Bit (key_number - 1) denotes a held G key, independently of packet layout.
  [[nodiscard]] std::uint32_t held_mask() const noexcept { return held_mask_; }

private:
  [[nodiscard]] static std::vector<key_event> changes(std::uint32_t previous,
                                                      std::uint32_t current) {
    const auto released = previous & ~current;
    const auto pressed = current & ~previous;
    std::vector<key_event> events;

    // Releases before presses avoid a transient overlap when one report changes
    // from one held key to another. Each group is ordered by G-key number.
    for (auto type : {transition::up, transition::down}) {
      const auto changed = type == transition::up ? released : pressed;
      for (std::size_t i = 0; i < key_count; ++i) {
        if ((changed & (1U << i)) != 0) {
          events.push_back({static_cast<std::uint8_t>(i + 1U), type});
        }
      }
    }
    return events;
  }

  device_identity device_;
  std::uint32_t held_mask_ = 0;
};

} // namespace krbn::hid_report_only_events::corsair::k95
