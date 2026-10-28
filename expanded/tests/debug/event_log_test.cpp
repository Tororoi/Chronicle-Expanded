#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <vector>

#include "app/gameloop.hpp"
#include "collisiondata.hpp"
#include "ebattle.hpp"
#include "edit.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/own_sounds.hpp"
#include "fish.hpp"
#include "fishing.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "savedata.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

namespace {

// An event log open for one test, read back line by line.
class LogScope {
public:
    LogScope() {
        std::string why;
        EXPECT_TRUE(expanded::SetEventLog(path().string().c_str(), &why)) << why;
    }

    ~LogScope() { expanded::SetEventLog(nullptr); }

    LogScope(const LogScope &) = delete;
    LogScope &operator=(const LogScope &) = delete;

    std::vector<std::string> Lines() const {
        std::vector<std::string> lines;
        std::ifstream            file(path());
        for (std::string line; std::getline(file, line);) {
            lines.push_back(line);
        }
        return lines;
    }

    // The line's text after its frame: ,"event":... up to the closing brace.
    static std::string Event(const std::string &line) {
        size_t at = line.find(",\"event\"");
        return at == std::string::npos ? line : line.substr(at);
    }

private:
    std::filesystem::path path() const { return folder_.path() / "test.events.jsonl"; }

    expanded::test::ScratchDir folder_{"event_log"};
};

// A town's fishing for a test: six fish of kinds 10-15, nothing biting, the player standing with the rod and bait
// esa (esa_info's index) on the hook.
class TestFishing {
public:
    explicit TestFishing(int esa) {
        Fish = fish_.get();
        for (int i = 0; i < 6; i++) {
            fish_[i].fish_kind = 10 + i;
            fish_[i].action = FISH_ACTION_NONE;
        }
        EdMoveCharaInfo.fishing = 1;
        chara_fishing = ED_FISHING_STAND;
        esa_type = esa;
    }

    ~TestFishing() {
        Fish = saved_fish_;
        EdMoveCharaInfo.fishing = saved_on_;
        chara_fishing = saved_state_;
        esa_type = saved_esa_;
    }

    TestFishing(const TestFishing &) = delete;
    TestFishing &operator=(const TestFishing &) = delete;

    // The state the step finds, and the fish doing what it does.
    void Step(int state, int fish = -1, FishAction action = FISH_ACTION_NONE) {
        chara_fishing = state;
        if (fish >= 0) {
            fish_[fish].action = action;
        }
        expanded::EventLogFishingStep();
    }

private:
    std::unique_ptr<CFish[]> fish_ = std::make_unique<CFish[]>(6);
    CFish                   *saved_fish_ = Fish;
    int                      saved_on_ = EdMoveCharaInfo.fishing;
    int                      saved_state_ = chara_fishing;
    int                      saved_esa_ = esa_type;
};

constexpr int kMimi = 1; // esa_info's index of the Mimi

} // namespace

TEST(EventLog, AnEventIsALineOfJsonFromItsFrame) {
    LogScope log;
    ASSERT_TRUE(expanded::EventLogOn());
    expanded::LogEvent("stage", "\"step\":\"placed\"");
    expanded::LogEvent("ping");
    std::vector<std::string> lines = log.Lines();
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0], std::format("{{\"frame\":{},\"event\":\"stage\",\"step\":\"placed\"}}", GameFrameCount()));
    EXPECT_EQ(LogScope::Event(lines[1]), ",\"event\":\"ping\"}");
}

TEST(EventLog, OffItWritesNothing) {
    expanded::SetEventLog(nullptr);
    EXPECT_FALSE(expanded::EventLogOn());
    expanded::LogEvent("ping"); // nowhere to go
}

TEST(EventLog, AFileThatCannotBeOpenedIsRefused) {
    std::string why;
    EXPECT_FALSE(expanded::SetEventLog("/nonexistent/folder/log.jsonl", &why));
    EXPECT_EQ(why, "cannot be opened");
    EXPECT_FALSE(expanded::EventLogOn());
}

// Damage names the monster and the hit's owner and attack; a kill, the member it is credited to.
TEST(EventLog, DamageAndKillsNameTheMonster) {
    expanded::test::TestFloor floor;
    LogScope                  log;
    floor.unit().monster[3].hp = 14;
    floor.unit().monster[3].last_attacker = CHARA_XIAO;
    COLLISION_HIT hit = {};
    hit.owner = CHARA_XIAO;
    hit.attack_no = 6;
    expanded::EventLogDamaged(floor.unit(), 3, hit, 136);
    expanded::EventLogKilled(floor.unit(), 3);
    std::vector<std::string> lines = log.Lines();
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_NE(lines[0].find(",\"event\":\"damage\",\"monster\":3,\"kind\":"), std::string::npos);
    EXPECT_NE(lines[0].find(",\"amount\":136,\"life\":14,\"owner\":1,\"attack\":6}"), std::string::npos);
    EXPECT_NE(lines[1].find(",\"event\":\"kill\",\"monster\":3,\"kind\":"), std::string::npos);
    EXPECT_NE(lines[1].find(",\"by\":1}"), std::string::npos);
}

TEST(EventLog, AnOwnSoundIsItsProgramAndNote) {
    LogScope                     log;
    constexpr expanded::OwnSound kSound = {"sound/set/snd100.snd", "d01a_i.hd", 30, 22, 27};
    expanded::EventLogSound(kSound);
    std::vector<std::string> lines = log.Lines();
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(LogScope::Event(lines[0]), ",\"event\":\"sound\",\"program\":30,\"note\":22}");
}

// Each step reads the member out: who it is the first time and on a change, then its life and its weapon's HP as
// they change.
TEST(EventLog, TheMembersLifeAndWeaponHpAsTheyChange) {
    expanded::test::TestFloor floor;
    CUserStatus              &status = floor.status();
    status.cur_chara = CHARA_UNGAGA;
    status.hp[CHARA_UNGAGA] = 150;
    WEAPON_HAVE &weapon = status.chara_weapons[CHARA_UNGAGA][static_cast<int>(status.equipped_weapon_slot[CHARA_UNGAGA])];
    weapon.item_no = ITEM_WEAPON_HERCULES_WRATH;
    weapon.durability_f = 81.0f;
    LogScope log;
    expanded::EventLogStep();
    expanded::EventLogStep();
    weapon.durability_f = 70.33f;
    status.hp[CHARA_UNGAGA] = 140;
    expanded::EventLogStep();
    status.cur_chara = CHARA_TOAN;
    expanded::EventLogStep();
    std::vector<std::string> lines = log.Lines();
    ASSERT_EQ(lines.size(), 4u);
    EXPECT_EQ(LogScope::Event(lines[0]), ",\"event\":\"character\",\"chara\":4}");
    EXPECT_EQ(LogScope::Event(lines[1]), ",\"event\":\"life\",\"chara\":4,\"from\":150,\"to\":140}");
    EXPECT_EQ(LogScope::Event(lines[2]), ",\"event\":\"whp\",\"chara\":4,\"weapon\":356,\"from\":81.00,\"to\":70.33}");
    EXPECT_EQ(LogScope::Event(lines[3]), ",\"event\":\"character\",\"chara\":0}");
}

// Each town step reads the fishing: the cast and the float, a nibble the fish lets go of, a bite that gets away with
// the bait, the bait put back on, then a bite struck, fought, landed and caught, and the fishing quit.
TEST(EventLog, FishingAsItGoes) {
    expanded::test::TestSave save;
    LogScope                 log;
    TestFishing              fishing(kMimi);
    fishing.Step(ED_FISHING_STAND);
    fishing.Step(ED_FISHING_CAST_START);
    fishing.Step(ED_FISHING_CASTING);
    fishing.Step(ED_FISHING_WAIT_BITE);
    fishing.Step(ED_FISHING_BITE_CHECK, 2, FISH_ACTION_EAT_FOOD);
    fishing.Step(ED_FISHING_WAIT_BITE, 2, FISH_ACTION_LEAVE);
    fishing.Step(ED_FISHING_HOOKED, 3, FISH_ACTION_BITE_HOOK);
    esa_type = -1;
    fishing.Step(ED_FISHING_WAIT_BITE, 3, FISH_ACTION_LEAVE);
    esa_type = kMimi;
    fishing.Step(ED_FISHING_WAIT_BITE);
    fishing.Step(ED_FISHING_HOOKED, 4, FISH_ACTION_BITE_HOOK);
    fishing.Step(ED_FISHING_BATTLE, 4, FISH_ACTION_BATTLE);
    esa_type = -1; // the bait goes with the fish
    fishing.Step(ED_FISHING_LANDED);
    fishing.Step(ED_FISHING_RESULT);
    SaveData->AddFishingPoint(90);
    expanded::EventLogCaught(14, 71, 7);
    fishing.Step(ED_FISHING_FINISH);
    fishing.Step(ED_FISHING_INIT);
    fishing.Step(ED_FISHING_STAND);
    EdMoveCharaInfo.fishing = 0;
    fishing.Step(ED_FISHING_INIT);
    std::vector<std::string> lines = log.Lines();
    std::vector<std::string> events;
    for (const std::string &line : lines) {
        events.push_back(LogScope::Event(line));
    }
    EXPECT_EQ(events, (std::vector<std::string>{
                          ",\"event\":\"fishing\",\"step\":\"start\",\"bait\":197}",
                          ",\"event\":\"fishing\",\"step\":\"cast\",\"bait\":197}",
                          ",\"event\":\"fishing\",\"step\":\"float\",\"bait\":197}",
                          ",\"event\":\"fishing\",\"step\":\"bite\",\"fish\":2,\"kind\":12}",
                          ",\"event\":\"fishing\",\"step\":\"let_go\",\"fish\":2,\"kind\":12}",
                          ",\"event\":\"fishing\",\"step\":\"hooked\",\"fish\":3,\"kind\":13}",
                          ",\"event\":\"fishing\",\"step\":\"escaped\",\"fish\":3,\"kind\":13}",
                          ",\"event\":\"fishing\",\"step\":\"bait_lost\",\"bait\":197}",
                          ",\"event\":\"fishing\",\"step\":\"bait\",\"bait\":197}",
                          ",\"event\":\"fishing\",\"step\":\"hooked\",\"fish\":4,\"kind\":14}",
                          ",\"event\":\"fishing\",\"step\":\"fight\",\"fish\":4,\"kind\":14}",
                          ",\"event\":\"fishing\",\"step\":\"landed\",\"fish\":4,\"kind\":14}",
                          ",\"event\":\"fishing\",\"step\":\"caught\",\"kind\":14,\"size\":71,\"points\":7,\"total\":90}",
                          ",\"event\":\"fishing\",\"step\":\"ready\",\"bait\":-1}",
                          ",\"event\":\"fishing\",\"step\":\"quit\"}",
                      }));
}

// A float that sinks under a strike made too soon may take the bait with it; the rod is then ready again, as it is
// after a reel.
TEST(EventLog, AStrikeTooSoonAndAReel) {
    LogScope    log;
    TestFishing fishing(kMimi);
    fishing.Step(ED_FISHING_WAIT_BITE);
    fishing.Step(ED_FISHING_BITE_CHECK, 1, FISH_ACTION_EAT_FOOD);
    esa_type = -1;
    fishing.Step(ED_FISHING_SINK, 1, FISH_ACTION_LEAVE);
    fishing.Step(ED_FISHING_STAND);
    fishing.Step(ED_FISHING_CAST_START);
    fishing.Step(ED_FISHING_CASTING);
    fishing.Step(ED_FISHING_WAIT_BITE);
    fishing.Step(ED_FISHING_REEL_IN);
    fishing.Step(ED_FISHING_STAND);
    std::vector<std::string> events;
    for (const std::string &line : log.Lines()) {
        events.push_back(LogScope::Event(line));
    }
    EXPECT_EQ(events, (std::vector<std::string>{
                          ",\"event\":\"fishing\",\"step\":\"start\",\"bait\":197}",
                          ",\"event\":\"fishing\",\"step\":\"bite\",\"fish\":1,\"kind\":11}",
                          ",\"event\":\"fishing\",\"step\":\"missed\",\"fish\":1,\"kind\":11}",
                          ",\"event\":\"fishing\",\"step\":\"bait_lost\",\"bait\":197}",
                          ",\"event\":\"fishing\",\"step\":\"ready\",\"bait\":-1}",
                          ",\"event\":\"fishing\",\"step\":\"cast\",\"bait\":-1}",
                          ",\"event\":\"fishing\",\"step\":\"float\",\"bait\":-1}",
                          ",\"event\":\"fishing\",\"step\":\"reeled\",\"bait\":-1}",
                          ",\"event\":\"fishing\",\"step\":\"ready\",\"bait\":-1}",
                      }));
}

TEST(EventLog, OffFishingWritesNothing) {
    expanded::SetEventLog(nullptr);
    TestFishing fishing(kMimi);
    fishing.Step(ED_FISHING_CAST_START);
    expanded::EventLogCaught(2, 71, 7);
}
