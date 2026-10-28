#pragma once

// The fishing scripts of a spot's town, written as Norune's: the entry the sign's trigger runs (the "!", the fishing
// menu, the rod check, the fishing Toan and _LOAD_FISHING_DATA), the quit menu (label 133, which Circle asks for while
// fishing), the bait menu (label 134, Square), and the menu the entry and quit share; and the Queens canal's message and
// arrival. Includes no game header, so dcdata can build with it.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "expanded/data/fishing_spots.hpp"
#include "expanded/data/stb.hpp"

namespace expanded {

inline constexpr std::uint32_t kFishingMenuLabel = 9600, kFishingQuitLabel = 133, kFishingBaitLabel = 134;

// The event messages the menus show: the entry menu, the no-rod line and the quit menu; and the catch's talk message.
inline constexpr int kFishingEntryMessage = 20, kFishingNoRodMessage = 21, kFishingQuitMessage = 22, kFishingCatchMessage = 2000;

// menu is the shared menu's header offset from the code base.
stb::Writer FishingMenuScript();
stb::Writer FishingEntryScript(const FishingSpot &spot, std::size_t menu);
stb::Writer FishingQuitScript(std::size_t menu);
stb::Writer FishingBaitScript();

// A message in the event window, as the fishing menus show theirs, once Cross is pressed at a point (the "!" drawn while
// the player stands there): the Queens canal ladder's when the tide is too high.
stb::Writer PointMessageScript(int message);

// Arriving at a place: the player there, facing yaw, the camera behind (East Harbor's dock, where the rising tide
// takes a player off the canal floor).
stb::Writer ArrivalScript(const std::array<float, 3> &position, float yaw);

// The town's event script with the menu, each spot's entry under its label, the quit and the bait, then what more adds.
Bytes AddFishingScripts(std::span<const unsigned char> script, std::span<const FishingSpot *const> spots,
                        void (*more)(stb::Appender &appender) = nullptr);

} // namespace expanded
