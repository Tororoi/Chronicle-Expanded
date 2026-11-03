#include <gtest/gtest.h>

#include <array>
#include <bit>
#include <cstring>
#include <initializer_list>
#include <string>
#include <vector>

#include "btactstatus.hpp"
#include "character.hpp"
#include "collisiondata.hpp"
#include "dun/gameloop.hpp"
#include "expanded/core/character_light.hpp"
#include "expanded/dungeon/dungeon_hooks.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/enemies/monster_actions.hpp"
#include "expanded/enemies/species.hpp"
#include "expanded/weapons/player_state.hpp"
#include "expanded/weapons/super_steve.hpp"
#include "expanded/weapons/toan/serpent_sword.hpp"
#include "expanded/weapons/toan/snake_backup.hpp"
#include "framevu1.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "runscript.hpp"
#include "savedata.hpp"
#include "test_support.hpp"
#include "userstatus.hpp"

using expanded::Monster;

namespace {

// The head's clips as e117e54a.cfg gives them.
std::vector<MOTION_INFO> g_clips = {
    {.start = 10,  .end = 40,  .speed = 0.2f },
    {.start = 41,  .end = 44,  .speed = 0.03f},
    {.start = 110, .end = 115, .speed = 0.05f},
    {.start = 46,  .end = 70,  .speed = 0.2f },
    {.start = 71,  .end = 85,  .speed = 0.2f },
    {.start = 86,  .end = 92,  .speed = 0.1f },
    {.start = 93,  .end = 100, .speed = 0.1f },
    {.start = 125, .end = 133, .speed = 0.1f },
    {.start = 135, .end = 155, .speed = 0.2f },
    {.start = 270, .end = 290, .speed = 0.3f },
    {.start = 176, .end = 260, .speed = 0.2f },
};
constexpr int kAppearSteps = 100; // (155 - 135) / 0.2

CFrameVu1                g_snake_frame;
std::vector<std::string> g_loaded;

bool FakeSnake(CCharacter &chara, CDataAlloc2<1> &, const char *path, const char *) {
    chara.Initialize();
    g_snake_frame.Initialize();
    chara.frame = &g_snake_frame;
    chara.motion_type.motion_info = g_clips.data();
    g_loaded.emplace_back(path);
    return true;
}

// One monster of each kind in its own slot, in play at the origin; Toan out with weapon (carrying sphere), the player
// at the origin facing +z, the guard down.
class SerpentSword : public ::testing::Test {
protected:
    explicit SerpentSword(int weapon = ITEM_WEAPON_SERPENT_SWORD, int sphere = 0) : wielding_(weapon, sphere) {
        for (int i = 0; i < 4; i++) {
            saved_pos_[i] = CharaMain.pos[i];
            CharaMain.pos[i] = 0.0f;
        }
        CharaMain.GetRotation(saved_rot_);
        CharaMain.SetRotation(0.0f, 0.0f, 0.0f);
        saved_guard_ = BtActStatus.guard_mode;
        BtActStatus.guard_mode = 0;
        SaveData->SetGameFlag(expanded::kKillerSnakeBeaten, 1);
        expanded::SetFloorCharacterLoader(FakeSnake);
        expanded::FloorCharactersClear();
        expanded::SnakeCharmerReset();
        g_loaded.clear();
    }

    ~SerpentSword() override {
        for (CRunScript &script : floor_.unit().interpreter) {
            script.prog = nullptr;
            script.func = nullptr;
        }
        expanded::SnakeCharmerReset();
        expanded::FloorCharactersClear();
        expanded::SetFloorCharacterLoader(nullptr);
        BtActStatus.guard_mode = saved_guard_;
        for (int i = 0; i < 4; i++) {
            CharaMain.pos[i] = saved_pos_[i];
        }
        CharaMain.SetRotation(saved_rot_[0], saved_rot_[1], saved_rot_[2]);
    }

    void Put(std::initializer_list<Monster> kinds) {
        int slot = 0;
        for (Monster kind : kinds) {
            floor_.SetUpModel(slot, kind);
            floor_.Spawn(slot, slot);
            floor_.unit().monster[slot].state = 2;
            floor_.unit().monster[slot].revealed = -1;
            floor_.unit().chara[slot][0].motion_type.motion_info = monster_clips_;
            slot++;
        }
    }

    void At(int monster, float x, float z) { floor_.unit().chara[monster][0].SetPosition(x, 0.0f, z); }

    void Guard(bool on) { BtActStatus.guard_mode = on ? expanded::kGuardUp : 0; }

    // A hit of the active character's weapon (or not) on monster, amount the game's.
    int Hit(int monster, int amount, bool weapon = true) {
        static char   table[10] = {};
        COLLISION_HIT hit = {};
        hit.owner = floor_.status().cur_chara;
        hit.vs_monster = weapon ? table : nullptr;
        return expanded::MonsterHitAmount(floor_.unit(), monster, hit, amount);
    }

    // A script on the monster: its retreat where the table has it, its limits (kPlayerLimit at kPlayerCell: the player 20
    // away; kPointLimit at kPointCell: 20 from where it started; kCountLimit at kCountCell: three hops), and its AI loop
    // (label 100) calling it when the player is within kTriggerDistance (kTriggerCell) on a roll under kTriggerRoll
    // (kRollCell), or, without trigger, calling nothing; then stepping in while the player is more than kStepIn away
    // (kStepInCell), shooting while within kShotRange (kShotCell), and walking in to kWalkIn (kWalkInCell).
    static constexpr int   kPlayerCell = 9, kPointCell = 20, kCountCell = 30, kTriggerCell = 10, kRollCell = 14;
    static constexpr int   kStepInCell = 19, kShotCell = 24, kWalkInCell = 29;
    static constexpr int   kCountLimit = 3, kTriggerRoll = 30;
    static constexpr float kPlayerLimit = 20.0f, kPointLimit = 20.0f, kTriggerDistance = 40.0f;
    static constexpr float kStepIn = 18.0f, kShotRange = 70.0f, kWalkIn = 50.0f;
    static constexpr int   kCode = 0x60, kProg = 0x20, kEntry = 0x10, kLabelEntry = 0x200, kStepEntry = 0x400, kShotEntry = 0x440,
                         kWalkEntry = 0x480;

    vmcode_t *RoutineCells() { return reinterpret_cast<vmcode_t *>(reinterpret_cast<char *>(program_.data()) + kCode + kEntry); }

    vmcode_t *ShotCells() { return reinterpret_cast<vmcode_t *>(reinterpret_cast<char *>(program_.data()) + kCode + kShotEntry); }

    vmcode_t *LabelCells() { return reinterpret_cast<vmcode_t *>(reinterpret_cast<char *>(program_.data()) + kCode + kLabelEntry); }

    float Float(const vmcode_t &cell) { return std::bit_cast<float>(cell.arg2); }

    CRunScript &LoadRetreat(int monster, const expanded::SnakeRetreat &retreat, bool trigger = true, bool rolled = true) {
        const int label_record = retreat.record + 0x20, step_record = retreat.record + 0x40, shot_record = retreat.record + 0x50,
                  walk_record = retreat.record + 0x60;
        program_.assign((kCode + retreat.record + 0x80) / 4, 0);
        char *base = reinterpret_cast<char *>(program_.data());
        auto *header = reinterpret_cast<RS_PROG_HEADER *>(base);
        header->code = kCode;
        header->prog = kProg;
        header->prog_num = 1;
        const RS_PROGDATA label = {100, kCode + label_record};
        std::memcpy(base + kProg, &label, sizeof(label));
        char     *code = base + kCode;
        const int record[4] = {kEntry, 0, 8, retreat.argc};
        const int routine[][3] = {
            {3,  1,  200                             }, // _SET_MOTION(motion)
            {3,  1,  retreat.motion                  },
            {4,  0,  0                               },
            {4,  0,  0                               },
            {3,  1,  10                              }, // _GET_DISTANCE(&v0): to the player
            {2,  0,  8                               },
            {21, 2,  0                               },
            {4,  0,  0                               },
            {1,  0,  8                               }, // v0 >= 20: done
            {3,  2,  std::bit_cast<int>(kPlayerLimit)},
            {14, 45, 0                               },
            {17, 0,  0                               },
            {3,  1,  10                              }, // _GET_DISTANCE(v2, v3, v4, &v5): from where it started
            {1,  2,  8                               },
            {1,  3,  8                               },
            {1,  4,  8                               },
            {2,  5,  8                               },
            {21, 5,  0                               },
            {4,  0,  0                               },
            {1,  5,  8                               }, // v5 >= 20: done
            {3,  2,  std::bit_cast<int>(kPointLimit) },
            {14, 45, 0                               },
            {17, 0,  0                               },
            {2,  1,  1                               }, // v1 = v1 + 1
            {1,  1,  1                               },
            {3,  1,  1                               },
            {6,  0,  0                               },
            {5,  0,  0                               },
            {4,  0,  0                               },
            {1,  1,  1                               }, // v1 > 3: done
            {3,  1,  kCountLimit                     },
            {14, 44, 0                               },
            {17, 0,  0                               },
            {3,  1,  0                               },
            {15, 0,  0                               }
        };
        const int label_header[4] = {kLabelEntry, 0, 4, 0};
        const int loop[][3] = {
            {3,  1,                     10                                  }, // _GET_DISTANCE(&v0)
            {2,  0,                     8                                   },
            {21, 2,                     0                                   },
            {4,  0,                     0                                   },
            {3,  1,                     180                                 }, // _GET_RAND(100, &v1)
            {3,  1,                     100                                 },
            {2,  1,                     1                                   },
            {21, 3,                     0                                   },
            {4,  0,                     0                                   },
            {1,  0,                     8                                   }, // v0 <= 40
            {3,  2,                     std::bit_cast<int>(kTriggerDistance)},
            {14, 43,                    0                                   },
            {17, kLabelEntry + 18 * 12, 0                                   },
            {1,  1,                     1                                   }, // v1 <= 30 (for a backup v1 > 30: no roll)
            {3,  1,                     kTriggerRoll                        },
            {14, rolled ? 43 : 44,      0                                   },
            {17, kLabelEntry + 18 * 12, 0                                   },
            {19, 0,                     trigger ? retreat.record : 0        }, // the retreat
            {1,  0,                     8                                   }, // v0 > 18: step in
            {3,  2,                     std::bit_cast<int>(kStepIn)         },
            {14, 44,                    0                                   },
            {17, kLabelEntry + 23 * 12, 0                                   },
            {19, 0,                     step_record                         },
            {1,  0,                     8                                   }, // v0 <= 70: shoot
            {3,  2,                     std::bit_cast<int>(kShotRange)      },
            {14, 43,                    0                                   },
            {22, 0,                     0                                   },
            {17, kLabelEntry + 29 * 12, 0                                   },
            {19, 0,                     shot_record                         },
            {3,  2,                     std::bit_cast<int>(kWalkIn)         }, // walk in to 50, for 10
            {3,  1,                     10                                  },
            {19, 0,                     walk_record                         },
            {3,  1,                     0                                   },
            {15, 0,                     0                                   }
        };
        const int step_header[4] = {kStepEntry, 0, 4, 0}, shot_header[4] = {kShotEntry, 0, 4, 0}, walk_header[4] = {kWalkEntry, 0, 4, 2};
        const int step[][3] = {
            {3,  1, 0},
            {15, 0, 0}
        };
        const int shot[][3] = {
            {3,  1, 133}, // _SET_SHOT's command
            {4,  0, 0  },
            {3,  1, 0  },
            {15, 0, 0  }
        };
        const int walk[][3] = {
            {3,  1,  10}, // _GET_DISTANCE(&v1) <= v0: there
            {2,  1,  8 },
            {21, 2,  0 },
            {4,  0,  0 },
            {1,  1,  8 },
            {1,  0,  8 },
            {14, 43, 0 },
            {17, 0,  0 },
            {3,  1,  0 },
            {15, 0,  0 }
        };
        std::memcpy(code + step_record, step_header, sizeof(step_header));
        std::memcpy(code + shot_record, shot_header, sizeof(shot_header));
        std::memcpy(code + walk_record, walk_header, sizeof(walk_header));
        std::memcpy(code + kStepEntry, step, sizeof(step));
        std::memcpy(code + kShotEntry, shot, sizeof(shot));
        std::memcpy(code + kWalkEntry, walk, sizeof(walk));
        std::memcpy(code + retreat.record, record, sizeof(record));
        std::memcpy(code + kEntry, routine, sizeof(routine));
        std::memcpy(code + label_record, label_header, sizeof(label_header));
        std::memcpy(code + kLabelEntry, loop, sizeof(loop));
        CRunScript &script = floor_.unit().interpreter[monster];
        script.load(header, nullptr, 64, nullptr, 8);
        return script;
    }

    // A script on the monster as its kind's loads (its AI loop yielding and jumping back to its top), through the load
    // hook, which adds the backup to it.
    CRunScript &LoadBackup(int monster, Monster kind) {
        constexpr int kLoop = 0x40;
        loaded_.assign(kCode + kLoop + 5 * sizeof(vmcode_t), 0);
        RS_PROG_HEADER header{};
        header.code = kCode;
        header.prog = kProg;
        header.prog_num = 1;
        std::memcpy(loaded_.data(), &header, sizeof(header));
        const RS_PROGDATA label = {100, kCode + kEntry};
        std::memcpy(loaded_.data() + kProg, &label, sizeof(label));
        const int record[4] = {kLoop, 0, 3, 0};
        std::memcpy(loaded_.data() + kCode + kEntry, record, sizeof(record));
        const vmcode_t loop[] = {
            {23, 0,     0},
            {16, kLoop, 0},
            {3,  1,     0},
            {15, 0,     0}
        };
        std::memcpy(loaded_.data() + kCode + kLoop, loop, sizeof(loop));
        char       *script = expanded::MonsterScriptLoaded(floor_.unit(), monster, expanded::No(kind), reinterpret_cast<char *>(loaded_.data()),
                                                           static_cast<int>(loaded_.size()));
        CRunScript &interpreter = floor_.unit().interpreter[monster];
        interpreter.load(reinterpret_cast<RS_PROG_HEADER *>(script), nullptr, 64, nullptr, 8);
        return interpreter;
    }

    // The added backup's cells: where it stops 40 from where it started, and after five passes.
    static constexpr int kBackupFromStartCell = 35, kBackupHopsCell = 115;

    const vmcode_t *BackupCells(CRunScript &script, const expanded::SnakeBackup &added) {
        int addr = 0;
        std::memcpy(&addr, script.code + added.record, sizeof(addr));
        return reinterpret_cast<const vmcode_t *>(script.code + addr);
    }

    std::vector<int>           program_;
    std::vector<unsigned char> loaded_;
    MOTION_INFO                monster_clips_[20] = {};
    expanded::test::TestSave   save_;

    expanded::test::Wielding  wielding_;
    expanded::test::TestFloor floor_;
    float                     saved_pos_[4] = {};
    float                     saved_rot_[4] = {};
    int                       saved_guard_ = 0;
};

class SuperSteveSerpent : public SerpentSword {
protected:
    SuperSteveSerpent() : SerpentSword(ITEM_WEAPON_SUPER_STEVE, ITEM_WEAPON_SERPENT_SWORD) {}
};

class NoSerpent : public SerpentSword {
protected:
    NoSerpent() : SerpentSword(ITEM_WEAPON_DAGGER) {}
};

} // namespace

TEST_F(SerpentSword, SnakeSlayerKillsAuntieMeduInOneHit) {
    Put({Monster::AUNTIE_MEDU, Monster::AUNTIE_MEDU_ENHANCED, Monster::WEREWOLF});
    EXPECT_EQ(Hit(0, 12), floor_.unit().monster[0].hp);
    EXPECT_EQ(Hit(1, 0), floor_.unit().monster[1].hp); // even a hit that would do nothing
    EXPECT_EQ(Hit(2, 12), 12);
    EXPECT_EQ(Hit(0, 12, false), 12); // not a weapon hit (an item, a monster)
}

TEST_F(SerpentSword, NothingUntilTheKillerSnakeIsBeaten) {
    Put({Monster::AUNTIE_MEDU});
    At(0, 0.0f, 40.0f);
    SaveData->SetGameFlag(expanded::kKillerSnakeBeaten, 0);
    EXPECT_FALSE(expanded::SerpentSwordActive());
    EXPECT_EQ(Hit(0, 12), 12);
    Guard(true);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::SnakeCharmerUp());
    EXPECT_TRUE(g_loaded.empty());
    SaveData->SetGameFlag(expanded::kKillerSnakeBeaten, 1);
    expanded::SnakeCharmerStep();
    EXPECT_TRUE(expanded::SnakeCharmerUp());
    EXPECT_EQ(Hit(0, 12), floor_.unit().monster[0].hp);
}

TEST_F(NoSerpent, SnakeSlayerNeedsTheSerpentSword) {
    Put({Monster::AUNTIE_MEDU});
    EXPECT_EQ(Hit(0, 12), 12);
}

TEST_F(SuperSteveSerpent, SuperSteveCarriesSnakeSlayer) {
    Put({Monster::AUNTIE_MEDU});
    floor_.status().cur_chara = CHARA_XIAO;
    EXPECT_EQ(Hit(0, 12), floor_.unit().monster[0].hp);
}

TEST_F(SerpentSword, TheSnakeComesWithTheGuard) {
    Put({Monster::WEREWOLF});
    At(0, 0.0f, 200.0f);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::SnakeCharmerUp());
    EXPECT_TRUE(g_loaded.empty());
    Guard(true);
    expanded::SnakeCharmerStep();
    ASSERT_TRUE(expanded::SnakeCharmerUp());
    ASSERT_EQ(g_loaded.size(), 1u);
    EXPECT_EQ(g_loaded[0], expanded::kKillerSnakeFile);
    Guard(false);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::SnakeCharmerUp());
}

TEST_F(SerpentSword, EveryKindThatCanBackOffDoes) {
    // A too-close backup of its own.
    const expanded::SnakeRetreat *chariot = expanded::SnakeRetreatOf(expanded::No(Monster::PIRATES_CHARIOT));
    ASSERT_NE(chariot, nullptr);
    EXPECT_STREQ(chariot->script, "e25a");
    EXPECT_EQ(chariot->record, 0x2084);
    // The backup added to its script, its hop its own backup motion.
    for (Monster kind : {Monster::MASTER_JACKET, Monster::WEREWOLF, Monster::ROCKANOFF, Monster::KING_MIMIC_DS}) {
        const expanded::SnakeRetreat *added = expanded::SnakeRetreatOf(expanded::No(kind));
        ASSERT_NE(added, nullptr) << expanded::No(kind);
        EXPECT_EQ(added->record, 0);
        EXPECT_EQ(added->motion, 2);
    }
    EXPECT_EQ(expanded::SnakeRetreatOf(expanded::No(Monster::HALLOWEEN))->motion, 13);
    EXPECT_EQ(expanded::SnakeRetreatOf(expanded::No(Monster::CAVE_BAT)), nullptr); // no backup motion
    // An Expanded Gemron's its stand-in's: Holy Gemron's script, rewritten.
    for (Monster gemron : {Monster::CRYSTAL_GEMRON, Monster::ATLA_GEMRON, Monster::BOMB_GEMRON}) {
        EXPECT_EQ(expanded::SnakeRetreatOf(expanded::No(gemron)), expanded::SnakeRetreatOf(expanded::No(Monster::GEMRON_HOLY)));
    }
}

TEST_F(SerpentSword, ExpandedGemronsRunTheirStandInsRetreat) {
    // Each loads under Holy Gemron's script name, so its loaded script is checked for the retreat as Holy Gemron's is.
    const expanded::SnakeRetreat &holy = *expanded::SnakeRetreatOf(expanded::No(Monster::GEMRON_HOLY));
    for (Monster gemron : {Monster::CRYSTAL_GEMRON, Monster::ATLA_GEMRON, Monster::BOMB_GEMRON}) {
        ASSERT_NE(expanded::Species(expanded::No(gemron)), nullptr);
        EXPECT_STREQ(expanded::Species(expanded::No(gemron))->script_name, holy.script);
    }
}

TEST_F(SerpentSword, TheirOwnAiRetreatsToTheEdgeOfReach) {
    Put({Monster::GYON});
    CMonstorUnit &unit = floor_.unit();
    CRunScript   &script = LoadRetreat(0, *expanded::SnakeRetreatOf(expanded::No(Monster::GYON)));
    vmcode_t     *loop = LabelCells();
    vmcode_t     *routine = RoutineCells();
    At(0, 0.0f, 40.0f);
    Guard(true);
    expanded::SnakeCharmerStep();

    // Near the player, for its own step: its AI rolls its retreat within the reach (the roll its own), its routine goes on
    // to the reach's edge, it approaches only to kSnakeApproachTo and holds its shot; its script steps itself.
    unit.script_state[0] = 1;
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_FLOAT_EQ(Float(loop[kTriggerCell]), expanded::kSnakeRetreatRadius);
    EXPECT_EQ(loop[kRollCell].arg2, kTriggerRoll);
    EXPECT_FLOAT_EQ(Float(routine[kPlayerCell]), expanded::kSnakeRetreatTo);
    EXPECT_FLOAT_EQ(Float(routine[kPointCell]), expanded::kSnakeRetreatUnlimited);
    EXPECT_EQ(routine[kCountCell].arg2, expanded::kSnakeRetreatHops);
    EXPECT_FLOAT_EQ(Float(loop[kStepInCell]), expanded::kSnakeApproachTo);
    EXPECT_FLOAT_EQ(Float(loop[kShotCell]), kShotRange); // a check that shoots keeps its distance
    vmcode_t *shot = ShotCells();
    EXPECT_EQ(shot[0].op, 3); // the shot routine returns at once
    EXPECT_EQ(shot[1].op, 15);
    EXPECT_FLOAT_EQ(Float(loop[kWalkInCell]), expanded::kSnakeApproachTo);
    // After its step its script (the kind's, shared) is as it was.
    expanded::MonsterScriptStepped(unit, 0);
    EXPECT_FLOAT_EQ(Float(loop[kTriggerCell]), kTriggerDistance);
    EXPECT_FLOAT_EQ(Float(routine[kPlayerCell]), kPlayerLimit);
    EXPECT_FLOAT_EQ(Float(routine[kPointCell]), kPointLimit);
    EXPECT_EQ(routine[kCountCell].arg2, kCountLimit);
    EXPECT_FLOAT_EQ(Float(loop[kStepInCell]), kStepIn);
    EXPECT_EQ(shot[0].arg2, 133);
    EXPECT_EQ(shot[1].op, 4);
    EXPECT_FLOAT_EQ(Float(loop[kWalkInCell]), kWalkIn);
    // It walks as its script has it: nothing taken off a step toward the player.
    MONSTOR &jacket = unit.monster[0];
    jacket.movement_speed = 1.0f;
    jacket.movement[0] = 0.0f, jacket.movement[2] = -1.0f;
    expanded::MonsterMoveChecked(unit, 0);
    EXPECT_FLOAT_EQ(jacket.movement_speed, 1.0f);

    // Out of reach in its retreat: the routine goes on to the edge, its trigger as it was; at the edge it stops at its
    // next check.
    funcdata in_routine = {.addr = kEntry};
    script.func = &in_routine;
    At(0, 0.0f, expanded::kSnakeRetreatRadius + 5.0f);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_FLOAT_EQ(Float(loop[kTriggerCell]), kTriggerDistance);
    EXPECT_FLOAT_EQ(Float(routine[kPointCell]), expanded::kSnakeRetreatUnlimited);
    expanded::MonsterScriptStepped(unit, 0);
    At(0, 0.0f, expanded::kSnakeRetreatTo);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_FLOAT_EQ(Float(routine[kPointCell]), 0.0f);
    EXPECT_EQ(routine[kCountCell].arg2, 0);
    EXPECT_FLOAT_EQ(Float(routine[kPlayerCell]), expanded::kSnakeRetreatTo);
    expanded::MonsterScriptStepped(unit, 0);
    script.func = nullptr;

    // Out of reach, not retreating: its approach and shot still kept as it might walk in, its routine as it is; beyond
    // that, and with the guard down, its script as it is.
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_FLOAT_EQ(Float(loop[kTriggerCell]), kTriggerDistance);
    EXPECT_FLOAT_EQ(Float(routine[kPointCell]), kPointLimit);
    EXPECT_FLOAT_EQ(Float(loop[kWalkInCell]), expanded::kSnakeApproachTo);
    expanded::MonsterScriptStepped(unit, 0);
    At(0, 0.0f, expanded::kSnakeApproachRadius + 5.0f);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_FLOAT_EQ(Float(loop[kWalkInCell]), kWalkIn);
    At(0, 0.0f, 40.0f);
    Guard(false);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_FLOAT_EQ(Float(loop[kTriggerCell]), kTriggerDistance);
    EXPECT_EQ(ShotCells()[0].arg2, 133);
}

TEST_F(SerpentSword, ATooCloseBackupStartsAtTheApproachsEnd) {
    Put({Monster::PIRATES_CHARIOT});
    CMonstorUnit &unit = floor_.unit();
    LoadRetreat(0, *expanded::SnakeRetreatOf(expanded::No(Monster::PIRATES_CHARIOT)), true, false);
    At(0, 0.0f, 40.0f);
    Guard(true);
    expanded::SnakeCharmerStep();
    // Called whenever the player is close, no roll: it backs off as it comes in to where its approach ends.
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_FLOAT_EQ(Float(LabelCells()[kTriggerCell]), expanded::kSnakeApproachTo);
    EXPECT_EQ(LabelCells()[kRollCell].arg2, kTriggerRoll);
    expanded::MonsterScriptStepped(unit, 0);
}

TEST_F(SerpentSword, ChargesAreHeldLikeShots) {
    Put({Monster::GYON});
    CMonstorUnit                 &unit = floor_.unit();
    const expanded::SnakeRetreat &gyon = *expanded::SnakeRetreatOf(expanded::No(Monster::GYON));
    constexpr int                 kChargeEntry = 0x500;
    vmcode_t                     *charge = nullptr;
    // Its step-in a charge playing motion: _GET_POSITION(-2, &v0, &v1, &v2), _SET_MOTION(motion), _SET_MOVE(v0, v1, v2, 0.5f).
    auto step_in = [&](int motion) {
        LoadRetreat(0, gyon);
        char     *code = reinterpret_cast<char *>(program_.data()) + kCode;
        const int cells[][3] = {
            {3,  1, 11                      },
            {3,  1, 2                       },
            {11, 0, 0                       },
            {2,  0, 8                       },
            {2,  1, 8                       },
            {2,  2, 8                       },
            {21, 5, 0                       },
            {4,  0, 0                       },
            {3,  1, 200                     },
            {3,  1, motion                  },
            {21, 2, 0                       },
            {4,  0, 0                       },
            {3,  1, 32                      },
            {1,  0, 8                       },
            {1,  1, 8                       },
            {1,  2, 8                       },
            {3,  2, std::bit_cast<int>(0.5f)},
            {21, 5, 0                       },
            {4,  0, 0                       },
            {3,  1, 0                       },
            {15, 0, 0                       }
        };
        const int header[4] = {kChargeEntry, 0, 4, 0};
        std::memcpy(code + gyon.record + 0x40, header, sizeof(header));
        charge = reinterpret_cast<vmcode_t *>(code + kChargeEntry);
        std::memcpy(charge, cells, sizeof(cells));
        At(0, 0.0f, 40.0f);
        Guard(true);
        expanded::SnakeCharmerStep();
        unit.script_state[0] = 1;
        EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    };

    // An attack's motion at the player: it returns at once for the step, and is as it was after.
    step_in(16);
    EXPECT_EQ(charge[0].op, 3);
    EXPECT_EQ(charge[1].op, 15);
    expanded::MonsterScriptStepped(unit, 0);
    EXPECT_EQ(charge[0].arg2, 11);
    EXPECT_EQ(charge[1].op, 3);
    // Its walk, or its backup, is no charge.
    for (int motion : {1, gyon.motion}) {
        step_in(motion);
        EXPECT_EQ(charge[1].op, 3);
        expanded::MonsterScriptStepped(unit, 0);
    }
}

TEST_F(SerpentSword, AWalkInFromFartherOutComesInTo70) {
    Put({Monster::GYON});
    CMonstorUnit                 &unit = floor_.unit();
    const expanded::SnakeRetreat &gyon = *expanded::SnakeRetreatOf(expanded::No(Monster::GYON));
    constexpr float               kFarther = 90.0f;
    // Its step-in from 90: a step, kept where it is; a walk-in, in to kSnakeApproachTo, so it comes in to its backup.
    for (bool walks : {false, true}) {
        LoadRetreat(0, gyon);
        vmcode_t *loop = LabelCells();
        loop[kStepInCell].arg2 = std::bit_cast<int>(kFarther);
        if (walks) {
            loop[kStepInCell + 3].arg2 = gyon.record + 0x60;
        }
        At(0, 0.0f, 40.0f);
        Guard(true);
        expanded::SnakeCharmerStep();
        unit.script_state[0] = 1;
        EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
        EXPECT_FLOAT_EQ(Float(loop[kStepInCell]), walks ? expanded::kSnakeApproachTo : kFarther);
        expanded::MonsterScriptStepped(unit, 0);
        EXPECT_FLOAT_EQ(Float(loop[kStepInCell]), kFarther);
    }
}

TEST_F(SerpentSword, ARetreatItsAiNeverCallsIsHeldInstead) {
    Put({Monster::GYON});
    CMonstorUnit &unit = floor_.unit();
    LoadRetreat(0, *expanded::SnakeRetreatOf(expanded::No(Monster::GYON)), false);
    At(0, 0.0f, 40.0f);
    Guard(true);
    expanded::SnakeCharmerStep();
    EXPECT_TRUE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_EQ(unit.monster[0].requested_motion, 8);
    EXPECT_FLOAT_EQ(Float(LabelCells()[kTriggerCell]), kTriggerDistance);
}

TEST_F(SerpentSword, HeldMonstersStopWhatTheyAreDoing) {
    Put({Monster::CAVE_BAT});
    CMonstorUnit &unit = floor_.unit();
    monster_clips_[5] = {.start = 0, .end = 10, .speed = 1.0f}; // a one-shot: an attack
    At(0, 0.0f, 40.0f);
    Guard(true);
    expanded::SnakeCharmerStep();
    unit.script_state[0] = 1;
    expanded::PlayMonsterMotion(unit, 0, 5, expanded::kMotionHoldLast | expanded::kMotionCut);
    unit.chara[0][0].motion_type.state.time = 4.0f;
    // Mid-swing: the stagger, cross-faded in, in the swing's place.
    EXPECT_TRUE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_EQ(unit.monster[0].requested_motion_flags, expanded::kMotionHoldLast);
}

TEST_F(SerpentSword, TheSnakeCutsOffWhatTheyAreDoing) {
    Put({Monster::GYON});
    CMonstorUnit &unit = floor_.unit();
    CRunScript   &script = LoadRetreat(0, *expanded::SnakeRetreatOf(expanded::No(Monster::GYON)));
    MONSTOR      &gyon = unit.monster[0];
    auto          mid_attack = [&] {
        unit.script_state[0] = 1;
        gyon.movement_speed = 0.5f;
        gyon.turn_speed = 0.3f;
    };
    At(0, 0.0f, 40.0f);
    Guard(true);
    expanded::SnakeCharmerStep();
    // Mid-attack when the snake comes: stopped, its script back to its AI loop (which backs it off this step).
    mid_attack();
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_EQ(unit.script_state[0], 0);
    EXPECT_FLOAT_EQ(gyon.movement_speed, 0.0f);
    EXPECT_FLOAT_EQ(gyon.turn_speed, 0.0f);
    expanded::MonsterScriptStepped(unit, 0);
    // Once a guard: after that its script carries on.
    mid_attack();
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_EQ(unit.script_state[0], 1);
    expanded::MonsterScriptStepped(unit, 0);
    // The next guard cuts it off again, but not in its retreat.
    Guard(false);
    expanded::SnakeCharmerStep();
    Guard(true);
    expanded::SnakeCharmerStep();
    funcdata in_routine = {.addr = kEntry};
    script.func = &in_routine;
    mid_attack();
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_EQ(unit.script_state[0], 1);
    expanded::MonsterScriptStepped(unit, 0);
    script.func = nullptr;
}

TEST_F(SerpentSword, KindsWithoutARetreatStaggerThenIdle) {
    Put({Monster::CAVE_BAT});
    CMonstorUnit &unit = floor_.unit();
    monster_clips_[8] = {.start = 0, .end = 3, .speed = 1.0f}; // its stagger, three steps
    At(0, 0.0f, 40.0f);
    Guard(true);
    expanded::SnakeCharmerStep();
    MONSTOR &bat = unit.monster[0];
    bat.movement_speed = 1.0f;
    unit.script_state[0] = 1;

    // Taken from its script: its stagger, then its idle, standing where it is.
    ASSERT_TRUE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_EQ(unit.script_state[0], 0);
    EXPECT_EQ(bat.requested_motion, 8);
    EXPECT_FLOAT_EQ(bat.movement_speed, 0.0f);
    expanded::MonsterStepTaken(unit, 0);
    expanded::MonsterStepTaken(unit, 0);
    EXPECT_EQ(bat.requested_motion, 8);
    expanded::MonsterStepTaken(unit, 0);
    EXPECT_EQ(bat.requested_motion, expanded::kSnakeIdle);
    for (int step = 0; step < 60; step++) {
        ASSERT_TRUE(expanded::MonsterStepTaken(unit, 0)) << step;
    }
    EXPECT_EQ(bat.requested_motion, expanded::kSnakeIdle);

    // Struck, it staggers again.
    unit.script_state[0] = 1;
    EXPECT_TRUE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_EQ(bat.requested_motion, 8);
    EXPECT_EQ(unit.script_state[0], 0);

    // The guard down, its script takes it back from the top.
    Guard(false);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_EQ(unit.script_state[0], 0);
}

TEST_F(SerpentSword, TheBackupAddedToItsScriptBacksItOff) {
    Put({Monster::ROCKANOFF});
    CMonstorUnit                &unit = floor_.unit();
    CRunScript                  &script = LoadBackup(0, Monster::ROCKANOFF);
    const expanded::SnakeBackup *added = expanded::SnakeBackupOf(script.code);
    ASSERT_NE(added, nullptr);
    vmcode_t &trigger = *reinterpret_cast<vmcode_t *>(script.code + added->trigger);
    At(0, 0.0f, 40.0f);
    Guard(true);
    expanded::SnakeCharmerStep();
    // Near the player, for its own step: its loop calls the backup within the approach's end, and the backup's limits
    // (40 from where it started, five passes) go on.
    EXPECT_FLOAT_EQ(Float(trigger), 0.0f);
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_FLOAT_EQ(Float(trigger), expanded::kSnakeApproachTo);
    const vmcode_t *backup = BackupCells(script, *added);
    EXPECT_FLOAT_EQ(Float(backup[kBackupFromStartCell]), expanded::kSnakeRetreatUnlimited);
    EXPECT_EQ(backup[kBackupHopsCell].arg2, expanded::kSnakeRetreatHops);
    expanded::MonsterScriptStepped(unit, 0);
    EXPECT_FLOAT_EQ(Float(trigger), 0.0f);
    EXPECT_FLOAT_EQ(Float(backup[kBackupFromStartCell]), 40.0f);
    EXPECT_EQ(backup[kBackupHopsCell].arg2, 4);
    // The guard down: as its script was.
    Guard(false);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_FLOAT_EQ(Float(trigger), 0.0f);
}

TEST_F(SerpentSword, MimicsBackOffOnceAwake) {
    Put({Monster::MIMIC_DBC});
    CMonstorUnit &unit = floor_.unit();
    CRunScript   &script = LoadBackup(0, Monster::MIMIC_DBC);
    vmcode_t     &trigger = *reinterpret_cast<vmcode_t *>(script.code + expanded::SnakeBackupOf(script.code)->trigger);
    At(0, 0.0f, 40.0f);
    Guard(true);
    expanded::SnakeCharmerStep();
    // Still waking from its chest (its appear clip, or about to start it): its script as it is.
    unit.monster[0].requested_motion = expanded::kMimicWake;
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_FLOAT_EQ(Float(trigger), 0.0f);
    unit.monster[0].requested_motion = 0;
    unit.monster[0].revealed = 1;
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_FLOAT_EQ(Float(trigger), 0.0f);
    // Awake: it backs off.
    unit.monster[0].revealed = -1;
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    EXPECT_FLOAT_EQ(Float(trigger), expanded::kSnakeApproachTo);
    expanded::MonsterScriptStepped(unit, 0);
}

TEST_F(SerpentSword, WakingMimicsOnlyStopComingOn) {
    Put({Monster::KING_MIMIC_DBC});
    CMonstorUnit &unit = floor_.unit();
    unit.monster[0].requested_motion = expanded::kKingMimicWake;
    At(0, 0.0f, 40.0f);
    Guard(true);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::MonsterStepTaken(unit, 0));
    MONSTOR &mimic = unit.monster[0];
    // Straight at the player: it stops.
    mimic.movement_speed = 1.0f;
    mimic.movement[0] = 0.0f, mimic.movement[2] = -1.0f;
    expanded::MonsterMoveChecked(unit, 0);
    EXPECT_FLOAT_EQ(mimic.movement_speed, 0.0f);
    // Slanting in: only the sideways part is left.
    mimic.movement_speed = 1.0f;
    mimic.movement[0] = 0.6f, mimic.movement[2] = -0.8f;
    expanded::MonsterMoveChecked(unit, 0);
    EXPECT_FLOAT_EQ(mimic.movement[0], 1.0f);
    EXPECT_NEAR(mimic.movement[2], 0.0f, 1e-6f);
    EXPECT_FLOAT_EQ(mimic.movement_speed, 0.6f);
    // Away, as its script would have it: kept.
    mimic.movement_speed = 1.0f;
    mimic.movement[0] = 0.0f, mimic.movement[2] = 1.0f;
    expanded::MonsterMoveChecked(unit, 0);
    EXPECT_FLOAT_EQ(mimic.movement[2], 1.0f);
    EXPECT_FLOAT_EQ(mimic.movement_speed, 1.0f);
}

TEST_F(SerpentSword, TheHeadFadesInAndOutWithTheGuard) {
    Put({Monster::WEREWOLF});
    At(0, 0.0f, 200.0f);
    Guard(true);
    for (int step = 1; step <= expanded::kSnakeFadeSteps; step++) {
        expanded::SnakeCharmerStep();
        EXPECT_NEAR(expanded::SnakeCharmerShown(), static_cast<float>(step) / expanded::kSnakeFadeSteps, 1e-5f) << step;
    }
    // It glows white as it shows, and others do not.
    CCharacter *head = expanded::EnterFloorCharacter(expanded::kKillerSnakeFile, expanded::kKillerSnakeCfg);
    ASSERT_NE(head, nullptr);
    float ambient[4] = {};
    expanded::CharacterLight(*head, ambient);
    EXPECT_FLOAT_EQ(ambient[0], expanded::kSnakeGlow);
    CCharacter other;
    float      none[4] = {};
    expanded::CharacterLight(other, none);
    EXPECT_FLOAT_EQ(none[0], 0.0f);
    // Down with the guard it fades out as long, the monsters free of it at once.
    Guard(false);
    for (int step = expanded::kSnakeFadeSteps - 1; step >= 0; step--) {
        expanded::SnakeCharmerStep();
        EXPECT_FALSE(expanded::SnakeCharmerUp());
        EXPECT_NEAR(expanded::SnakeCharmerShown(), static_cast<float>(step) / expanded::kSnakeFadeSteps, 1e-5f) << step;
    }
    float after[4] = {};
    expanded::CharacterLight(*head, after);
    EXPECT_FLOAT_EQ(after[0], 0.0f);
}

TEST_F(SerpentSword, TheGlowDiscGrowsAndShrinksSlowerThanTheFade) {
    Put({Monster::WEREWOLF});
    At(0, 0.0f, 200.0f);
    Guard(true);
    for (int step = 1; step <= expanded::kSnakeGlowSteps; step++) {
        expanded::SnakeCharmerStep();
        EXPECT_NEAR(expanded::SnakeCharmerGlow(), static_cast<float>(step) / expanded::kSnakeGlowSteps, 1e-5f) << step;
    }
    // The head gone with its fade, the disc still shrinking away after it.
    Guard(false);
    for (int step = 0; step < expanded::kSnakeFadeSteps; step++) {
        expanded::SnakeCharmerStep();
    }
    EXPECT_FLOAT_EQ(expanded::SnakeCharmerShown(), 0.0f);
    EXPECT_GT(expanded::SnakeCharmerGlow(), 0.0f);
    for (int step = expanded::kSnakeFadeSteps; step < expanded::kSnakeGlowSteps; step++) {
        expanded::SnakeCharmerStep();
    }
    EXPECT_FLOAT_EQ(expanded::SnakeCharmerGlow(), 0.0f);
}

TEST_F(SerpentSword, TheHeadAppearsThenOpensItsMouthNowAndThen) {
    Put({Monster::WEREWOLF});
    At(0, 0.0f, 200.0f);
    Guard(true);
    expanded::SnakeCharmerStep();
    CCharacter *head = expanded::EnterFloorCharacter(expanded::kKillerSnakeFile, expanded::kKillerSnakeCfg);
    ASSERT_NE(head, nullptr);
    // The appearance, held on its last frame from then on.
    EXPECT_EQ(head->motion_no, expanded::kSnakeAppear);
    EXPECT_NE(head->motion_flags & 2, 0);
    for (int step = 1; step < kAppearSteps + 2 * expanded::kSnakeMouthPeriod; step++) {
        expanded::SnakeCharmerStep();
        ASSERT_EQ(head->motion_no, expanded::kSnakeAppear) << step;
    }
    // The mouth shut after the appearance until its cycle comes round, then open, held and shut again.
    for (int step = 0; step < expanded::kSnakeMouthPeriod; step++) {
        expanded::SnakeCharmerStep();
        if (expanded::SnakeCharmerMouth() > 0.99f) {
            break;
        }
    }
    ASSERT_GT(expanded::SnakeCharmerMouth(), 0.99f);
    for (int step = 0; step < expanded::kSnakeMouthHold + expanded::kSnakeMouthEase; step++) {
        expanded::SnakeCharmerStep();
    }
    EXPECT_LT(expanded::SnakeCharmerMouth(), 0.01f);
    // A monster coming near opens it at once.
    expanded::SnakeCharmerStep();
    At(0, 0.0f, 50.0f);
    for (int step = 0; step < expanded::kSnakeMouthEase; step++) {
        expanded::SnakeCharmerStep();
    }
    EXPECT_GT(expanded::SnakeCharmerMouth(), 0.99f);
    // Gone with the guard, up again with its appearance.
    Guard(false);
    for (int step = 0; step < expanded::kSnakeFadeSteps; step++) {
        expanded::SnakeCharmerStep();
    }
    Guard(true);
    head->motion_no = 0;
    expanded::SnakeCharmerStep();
    EXPECT_EQ(head->motion_no, expanded::kSnakeAppear);
}

TEST_F(SuperSteveSerpent, SuperSteveCarriesSnakeCharmer) {
    Put({Monster::WEREWOLF});
    At(0, 0.0f, 40.0f);
    floor_.status().cur_chara = CHARA_XIAO;
    Guard(true);
    expanded::SnakeCharmerStep();
    EXPECT_TRUE(expanded::SnakeCharmerUp());
    // It takes no step nearer.
    MONSTOR &wolf = floor_.unit().monster[0];
    wolf.movement_speed = 1.0f;
    wolf.movement[0] = 0.0f, wolf.movement[2] = -1.0f;
    expanded::MonsterMoveChecked(floor_.unit(), 0);
    EXPECT_FLOAT_EQ(wolf.movement_speed, 0.0f);
}

TEST_F(SerpentSword, OffWithTheWeaponAbilities) {
    Put({Monster::AUNTIE_MEDU});
    At(0, 0.0f, 40.0f);
    expanded::test::OptionScope options;
    ASSERT_TRUE(options.Set(expanded::kWeaponAbilities, false));
    EXPECT_EQ(Hit(0, 12), 12);
    Guard(true);
    expanded::SnakeCharmerStep();
    EXPECT_FALSE(expanded::SnakeCharmerUp());
}
