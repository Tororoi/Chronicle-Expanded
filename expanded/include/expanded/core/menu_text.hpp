#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace expanded {

// Expanded text in place of the disc's in the menus' message banks (allmenu.mes, and the shop's itemshop.bin).
// A weapon's description (message ComItemInfo.msg + 500 in both) keeps the disc's name line and takes its lines
// below from the language files' "expanded.weapon.<item no>.description", or from a provider registered for the
// weapon. Any allmenu message can be given by a provider too. Text is as the language files write it (UTF-8,
// "\n" and the "{...}" codes of docs/LOCALIZATION.md). A provider is asked each time a menu builds the window,
// and gives std::nullopt for the disc's (or the language files') text.
using WeaponTextProvider = std::optional<std::string> (*)(int item_no);
using MenuTextProvider = std::optional<std::string> (*)();

// The language files' key of a weapon's description lines.
std::string WeaponDescriptionKey(int item_no);

// Registers or, with nullptr, removes the provider of a weapon's description lines.
void SetWeaponDescription(int item_no, WeaponTextProvider provider);

// Registers or, with nullptr, removes the provider of an allmenu message's whole text.
void SetMenuMessage(int message_id, MenuTextProvider provider);

// Registers or, with nullptr, removes the provider of a system message's whole text (the item-got line's bank,
// SystemMessage's), for a message the bank does not have.
void SetSystemMessage(int message_id, MenuTextProvider provider);

// The system messages Expanded adds, one id each past the bank's own.
inline constexpr int kAtlaSphereMessage = 1000;   // dungeon/atla_drops: a sphere found in an atla
inline constexpr int kWiseOwlMessage = 1001;      // weapons/toan/wise_owl_sword
inline constexpr int kSkeletonKeyMessage = 1002;  // weapons/toan/bone_rapier
inline constexpr int kKitchenKnifeBlessed = 1003; // weapons/toan/kitchen_knife: the spring's blessing …
inline constexpr int kKitchenKnifeFaded = 1004;   // … faded …
inline constexpr int kKitchenKnifeMended = 1005;  // … and the knife mended
inline constexpr int kMonstersSlowedMessage = 1006; // dungeon/magic_circles: the Secret Armlet's favoured rage circle

// Notes a pack entry as the game takes it (GetPackFile): allmenu.mes and itemshop.bin are the banks given Expanded
// text; any other entry at the same address is not.
void MenuTextLoaded(std::string_view name, const void *data);

// The Expanded text of message id in the bank at buff, as ClsMes::GetTextLineDataTop gives one, or nullptr for
// the bank's own. Valid until the next call for the same bank and id.
short *MenuText(const short *buff, int id);

// Forgets the banks and providers (for tests).
void MenuTextReset();

} // namespace expanded
