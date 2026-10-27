#include "expanded/core/menu_text.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <utility>
#include <vector>

#include "clsmes.hpp"
#include "itemdata.hpp"
#include "localize/localize.hpp"
#include "sysmes.hpp"
#include "text/gametext.hpp"

namespace expanded {

namespace {

constexpr int kDescriptionBase = 500;

enum class Bank {
    AllMenu,
    Shop,
};

// A bank as it was taken, with its message count to tell it from a later file at the same address.
struct Loaded {
    Bank bank;
    s16  count;
};

// Built on first use: providers register from other files' static initialisers.
struct Registry {
    std::map<const void *, Loaded>                           banks;
    std::map<int, WeaponTextProvider>                        weapons;
    std::map<int, MenuTextProvider>                          messages;
    std::map<int, MenuTextProvider>                          system;
    std::map<std::pair<const void *, int>, std::vector<s16>> built;
};

Registry &Text() {
    static Registry registry;
    return registry;
}

bool SameName(std::string_view a, std::string_view b) {
    return std::ranges::equal(a, b, [](char x, char y) {
        return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
    });
}

// The bank's own text of message id, as ClsMes::GetTextLineDataTop finds it.
const s16 *Find(const s16 *buff, int id) {
    const int  count = buff[0];
    const s16 *body = &buff[1];
    for (int i = 0; i < count; i++) {
        if (id == body[i * 2 + 1]) {
            return &body[count] + body[i * 2 + 2];
        }
    }
    return nullptr;
}

// The weapon whose description is message id, or 0.
int WeaponOf(int id) {
    for (int item = ITEM_WEAPON_START; item < ITEM_WEAPON_END; item++) {
        const COM_ITEM_INFO *info = GetCommonItemInfo(item);
        if (info != nullptr && info->msg + kDescriptionBase == id) {
            return item;
        }
    }
    return 0;
}

// The description lines of a weapon: its provider's, else the language files'.
std::optional<std::string> WeaponLines(int item_no) {
    if (auto provider = Text().weapons.find(item_no); provider != Text().weapons.end()) {
        if (std::optional<std::string> text = provider->second(item_no)) {
            return text;
        }
    }
    std::string text = LocalizeText(WeaponDescriptionKey(item_no), "");
    if (text.empty()) {
        return std::nullopt;
    }
    return text;
}

} // namespace

std::string WeaponDescriptionKey(int item_no) {
    return "expanded.weapon." + std::to_string(item_no) + ".description";
}

void SetWeaponDescription(int item_no, WeaponTextProvider provider) {
    if (provider != nullptr) {
        Text().weapons[item_no] = provider;
    } else {
        Text().weapons.erase(item_no);
    }
}

void SetMenuMessage(int message_id, MenuTextProvider provider) {
    if (provider != nullptr) {
        Text().messages[message_id] = provider;
    } else {
        Text().messages.erase(message_id);
    }
}

void SetSystemMessage(int message_id, MenuTextProvider provider) {
    if (provider != nullptr) {
        Text().system[message_id] = provider;
    } else {
        Text().system.erase(message_id);
    }
}

void MenuTextLoaded(std::string_view name, const void *data) {
    if (data == nullptr) {
        return;
    }
    const s16 count = static_cast<const s16 *>(data)[0];
    if (SameName(name, "allmenu.mes")) {
        Text().banks[data] = {Bank::AllMenu, count};
    } else if (SameName(name, "itemshop.bin")) {
        Text().banks[data] = {Bank::Shop, count};
    } else {
        Text().banks.erase(data);
    }
}

short *MenuText(const short *buff, int id) {
    if (auto message = Text().system.find(id); buff != nullptr && buff == SystemMessageWindow().buff && message != Text().system.end()) {
        std::optional<std::string> text = message->second();
        if (!text) {
            return nullptr;
        }
        std::vector<s16> &kept = Text().built[{buff, id}];
        kept.clear();
        GameTextEncode(*text, kept);
        return kept.data();
    }
    auto bank = buff != nullptr ? Text().banks.find(buff) : Text().banks.end();
    if (bank == Text().banks.end() || bank->second.count != buff[0]) {
        return nullptr;
    }
    std::vector<s16> codes;
    if (auto message = Text().messages.find(id); bank->second.bank == Bank::AllMenu && message != Text().messages.end()) {
        if (std::optional<std::string> text = message->second()) {
            GameTextEncode(*text, codes);
        }
    }
    if (codes.empty() && id > kDescriptionBase) {
        const int  item = WeaponOf(id);
        const s16 *retail = item != 0 ? Find(buff, id) : nullptr;
        std::optional<std::string> lines = retail != nullptr ? WeaponLines(item) : std::nullopt;
        if (lines) {
            // The disc's name line, then the Expanded lines.
            for (const s16 *at = retail; *at != MES_CODE_END && *at != MES_CODE_NEWLINE; at++) {
                codes.push_back(*at);
            }
            codes.push_back(MES_CODE_NEWLINE);
            GameTextEncode(*lines, codes);
        }
    }
    if (codes.empty()) {
        return nullptr;
    }
    std::vector<s16> &kept = Text().built[{buff, id}];
    kept = std::move(codes);
    return kept.data();
}

void MenuTextReset() {
    Text().banks.clear();
    Text().weapons.clear();
    Text().messages.clear();
    Text().system.clear();
    Text().built.clear();
}

} // namespace expanded
