#include "expanded/weapons/ruby/fairys_ring.hpp"

#include <algorithm>
#include <format>

#include "clsmes.hpp"
#include "editloop.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/weapons/ownership.hpp"
#include "itemdata.hpp"

namespace expanded {

namespace {

constexpr std::string_view kPoints = "{value2}";
constexpr int              kIconSword = -0x2EE; // {icon-sword}, the weapons' icon

bool g_fortune_catch = false;

[[maybe_unused]] const bool kRegistered =
    (SetTalkMessage(kCatchMessage, FairyFortuneCatchText), AddMessagePicture(FairyFortunePicture), true);

} // namespace

int FairyFortunePoints(int fish_kind, int points) {
    g_fortune_catch = !std::ranges::contains(kGarayans, fish_kind) && WeaponOwned(ITEM_WEAPON_FAIRY_S_RING);
    if (!g_fortune_catch) {
        return points;
    }
    LogEvent("fairy_fortune", std::format("\"kind\":{},\"points\":{},\"paid\":{}", fish_kind, points, points * kFairyFortune));
    return points * kFairyFortune;
}

std::optional<std::string> FairyFortuneCatchText(std::string_view own) {
    const std::size_t at = own.find(kPoints);
    if (!g_fortune_catch || at == std::string_view::npos) {
        return std::nullopt;
    }
    std::string text(own);
    text.replace(at, kPoints.size(), "{highlight}{value2}{/color} {icon-sword}");
    return text;
}

int FairyFortunePicture(const ClsMes &mes, int code) {
    const bool catch_icon = &mes == &EditMes1 && mes.mes_made == kCatchMessage && code == kIconSword;
    return g_fortune_catch && catch_icon ? ITEM_WEAPON_FAIRY_S_RING : 0;
}

} // namespace expanded
