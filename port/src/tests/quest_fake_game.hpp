#pragma once

// The game as a quest sees it, for tests of quests (port/src/tests/quest_framework_test.cpp, and
// Expanded's quests).

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "quest/framework.hpp"

namespace dc::test {

// The game as a quest sees it, kept in a few maps.
class FakeGame final : public quest::IGame {
public:
    std::map<int, int>                  items;
    std::map<int, int>                  stored;
    std::map<int, int>                  floors;
    std::map<int, int>                  dungeon_counts;
    std::map<std::pair<int, int>, bool> part_events;
    std::map<std::pair<int, int>, int>  parts_placed;
    std::map<std::pair<int, int>, bool> parts_complete;
    std::map<std::pair<int, int>, bool> requests;
    std::map<std::pair<int, int>, bool> villagers_home;
    std::map<int, bool>                 game_flags;
    std::map<int, int>                  game_numbers;
    std::map<std::pair<int, int>, bool> map_flags;
    std::map<int, std::string>          talk_lines;
    int                                 money = 0;
    int                                 fishing_points = 0;
    int                                 party = 1;
    int                                 pack_room = 10;
    std::optional<quest::ShopSpec>      shop;
    bool                                can_show_items = true;
    std::vector<int>                    items_shown;

    [[nodiscard]] int ItemCount(int item) const override {
        const auto it = items.find(item);
        return it == items.end() ? 0 : it->second;
    }

    bool GiveItem(int item) override {
        if (pack_room <= 0) {
            return false;
        }
        --pack_room;
        ++items[item];
        return true;
    }

    bool TakeItem(int item) override {
        if (ItemCount(item) <= 0) {
            return false;
        }
        --items[item];
        ++pack_room;
        return true;
    }

    [[nodiscard]] int Money() const override { return money; }

    bool SpendMoney(int amount) override {
        if (amount > money) {
            return false;
        }
        money -= amount;
        return true;
    }

    void EarnMoney(int amount) override { money += amount; }

    [[nodiscard]] int FishingPoints() const override { return fishing_points; }

    void EarnFishingPoints(int amount) override { fishing_points += amount; }

    [[nodiscard]] int DeepestFloor(int dungeon) const override {
        const auto it = floors.find(dungeon);
        return it == floors.end() ? -1 : it->second;
    }

    [[nodiscard]] bool HasItemAnywhere(int item) const override {
        const auto it = stored.find(item);
        return ItemCount(item) > 0 || (it != stored.end() && it->second > 0);
    }

    [[nodiscard]] int DungeonCount(int dungeon) const override {
        const auto it = dungeon_counts.find(dungeon);
        return it == dungeon_counts.end() ? 0 : it->second;
    }

    [[nodiscard]] int PartySize() const override { return party; }

    template <typename Map, typename Key>
    static auto Get(const Map &map, const Key &key) {
        const auto it = map.find(key);
        return it == map.end() ? typename Map::mapped_type{} : it->second;
    }

    [[nodiscard]] bool GameFlag(int flag) const override { return Get(game_flags, flag); }

    void SetGameFlag(int flag, bool on) override { game_flags[flag] = on; }

    [[nodiscard]] int GameNumber(int slot) const override { return Get(game_numbers, slot); }

    void SetGameNumber(int slot, int value) override { game_numbers[slot] = value; }

    [[nodiscard]] bool MapFlag(int map, int flag) const override { return Get(map_flags, std::make_pair(map, flag)); }

    void SetMapFlag(int map, int flag, bool on) override { map_flags[{map, flag}] = on; }

    [[nodiscard]] bool PartObtained(int town, int part) const override { return PartPlaced(town, part) > 0; }

    [[nodiscard]] int PartPlaced(int town, int part) const override { return Get(parts_placed, std::make_pair(town, part)); }

    [[nodiscard]] bool PartComplete(int town, int part) const override { return Get(parts_complete, std::make_pair(town, part)); }

    [[nodiscard]] bool PartEventDone(int town, int part) const override { return Get(part_events, std::make_pair(town, part)); }

    [[nodiscard]] bool RequestMet(int town, int request) const override { return Get(requests, std::make_pair(town, request)); }

    [[nodiscard]] bool VillagerHome(int town, int villager) const override {
        return Get(villagers_home, std::make_pair(town, villager));
    }

    [[nodiscard]] std::string TalkLine(int message) const override {
        const auto it = talk_lines.find(message);
        return it == talk_lines.end() ? std::string() : it->second;
    }

    void OpenShop(quest::ShopSpec spec) override { shop = std::move(spec); }

    bool ShowItemReceived(int item) override {
        if (!can_show_items) {
            return false;
        }
        items_shown.push_back(item);
        return true;
    }
};

} // namespace dc::test
