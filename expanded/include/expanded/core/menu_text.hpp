#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace dc::game {
class ClsMes;
class CRect_i_;
} // namespace dc::game

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

// Adds a provider of an allmenu message's whole text, or, with nullptr, removes the message's providers. A message may
// have several (features that each change it in their own case): they are asked in turn, and the first with text gives it.
void SetMenuMessage(int message_id, MenuTextProvider provider);

// Registers or, with nullptr, removes the provider of a system message's whole text (the item-got line's bank,
// SystemMessage's), for a message the bank does not have.
void SetSystemMessage(int message_id, MenuTextProvider provider);

// The text of a town talk message (in the bank of the town's talk window, EditMes1) made from the bank's own text, in
// the same form, or std::nullopt for the bank's own. Asked each time the window lays the message out.
using TalkTextProvider = std::optional<std::string> (*)(std::string_view own);

// Registers or, with nullptr, removes the provider of a town talk message.
void SetTalkMessage(int message_id, TalkTextProvider provider);

// The item whose picture window mes draws in place of icon code ({icon-sword}...), or 0 for the icon.
using MessagePictureProvider = int (*)(const ClsMes &mes, int code);

// Registers, once, a provider of the item pictures message windows draw in place of their icons.
void AddMessagePicture(MessagePictureProvider provider);

// Hook: ClsMes::DrawGaijiFont: draws in place of icon code, over screen, the picture of the item a provider gives for
// it; true when drawn.
bool MessagePicture(const ClsMes &mes, int code, const CRect_i_ &screen, int alpha);

// The system messages Expanded adds, one id each past the bank's own.
inline constexpr int kAtlaSphereMessage = 1000;   // dungeon/atla_drops: a sphere found in an atla
inline constexpr int kWiseOwlMessage = 1001;      // weapons/toan/wise_owl_sword
inline constexpr int kSkeletonKeyMessage = 1002;  // weapons/toan/bone_rapier
inline constexpr int kKitchenKnifeBlessed = 1003; // weapons/toan/kitchen_knife: the spring's blessing …
inline constexpr int kKitchenKnifeFaded = 1004;   // … faded …
inline constexpr int kKitchenKnifeMended = 1005;  // … and the knife mended
inline constexpr int kMonstersSlowedMessage = 1006; // dungeon/magic_circles: the Secret Armlet's favoured rage circle
inline constexpr int kPocklekulMessage = 1007;      // weapons/ruby/pocklekul: the pockle summoned
inline constexpr int kShootingStarMessage = 1008;   // weapons/osmond/star_breaker: a shooting star coming down

// Notes a pack entry as the game takes it (GetPackFile): allmenu.mes and itemshop.bin are the banks given Expanded
// text; any other entry at the same address is not.
void MenuTextLoaded(std::string_view name, const void *data);

// The Expanded text of message id in the bank at buff, as ClsMes::GetTextLineDataTop gives one, or nullptr for
// the bank's own. Valid until the next call for the same bank and id.
short *MenuText(const short *buff, int id);

// Forgets the banks and the providers registered since the program started, keeping the features' own (for tests).
void MenuTextReset();

} // namespace expanded
