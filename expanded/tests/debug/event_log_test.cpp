#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <vector>

#include "app/gameloop.hpp"
#include "collisiondata.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/own_sounds.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
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
