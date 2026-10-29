#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <pqrs/hid.hpp>
#include <string>
#include <string_view>

namespace krbn::momentary_switch_event_details::corsair_k95_g_key {
// This is an internal event namespace, not a physical HID usage page. HID usage
// pages are 16-bit; using a value outside that range prevents a real vendor HID
// element from accidentally matching one of the synthesized K95 G keys. The
// virtual-HID output layer does not forward this page: these are input-only keys.
inline constexpr pqrs::hid::usage_page::value_t usage_page(0x10000);
inline constexpr std::array<std::string_view, 18> names{
    "corsair_k95_g1",
    "corsair_k95_g2",
    "corsair_k95_g3",
    "corsair_k95_g4",
    "corsair_k95_g5",
    "corsair_k95_g6",
    "corsair_k95_g7",
    "corsair_k95_g8",
    "corsair_k95_g9",
    "corsair_k95_g10",
    "corsair_k95_g11",
    "corsair_k95_g12",
    "corsair_k95_g13",
    "corsair_k95_g14",
    "corsair_k95_g15",
    "corsair_k95_g16",
    "corsair_k95_g17",
    "corsair_k95_g18",
};

[[nodiscard]] inline bool target(pqrs::hid::usage_page::value_t page,
                                 pqrs::hid::usage::value_t usage) noexcept {
  auto value = type_safe::get(usage);
  return page == usage_page && value >= 1 && value <= static_cast<int32_t>(names.size());
}

[[nodiscard]] inline std::optional<pqrs::hid::usage_pair> find_usage_pair(std::string_view name) {
  for (size_t i = 0; i < names.size(); ++i) {
    if (names[i] == name) {
      return pqrs::hid::usage_pair(usage_page, pqrs::hid::usage::value_t(static_cast<int32_t>(i + 1)));
    }
  }
  return std::nullopt;
}

[[nodiscard]] inline std::string make_name(pqrs::hid::usage::value_t usage) {
  if (!target(usage_page, usage)) {
    return "unsupported";
  }
  return std::string(names[static_cast<size_t>(type_safe::get(usage) - 1)]);
}
} // namespace krbn::momentary_switch_event_details::corsair_k95_g_key
